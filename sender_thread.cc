/* Copyright (C) 2010 Sergei Golubchik and Monty Program Ab

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "http_monitor.h"
#include <sql_acl.h>
#include <sql_parse.h>
#include <time.h>
#include "sys_tbl.h"
#include <ma_dyncol.h>
#include <string>
#include <iostream>



namespace http_monitor {
static THD *thd= 0;                ///< background thread thd
static my_thread_id thd_thread_id; ///< its thread_id

static size_t needed_size= 20480;

static const time_t startup_interval= 0;     ///< in seconds (5 minutes)
static const time_t first_interval= 0;   ///< in seconds (one day)

//static const time_t first_interval= 60*60*24;   ///< in seconds (one day)
//static const time_t interval= 60*60*24*7;       ///< in seconds (one week)
static time_t interval= 10;       ///< in seconds (one week)

/**
  reads the rows from a table and puts them, concatenated, in a String

  @note
  1. only supports two column tables - no less, no more.
  2. it emulates mysql -e "select * from..." and thus it separates
     columns with \t and starts the output with column names.
*/
static int table_to_string(TABLE *table, String *result)
{
  bool res;
  char buff1[MAX_FIELD_WIDTH], buff2[MAX_FIELD_WIDTH];
  String str1(buff1, sizeof(buff1), system_charset_info);
  String str2(buff2, sizeof(buff2), system_charset_info);

  res= table->file->ha_rnd_init(1);

  dbug_tmp_use_all_columns(table, table->read_set);

  while(!res && !table->file->ha_rnd_next(table->record[0]))
  {
    table->field[0]->val_str(&str1);
    table->field[1]->val_str(&str2);
    if (result->reserve(str1.length() + str2.length() + 3))
      res= 1;
    else
    {
      result->qs_append(str1.ptr(), str1.length());
      result->qs_append('\t');
      result->qs_append(str2.ptr(), str2.length());
      result->qs_append('\n');
    }
  }

  res = res || result->append('\n');

  /*
    Note, "|=" and not "||" - because we want to call ha_rnd_end()
    even if res is already 1.
  */
  res |= table->file->ha_rnd_end();

  return res;
}

static int table_to_json(TABLE *table, String *result)
{
  bool res;
  char buff1[MAX_FIELD_WIDTH], buff2[MAX_FIELD_WIDTH];
  String str1(buff1, sizeof(buff1), system_charset_info);
  String str2(buff2, sizeof(buff2), system_charset_info);
 
  res= table->file->ha_rnd_init(1);

  dbug_tmp_use_all_columns(table, table->read_set);
  result->append("{\"information\":[\n");
  HISTO_INDEX ++;
  GALERA_STATUS=0;
  REPL_STATUS=0;
  while(!res && !table->file->ha_rnd_next(table->record[0]))
  {
    table->field[0]->val_str(&str1);
    table->field[1]->val_str(&str2);
    
    if (!strcmp(str1.ptr(), "WSREP_ON") && !strcmp(str2.ptr(), "ON") && !strcmp(str2.ptr(), "4")) GALERA_STATUS=1;
    if (!strcmp(str1.ptr(), "SLAVE_RUNNING") && !strcmp(str2.ptr(), "ON")) REPL_STATUS=1;  
    
    if (result->reserve(str1.length() + str2.length() + 25))
      res= 1;
    else
    {
      if ( (table->field[2]->val_int()) == 4){
        result->qs_append("{\"name\":\"");
        result->qs_append(str1.ptr(), str1.length());
        result->qs_append("\",\"value\":\"");
        result->qs_append(str2.ptr(), str2.length());
        result->qs_append("\"},\n");
       }
    }
  }
   
   result->chop();
   result->chop();
    res = res || result->append("]}\n");


  /*
    Note, "|=" and not "||" - because we want to call ha_rnd_end()
    even if res is already 1.
  */
  res |= table->file->ha_rnd_end();

  return res;
}


/**
  Initialize the THD and TABLE_LIST

  The structures must be sufficiently initialized for create_tmp_table()
  and fill_http_monitor() to work.
*/
static int prepare_for_fill(TABLE_LIST *tables)
{
  /*
    Add our thd to the list, for it to be visible in SHOW PROCESSLIST.
    But don't generate thread_id every time - use the saved value
    (every increment of global thread_id counts as a new connection
    in SHOW STATUS and we want to avoid skewing the statistics)
  */
  thd->thread_id= thd->variables.pseudo_thread_id= thd_thread_id;
  mysql_mutex_lock(&LOCK_thread_count);
  thread_count++;
  threads.append(thd);
  mysql_mutex_unlock(&LOCK_thread_count);
  thd->thread_stack= (char*) &tables;
  if (thd->store_globals())
    return 1;

  thd->mysys_var->current_cond= &sleep_condition;
  thd->mysys_var->current_mutex= &sleep_mutex;
  thd->proc_info="http_monitor";
  thd->set_command(COM_SLEEP);
  thd->system_thread= SYSTEM_THREAD_EVENT_WORKER; // whatever
  thd->set_time();
  thd->init_for_queries();
  thd->real_id= pthread_self();
  thd->db= NULL;
  thd->db_length= 0;
  thd->security_ctx->host_or_ip= "";
  thd->security_ctx->db_access= DB_ACLS;
  thd->security_ctx->master_access= ~NO_ACCESS;
  bzero((char*) &thd->net, sizeof(thd->net));
  lex_start(thd);
  mysql_init_select(thd->lex);

  tables->init_one_table(INFORMATION_SCHEMA_NAME.str,
                         INFORMATION_SCHEMA_NAME.length,
                         i_s_http_monitor->table_name,
                         strlen(i_s_http_monitor->table_name),
                         0, TL_READ);
  tables->schema_table= i_s_http_monitor;
  tables->table= i_s_http_monitor->create_table(thd, tables);
  if (!tables->table)
    return 1;

  tables->table->pos_in_table_list= tables;
  return 0;
}





/**
  Try to detect if this thread is going down

  which can happen for different reasons:
  * plugin is being unloaded
  * mysqld server is being shut down
  * the thread is being killed

*/
static bool going_down()
{
  return shutdown_plugin || shutdown_in_progress || (thd && thd->killed);
}

/**
  just like sleep, but waits on a condition and checks "plugin shutdown" status
*/
static int slept_ok(time_t sec)
{
  struct timespec abstime;
  int ret= 0;

  set_timespec(abstime, sec);

  mysql_mutex_lock(&sleep_mutex);
  while (!going_down() && ret != ETIMEDOUT)
    ret= mysql_cond_timedwait(&sleep_condition, &sleep_mutex, &abstime);
  mysql_mutex_unlock(&sleep_mutex);

  return !going_down();
}

/**
  create a http_monitor report and send it to all specified urls

  If "when" argument is not null, only it and the server uid are sent.
  Otherwise a full report is generated.
*/
static void send_report(const char *when)
{
  TABLE_LIST tables;
  int i, last_todo;
 
  String str; 
  str.alloc(needed_size); // preallocate it to avoid many small mallocs
   
  /*
    on startup and shutdown the server may not be completely
    initialized, and full report won't work.
    We send a short status notice only.
  */
  if (when)
  {
    str.length(0);
    str.append(STRING_WITH_LEN("FEEDBACK_SERVER_UID"));
    str.append('\t');
    str.append(server_uid_buf);
    str.append('\n');
    str.append(STRING_WITH_LEN("FEEDBACK_WHEN"));
    str.append('\t');
    str.append(when);
    str.append('\n');
    str.append(STRING_WITH_LEN("FEEDBACK_USER_INFO"));
    str.append('\t');
    str.append('\n');
  }
  else
  {
    /*
      otherwise, prepare the THD and TABLE_LIST,
      create and fill the temporary table with data just like
      SELECT * FROM IFROEMATION_SCHEMA.http_monitor is doing,
      read and concatenate table data into a String.
    */
    if (!(thd= new THD()))
      return;
      
    if (prepare_for_fill(&tables))
      goto ret;

   /* if (fill_http_monitor(thd, &tables, NULL))
      goto ret;

    if (table_to_json(tables.table, &str))
      goto ret;
*/
    needed_size= (size_t)(str.length() * 1.1);
   
    loadContent(thd);
 
    HTTP_REPORT.copy(str);
    
    
    sql_print_information("http_monitor *send_report*:  %ul  ",http_monitor::history_index);
    if ( http_monitor::history_length-1< http_monitor::history_index && send_mail) {
        http_monitor::history_index=0;
        http_monitor::history_uptime++;
    
    Server **nodes= (Server**)alloca(mysql_servers_count*sizeof(Server*));
    memcpy(nodes, mysql_servers, mysql_servers_count*sizeof(Server*));
   
    int j, last_node;
    last_node= http_monitor::mysql_servers_count -1 ;
    for (j= 0; j <= last_node;)
    {
      http_monitor::Server *url_node= nodes[j];
      j++;
      String Server; 
      std::string stdServer;
      stdServer.append(url_node->getPath().str);
      Server.append(remove_letter(stdServer,'/').c_str() );
    
        
     http_monitor::Server **todo= (http_monitor::Server**)alloca(http_monitor::smtp_servers_count*sizeof(http_monitor::Server*));
     memcpy(todo, http_monitor::smtp_servers, http_monitor::smtp_servers_count*sizeof(http_monitor::Server*));
    
        int i, last_todo;
        last_todo= http_monitor::smtp_servers_count -1 ;
        String strHistory;
        strHistory.append((char*) "/history_");
        strHistory.append(Server);
        String strTemplateQuery;
        strTemplateQuery.append((char*) "/template_");
        strTemplateQuery.append(Server);
        strTemplateQuery.append((char*) "_queries");
        
        String strTemplateStatus;
        strTemplateStatus.append((char*) "/template_");
        strTemplateStatus.append(Server);
        strTemplateStatus.append((char*) "_status");
        
        String strTemplateVariables;
        strTemplateVariables.append((char*) "/template_");
        strTemplateVariables.append(Server);
        strTemplateVariables.append((char*) "_variables");
    
        String strTemplateExplain;
        strTemplateExplain.append((char*) "/template_");
        strTemplateExplain.append(Server);
        strTemplateExplain.append((char*) "_explain");
      
        String strTemplateMaster;
        strTemplateMaster.append((char*) "/template_");
        strTemplateMaster.append(Server);
        strTemplateMaster.append((char*) "_master");
        
        String strTemplateSlave;
        strTemplateSlave.append((char*) "/template_");
        strTemplateSlave.append(Server);
        strTemplateSlave.append((char*) "_slave");
        
        String strTemplateColumns;
        strTemplateColumns.append((char*) "/template_");
        strTemplateColumns.append(Server);
        strTemplateColumns.append((char*) "_columns");
        
        String strTemplateTables;
        strTemplateTables.append((char*) "/template_");
        strTemplateTables.append(Server);
        strTemplateTables.append((char*) "_tables");
        
        String strTemplateDict;
        strTemplateDict.append((char*) "/template_");
        strTemplateDict.append(Server);
        strTemplateDict.append((char*) "_dict");
        
        String strTemplateKey;
        strTemplateKey.append((char*) "/template_");
        strTemplateKey.append(Server);
        strTemplateKey.append((char*) "_key_column");
        
        
        
        http_content_row *content = getContent(&strHistory);
        http_content_row *rtemplateQuery = getContent(&strTemplateQuery);
        http_content_row *rtemplateStatus = getContent(&strTemplateStatus);
        http_content_row *rtemplateVariables = getContent(&strTemplateVariables);
        http_content_row *rtemplateExplain = getContent(&strTemplateExplain);
        http_content_row *rtemplateMaster = getContent(&strTemplateMaster);
        http_content_row *rtemplateSlave = getContent(&strTemplateSlave);
        http_content_row *rtemplateColumns = getContent(&strTemplateColumns);
        http_content_row *rtemplateTables = getContent(&strTemplateTables);
        http_content_row *rtemplateDict = getContent(&strTemplateDict);
        http_content_row  *rtemplateKey = getContent(&strTemplateKey);
              
        if (error_log)
            sql_print_information("http_monitor *send_report*: sending from mail index %ul  ",http_monitor::smtp_servers_count );
        for (i= 0; i <= last_todo;)
        {
          http_monitor::Server *url= todo[i];
            if (content != NULL)     
                url->send(content->content.c_str(), content->content.length());
            if (rtemplateQuery != NULL && (http_monitor::history_uptime % http_monitor::interval_send_query)==0 &&  http_monitor::send_query) 
                url->send(rtemplateQuery->content.c_str(), rtemplateQuery->content.length());
            if (rtemplateStatus != NULL && (http_monitor::history_uptime % http_monitor::interval_send_status)==0 &&  http_monitor::send_status) 
                url->send(rtemplateStatus->content.c_str(), rtemplateStatus->content.length());
            if (rtemplateVariables != NULL && (http_monitor::history_uptime % http_monitor::interval_send_variable)==0 &&  http_monitor::send_variable) 
                url->send(rtemplateVariables->content.c_str(), rtemplateVariables->content.length());
            if (rtemplateExplain != NULL && (http_monitor::history_uptime % http_monitor::interval_send_query)==0 &&  http_monitor::send_query) 
                url->send(rtemplateExplain->content.c_str(), rtemplateExplain->content.length());
            if (rtemplateMaster != NULL && (http_monitor::history_uptime % http_monitor::interval_send_replication)==0 &&  http_monitor::send_replication) 
                url->send(rtemplateMaster->content.c_str(), rtemplateMaster->content.length());
            if (rtemplateSlave != NULL && (http_monitor::history_uptime % http_monitor::interval_send_replication)==0 &&  http_monitor::send_replication)
                url->send(rtemplateSlave->content.c_str(), rtemplateSlave->content.length());
            if (rtemplateColumns != NULL && (http_monitor::history_uptime % http_monitor::interval_send_schema)==0 &&  http_monitor::send_schema)
                url->send(rtemplateColumns->content.c_str(), rtemplateColumns->content.length());
            if (rtemplateTables != NULL && (http_monitor::history_uptime % http_monitor::interval_send_schema)==0 &&  http_monitor::send_schema)
                url->send(rtemplateTables->content.c_str(), rtemplateTables->content.length());
            if (rtemplateDict != NULL && (http_monitor::history_uptime % http_monitor::interval_send_schema)==0 &&  http_monitor::send_dictionary)
                url->send(rtemplateDict->content.c_str(), rtemplateDict->content.length());
            if (rtemplateKey != NULL && (http_monitor::history_uptime % http_monitor::interval_send_schema)==0 &&  http_monitor::send_dictionary)
                url->send(rtemplateKey->content.c_str(), rtemplateKey->content.length());
          
            i++;
        }

      }
    }
    free_tmp_table(thd, tables.table);
    tables.table= 0;
  }

  
  
ret:
  if (thd)
  {
    if (tables.table)
      free_tmp_table(thd, tables.table);
    /*
      clean up, free the thd.
      reset all thread local status variables to minimize
      the effect of the background thread on SHOW STATUS.
    */
    mysql_mutex_lock(&LOCK_thread_count);
    bzero(&thd->status_var, sizeof(thd->status_var));
    thread_count--;
    thd->killed= KILL_CONNECTION;
    mysql_cond_broadcast(&COND_thread_count);
    mysql_mutex_unlock(&LOCK_thread_count);
    delete thd;
    thd= 0;
  }
}

/**
  background sending thread
*/
pthread_handler_t background_thread(void *arg __attribute__((unused)))
{
  if (my_thread_init())
    return 0;

  mysql_mutex_lock(&LOCK_thread_count);
  thd_thread_id= thread_id++;
  mysql_mutex_unlock(&LOCK_thread_count);

  if (slept_ok(refresh_rate))
  {

    if (slept_ok(refresh_rate))
    {
      send_report(NULL);
      first_run=0;
      while(slept_ok(refresh_rate))
        send_report(NULL);
    }

    send_report("shutdown");
  }

  my_thread_end();
  pthread_exit(0);
  return 0;
}

} // namespace http_monitor

