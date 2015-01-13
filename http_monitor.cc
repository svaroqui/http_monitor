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
#include "sql_show.h"
#include "handler.h" 
#include "set_var.h"


#ifdef HTTP_MONITOR_HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif


/* MySQL functions/variables not declared in mysql_priv.h */
int fill_variables(THD *thd, TABLE_LIST *tables, COND *cond);
int fill_status(THD *thd, TABLE_LIST *tables, COND *cond);



extern ST_SCHEMA_TABLE schema_tables[];




namespace http_monitor {
    String HTTP_REPORT;
    int GALERA_STATUS = 0;
    int REPL_STATUS = 0;
    int HISTO_INDEX = 0;
    char server_uid_buf[SERVER_UID_SIZE + 1]; ///< server uid will be written here
  
    /* backing store for system variables */
    static char *server_uid = server_uid_buf; 
    char *conn_user = 0;
    char *conn_password = 0;
    char *conn_host = 0;
    char *conn_socket = 0;
    char *smtp_email_from=0;
    char *smtp_email_to=0;
    char *smtp_certificat=0;
    char *node_name=0;
    char *node_group=0;
    char *port = 0;
    char *aes_key = 0; 
    char *salt_key = 0;
    ulong hash_len;
    char *notification_email=0;
    char *bo_user=0;
    char *bo_password=0;
    static char *smtp_address = 0;
    static char *node_address = 0; 
    static char *http_address = 0; 
    char error_log;
    char smtp_authentification;
    char send_mail;
    char use_spider;
    char use_vmime;
    char use_aes_encrypt;
    char first_run=1;
    char *smtp_user = 0;
    char *smtp_password = 0;
   
    
    uint conn_port=3306; 
    ulong send_timeout; 
    ulong refresh_rate;
    ulong interval_send_query;
    ulong interval_send_schema;
    ulong interval_send_replication;
    ulong interval_send_variable;
    ulong interval_send_status;
    ulong interval_get_query;
    ulong interval_get_explain;
    ulong interval_get_schema;
    ulong interval_get_replication;
    ulong interval_get_variable;
    ulong interval_get_status;
    char send_query;
    char send_schema;
    char send_replication;
    char send_variable;
    char send_status;
    char send_dictionary;
    char http_content;
    ulong history_length;
    
    ulong history_uptime;
    ulong history_index;
 
    Server **mysql_servers; ///< list of servers to monitor
    uint mysql_servers_count;
    Server **smtp_servers; ///< list of servers to send the report to by smtp
    uint smtp_servers_count;
    Server **http_servers; ///< list of servers to send the report to by html
    uint http_servers_count;

    /**
      these three are used to communicate the shutdown signal to the
      background thread
     */
    mysql_mutex_t sleep_mutex;
    mysql_cond_t sleep_condition;
    volatile bool shutdown_plugin;
    static pthread_t sender_thread;
    static pthread_t http_thread;

#ifdef HAVE_PSI_INTERFACE
    static PSI_mutex_key key_sleep_mutex;
    static PSI_mutex_info mutex_list[] ={
        { &key_sleep_mutex, "sleep_mutex", PSI_FLAG_GLOBAL}};

    static PSI_cond_key key_sleep_cond;
    static PSI_cond_info cond_list[] ={
        { &key_sleep_cond, "sleep_condition", PSI_FLAG_GLOBAL}};

    static PSI_thread_key key_sender_thread;
    static PSI_thread_info thread_list[] ={
        {&key_sender_thread, "sender_thread", 0}};
#endif
    static COND * make_http_cond_for_info_schema(COND *cond, TABLE_LIST *table);
    static int fill_http_variables(THD *thd, TABLE_LIST *tables, COND *cond);
#define mariadb_dyncol_value_init(V) (V)->type= DYN_COL_NULL
    ST_SCHEMA_TABLE *i_s_http_monitor; ///< table descriptor for our I_S table
    
    /*
      the column names *must* match column names in GLOBAL_VARIABLES and
      GLOBAL_STATUS tables otherwise condition pushdown below will not work
     */
  
     static ST_FIELD_INFO http_monitor_fields[] ={
        {"VARIABLE_NAME", 255, MYSQL_TYPE_STRING, 0, 0, 0, 0},
        {"VARIABLE_VALUE", 1024, MYSQL_TYPE_STRING, 0, 0, 0, 0},
        {"VARIABLE_TYPE", 1, MYSQL_TYPE_TINY, 0, 0, 0, 0},
        {0, 0, MYSQL_TYPE_NULL, 0, 0, 0, 0}
    };
    
    
    
    static COND * const OOM = (COND*) 1;

    
    
    
#ifdef HTTP_MONITOR_HAVE_OPENSSL
#define SSL_MUTEX_TYPE       pthread_mutex_t
#define SSL_MUTEX_SETUP(x,y) pthread_mutex_init(&(x),&(y))
#define SSL_MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define SSL_MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define SSL_MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define SSL_THREAD_ID        pthread_self( )

static SSL_MUTEX_TYPE  *pMutexSSL;
 
static void           SSLLockingFunction            (int mode, int n, const char * file, int line);
static unsigned long  SSLID_Function                (void);


/*============================================================================*/
/*
* InitOpenSSL
* /brief Initialisatioon de OpenSSL
 * A appeler une fois en debut de programme
* /param[in] bMultiThread indique si le programme possede plusieurs threads accedant aux API OpenSSL pour initialiser les mutex utilises par OpenSSL
* /return  true=OK false =KO
*/
 
bool InitOpenSSL(bool bMultiThread)
   {
   pthread_mutexattr_t MutAttr;
 
   SSL_load_error_strings();
   SSL_library_init();
 
   if(bMultiThread)
      {
      if((pMutexSSL = (SSL_MUTEX_TYPE *)OPENSSL_malloc(CRYPTO_num_locks( ) * sizeof(SSL_MUTEX_TYPE))) == (SSL_MUTEX_TYPE *)NULL)
         {
         return false;
         }
 
      for (int i = 0; i < CRYPTO_num_locks(); i++)
         {
         pthread_mutexattr_init( &MutAttr );
         pthread_mutexattr_settype( &MutAttr,PTHREAD_MUTEX_NORMAL/*PTHREAD_MUTEX_ERRORCHECK_NP*/);
         SSL_MUTEX_SETUP(pMutexSSL[i],MutAttr);
         pthread_mutexattr_destroy( &MutAttr );
         }
 
      CRYPTO_set_id_callback(SSLID_Function);
      CRYPTO_set_locking_callback(SSLLockingFunction);
      }
 
   return true; 
   }
 
/*============================================================================*/
/*
* CloseOpenSSL
* /brief Nettoyage des allocations effectuees par InitOpenSSL
* A appeler une fois en fin de programme
*/
 
void CloseOpenSSL()
   {
   if(pMutexSSL)
      {
      CRYPTO_set_id_callback(NULL);
      CRYPTO_set_locking_callback(NULL);
 
      for (int i = 0; i < CRYPTO_num_locks( ); i++) SSL_MUTEX_CLEANUP(pMutexSSL[i]);
 
      OPENSSL_free(pMutexSSL);
      pMutexSSL = (SSL_MUTEX_TYPE *)NULL;
      }
  
   }
 
 
/*============================================================================*/
 
static void SSLLockingFunction(int mode, int n, const char * file, int line)
   {
   if (mode & CRYPTO_LOCK)
      {
      SSL_MUTEX_LOCK (pMutexSSL[n]);
      }
   else
      {
      SSL_MUTEX_UNLOCK(pMutexSSL[n]);
      }
   }
 
/*============================================================================*/

  static unsigned long SSLID_Function(void)
   {
   return((unsigned long)SSL_THREAD_ID);
   }
#endif

    
    
    /**
      Generate the COND tree for the condition pushdown

      This function takes a list of strings and generates an Item tree
      corresponding to the following expression:

        field LIKE str1 OR field LIKE str2 OR field LIKE str3 OR ...

      where 'field' is the first field in the table - VARIABLE_NAME field -
      and str1, str2... are strings from the list.

      This condition is used to filter the selected rows, emulating

        SELECT * FROM INFORMATION_SCHEMA.GLOBAL_VARIABLES WHERE ...
     */
 
 /* static COND* make_cond(THD *thd, TABLE_LIST *tables, LEX_STRING *filter) {
        Item_cond_or *res = NULL;
        Name_resolution_context nrc;
        const char *db = tables->db, *table = tables->alias,
                *field = tables->table->field[0]->field_name;
        CHARSET_INFO *cs = &my_charset_latin1;

        if (!filter->str)
            return 0;

        nrc.init();
        nrc.resolve_in_table_list_only(tables);

        res = new Item_cond_or();
        if (!res)
            return OOM;

        for (; filter->str; filter++) {
            Item_field *fld = new Item_field(&nrc, db, table, field);
            Item_string *pattern = new Item_string(filter->str, filter->length, cs);
            Item_string *escape = new Item_string("\\", 1, cs);

            if (!fld || !pattern || !escape)
                return OOM;

            Item_func_like *like = new Item_func_like(fld, pattern, escape, 0);

            if (!like)
                return OOM;

            res->add(like);
        }

        if (res->fix_fields(thd, (Item**) & res))
            return OOM;

        return res;
    }
   */

    /**
      System variables that we want to see in the http_monitor report
     */
    static LEX_STRING vars_filter[] = {
        {C_STRING_WITH_LEN("auto\\_increment%")},
        {C_STRING_WITH_LEN("binlog\\_format")},
        {C_STRING_WITH_LEN("character\\_set\\_%")},
        {C_STRING_WITH_LEN("collation%")},
        {C_STRING_WITH_LEN("engine\\_condition\\_pushdown")},
        {C_STRING_WITH_LEN("event\\_scheduler")},
        {C_STRING_WITH_LEN("http_monitor\\_%")},
        {C_STRING_WITH_LEN("ft\\_m%")},
        {C_STRING_WITH_LEN("have\\_%")},
        {C_STRING_WITH_LEN("%\\_size")},
        {C_STRING_WITH_LEN("innodb_f%")},
        {C_STRING_WITH_LEN("%\\_length%")},
        {C_STRING_WITH_LEN("%\\_timeout")},
        {C_STRING_WITH_LEN("large\\_%")},
        {C_STRING_WITH_LEN("lc_time_names")},
        {C_STRING_WITH_LEN("log")},
        {C_STRING_WITH_LEN("log_bin")},
        {C_STRING_WITH_LEN("log_output")},
        {C_STRING_WITH_LEN("log_slow_queries")},
        {C_STRING_WITH_LEN("log_slow_time")},
        {C_STRING_WITH_LEN("lower_case%")},
        {C_STRING_WITH_LEN("max_allowed_packet")},
        {C_STRING_WITH_LEN("max_connections")},
        {C_STRING_WITH_LEN("max_prepared_stmt_count")},
        {C_STRING_WITH_LEN("max_sp_recursion_depth")},
        {C_STRING_WITH_LEN("max_user_connections")},
        {C_STRING_WITH_LEN("max_write_lock_count")},
        {C_STRING_WITH_LEN("myisam_recover_options")},
        {C_STRING_WITH_LEN("myisam_repair_threads")},
        {C_STRING_WITH_LEN("myisam_stats_method")},
        {C_STRING_WITH_LEN("myisam_use_mmap")},
        {C_STRING_WITH_LEN("net\\_%")},
        {C_STRING_WITH_LEN("new")},
        {C_STRING_WITH_LEN("old%")},
        {C_STRING_WITH_LEN("optimizer%")},
        {C_STRING_WITH_LEN("profiling")},
        {C_STRING_WITH_LEN("query_cache%")},
        {C_STRING_WITH_LEN("secure_auth")},
        {C_STRING_WITH_LEN("slow_launch_time")},
        {C_STRING_WITH_LEN("sql%")},
        {C_STRING_WITH_LEN("storage_engine")},
        {C_STRING_WITH_LEN("sync_binlog")},
        {C_STRING_WITH_LEN("table_definition_cache")},
        {C_STRING_WITH_LEN("table_open_cache")},
        {C_STRING_WITH_LEN("thread_handling")},
        {C_STRING_WITH_LEN("time_zone")},
        {C_STRING_WITH_LEN("timed_mutexes")},
        {C_STRING_WITH_LEN("version%")},
        {0, 0}
    };

    /**
      Status variables that we want to see in the http_monitor report

      (empty list = no WHERE condition)
     */
    static LEX_STRING status_filter[] = {
        {0, 0}};


    /**
      Fill I_S table with data

      This function works by invoking fill_variables() and
      fill_status() of the corresponding I_S tables - to have
      their data UNION-ed in the same target table.
      After that it invokes our own fill_* functions
      from the utils.cc - to get the data that aren't available in the
      I_S.GLOBAL_VARIABLES and I_S.GLOBAL_STATUS.
     */

 

   /* int fill_http_monitor(THD *thd, TABLE_LIST *tables, COND *unused) {
        int res;
        COND *cond;

        tables->schema_table = schema_tables + SCH_GLOBAL_VARIABLES;
        cond = make_cond(thd, tables, vars_filter);
        res = (cond == OOM) ? 1 : fill_variables(thd, tables, cond);

        tables->schema_table = schema_tables + SCH_GLOBAL_STATUS;
        if (!res) {
            cond = make_cond(thd, tables, status_filter);
            res = (cond == OOM) ? 1 : fill_status(thd, tables, cond);
        }

        tables->schema_table = i_s_http_monitor;
        res = res || fill_plugin_version(thd, tables)
                || fill_misc_data(thd, tables)
                || fill_linux_info(thd, tables);

        return res;
    }*/
    
    
    /**
       plugin initialization function
     */
    static int init(void *p) {
        /* initialize the I_S descriptor structure */
     
        i_s_http_monitor = (ST_SCHEMA_TABLE*) p;
        i_s_http_monitor->fields_info = http_monitor_fields; ///< field descriptor
       // i_s_http_monitor->fill_table = fill_http_monitor; ///< how to fill the I_S table
        i_s_http_monitor->idx_field1 = 0; ///< virtual index on the 1st col
      
        
        #ifdef HAVE_PSI_INTERFACE
        #define PSI_register(X) \
          if(PSI_server) PSI_server->register_ ## X("http_monitor", X ## _list, array_elements(X ## _list))
        #else
        #define PSI_register(X) /* no-op */
        #endif

        PSI_register(mutex);
        PSI_register(cond);
        PSI_register(thread);
        #ifdef HTTP_MONITOR_HAVE_OPENSSL
        InitOpenSSL(true);        
        #endif
        // Prepare callbacks structure. We have only one callback, the rest are NULL.
        send_timeout = 60;
        //refresh_rate = 60;

        
        if (calculate_server_uid(server_uid_buf))
            return 1;
        if (strcmp(salt_key,"" ) ==0) 
         salt_key = strdup(server_uid_buf);
        if (strcmp(aes_key,"" ) ==0) 
         aes_key = strdup(server_uid_buf);
        bo_password  = strdup(server_uid_buf);
        prepare_linux_info();

        mysql_servers_count = 0;
        if (*node_address) {
            // now we split node_address on spaces and store them in Url objects
            int slot;
            char *s, *e;

            for (s = node_address, mysql_servers_count = 1; *s; s++)
                if (*s == ',')
                    mysql_servers_count++;
            mysql_servers = (Server **) my_malloc(mysql_servers_count * sizeof (Server*), MYF(MY_WME));
            if (!node_address)
                return 1;

            for (s = node_address, e = node_address + 1, slot = 0; e[-1]; e++) {
                if (*e == 0 || *e == ',') {
                       sql_print_information("http_monitor plugin: Adding database server to monitoring '%.*s'", (int) (e - s), s);
                    
                    if (e > s && (mysql_servers[slot] = Server::create(s, e - s))){ 
                       
                       slot++;
                     } 
                    else { 
                        if (e > s){
                           
                        }
                        mysql_servers_count--;
                    }
                    s = e + 1;
                }

             }
             
            
            if (*smtp_address) {
            smtp_servers_count = 0 ; 
            for (s = smtp_address, smtp_servers_count = 1; *s; s++)
                if (*s == ',')
                    smtp_servers_count++;
            smtp_servers = (Server **) my_malloc(smtp_servers_count * sizeof (Server*), MYF(MY_WME));
            if (!smtp_address)
                return 1;

            for (s = smtp_address, e = smtp_address + 1, slot = 0; e[-1]; e++) {
                if (*e == 0 || *e == ',') {
                    sql_print_information("http_monitor plugin: Adding mail server to report '%.*s'", (int) (e - s), s);
                       
                    if (e > s && (smtp_servers[slot] = Server::create(s, e - s))){ 
                       slot++;
                    } 
                    else { 
                        if (e > s){
                         
                        }
                        smtp_servers_count--;
                    }
                    s = e + 1;
                }

             }
            }
            
            
             if (*http_address) {
            http_servers_count = 0 ; 
            for (s = http_address, http_servers_count = 1; *s; s++)
                if (*s == ',')
                    http_servers_count++;
            http_servers = (Server **) my_malloc(http_servers_count * sizeof (Server*), MYF(MY_WME));
            if (!http_address)
                return 1;

            for (s = http_address, e = http_address + 1, slot = 0; e[-1]; e++) {
                if (*e == 0 || *e == ',') {
                    sql_print_information("http_monitor plugin: Adding mail server to report '%.*s'", (int) (e - s), s);
                       
                    if (e > s && (http_servers[slot] = Server::create(s, e - s))){ 
                       slot++;
                    } 
                    else { 
                        if (e > s){
                         
                        }
                        http_servers_count--;
                    }
                    s = e + 1;
                }

             }
            }
      
            // create a background thread to handle node_address, if any

            mysql_mutex_init(0, &sleep_mutex, 0);
            mysql_cond_init(0, &sleep_condition, 0);
            shutdown_plugin = false;

            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            if (pthread_create(&sender_thread, &attr, background_thread, 0) != 0) {
                sql_print_error("http_monitor plugin: failed to start a background thread");
                return 1;
            }
            pthread_attr_t attr2;
            pthread_attr_init(&attr2);
            pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE);
            if (pthread_create(&http_thread, &attr2, background_http_thread, 0) != 0) {
                sql_print_error("http_monitor plugin: failed to start a http thread");
                return 1;
            }
        }

        return 0;
    }

    /**
       plugin deinitialization function
     */
    static int free(void *p) {
        #ifdef HTTP_MONITOR_HAVE_OPENSSL
        CloseOpenSSL();
        #endif
        if (mysql_servers_count) {
            
      
            mysql_mutex_lock(&sleep_mutex);
            shutdown_plugin = true;
            mysql_cond_signal(&sleep_condition);
            mysql_mutex_unlock(&sleep_mutex);
            pthread_join(sender_thread, NULL);
            pthread_join(http_thread, NULL);
            mysql_mutex_destroy(&sleep_mutex);
            mysql_cond_destroy(&sleep_condition);
   /*for (uint j = 0; j < smtp_servers_count; j++)
            {   
                sql_print_error("smtp_servers");
                delete smtp_servers[j];
               
           } 
     */    
            for (uint i = 0; i < mysql_servers_count; i++)
             {       delete mysql_servers[i];
               sql_print_error("mysql_servers");
           } 
            my_free(mysql_servers);
            my_free(smtp_servers);
        }
        return 0;
    }



     static MYSQL_SYSVAR_STR(server_uid, server_uid,
            PLUGIN_VAR_READONLY | PLUGIN_VAR_NOCMDOPT,
            "Automatically calculated server unique id hash.", NULL, NULL, 0);
     static MYSQL_SYSVAR_STR(node_address, node_address, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Comma separated URLs to send the http_monitor report to.", NULL, NULL,
            "mysql://127.0.0.1:3306/localhost");
     static MYSQL_SYSVAR_STR(http_address, http_address, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Comma separated URLs to send the http_monitor report to.", NULL, NULL,
             "http://88.181.24.43:80/feedback");
     static MYSQL_SYSVAR_STR(smtp_address, smtp_address, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Comma separated URLs to send the http_monitor report to.", NULL, NULL,
            "smtp://smtp.scrambledb.org:587");
     static MYSQL_SYSVAR_STR(smtp_user, smtp_user, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "smtp user login", NULL, NULL,
            "support@scrambledb.org");
     static MYSQL_SYSVAR_STR(smtp_email_from, smtp_email_from, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "smtp email FROM field", NULL, NULL,
            "monitor@scrambledb.org");
     static MYSQL_SYSVAR_STR(smtp_email_to, smtp_email_to, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "smtp email TO field", NULL, NULL,
            "monitor@scrambledb.org");
     static MYSQL_SYSVAR_STR(smtp_password, smtp_password, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "smtp password login", NULL, NULL,
            "support");
     static MYSQL_SYSVAR_BOOL(smtp_authentification, smtp_authentification,   PLUGIN_VAR_OPCMDARG, 
            "Authenticate to smtp server", 
            NULL, NULL,0);
     static MYSQL_SYSVAR_STR(port, port,PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Http port.",
            NULL, NULL,  "8080");
     static MYSQL_SYSVAR_BOOL(use_aes_encrypt, use_aes_encrypt,PLUGIN_VAR_OPCMDARG,
            "Shoud use aes encryption that only you or a trusted use can decode the mail content",
            NULL, NULL,0);
     static MYSQL_SYSVAR_STR(aes_key, aes_key,PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "AES Ecrypt key for non SSH/TLS messages",
            NULL, NULL,  "");
     static MYSQL_SYSVAR_STR(salt_key, salt_key,PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Salt key to anonymize meta data string ",
            NULL, NULL,  "");
     static MYSQL_SYSVAR_BOOL(use_spider, use_spider,PLUGIN_VAR_OPCMDARG,
            "Shoud use spider storage engine to access remote nodes default is spider off use federatedx",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(use_vmime, use_vmime,PLUGIN_VAR_OPCMDARG,
            "Use vmime to send email default vmime when off use curl",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(send_query, send_query,PLUGIN_VAR_OPCMDARG,
            "Send queries",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(send_schema, send_schema,PLUGIN_VAR_OPCMDARG,
            "Send schemas",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(send_replication, send_replication,PLUGIN_VAR_OPCMDARG,
            "Send replication status",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(send_variable, send_variable,PLUGIN_VAR_OPCMDARG,
            "Send variables",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(send_status, send_status,PLUGIN_VAR_OPCMDARG,
            "Send status",
            NULL, NULL,1);
      static MYSQL_SYSVAR_BOOL(send_dictionary, send_dictionary,PLUGIN_VAR_OPCMDARG,
            "Send dictionary",
            NULL, NULL,1);
     static MYSQL_SYSVAR_BOOL(http_content, http_content,PLUGIN_VAR_OPCMDARG,
            "Produce content for http interface",
            NULL, NULL,0);
     static MYSQL_SYSVAR_ULONG(history_uptime, history_uptime, PLUGIN_VAR_RQCMDARG,
            "Number of wakeup every refresh rate",
            NULL, NULL, 1, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(history_index, history_index,  PLUGIN_VAR_RQCMDARG,
            "Number of wakeup every refresh rate",
            NULL, NULL, 0, 0, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(history_length, history_length, PLUGIN_VAR_RQCMDARG,
            "Numbers of metrics to keep in history",
            NULL, NULL, 10, 1,1024, 1);
     static MYSQL_SYSVAR_ULONG(refresh_rate, refresh_rate,  PLUGIN_VAR_RQCMDARG,
            "Wait in seconds before gathering information",
            NULL, NULL, 10, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_send_query, interval_send_query, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Email queries after x refresh rate",
            NULL, NULL, 30, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_send_schema, interval_send_schema, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Send schema informations interval in refresh rate",
            NULL, NULL, 80, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_send_replication, interval_send_replication, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Send replication informations interval in refresh rate",
            NULL, NULL, 1, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_send_variable, interval_send_variable, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Send variable informations interval in refresh rate",
            NULL, NULL, 60, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_send_status, interval_send_status, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Send status informations interval in refresh rate ",
            NULL, NULL, 2, 1, 60*60*24, 1);
     static MYSQL_SYSVAR_ULONG(interval_get_query, interval_get_query, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get queries interval in seconds",
            NULL, NULL, 120, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_get_explain, interval_get_explain, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get explain informations interval in seconds",
            NULL, NULL, 300, 1, 60*60*24, 1); 
     static MYSQL_SYSVAR_ULONG(interval_get_schema, interval_get_schema, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get schema informations interval in seconds",
            NULL, NULL, 120, 1, 60*60*24, 80); 
     
     static MYSQL_SYSVAR_ULONG(interval_get_replication, interval_get_replication, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get replication informations interval in seconds",
            NULL, NULL, 5, 1, 60*60*24, 5); 
     static MYSQL_SYSVAR_ULONG(interval_get_variable, interval_get_variable, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get variable informations interval in seconds",
            NULL, NULL, 30, 1, 60*60*24, 30); 
     static MYSQL_SYSVAR_ULONG(interval_get_status, interval_get_status, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Get status informations interval in in seconds",
            NULL, NULL, 2, 1, 60*60*24, 2); 
     static MYSQL_SYSVAR_ULONG(hash_len, hash_len, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "SHA2 hash length",
            NULL, NULL, 256, 1, 60*60*24, 256); 
     static MYSQL_SYSVAR_BOOL(error_log, error_log,   PLUGIN_VAR_OPCMDARG, 
            "Trace execution to error log.", 
            NULL, NULL,0);
     static MYSQL_SYSVAR_BOOL(send_mail, send_mail,   PLUGIN_VAR_OPCMDARG, 
            "Send information by Email to support.", 
            NULL, NULL,0);
     static MYSQL_SYSVAR_STR(conn_user, conn_user, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "MariaDB external connection user",
            NULL, NULL, "root");
     static MYSQL_SYSVAR_STR(conn_password, conn_password, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "MariaDB external connection password",
            NULL, NULL, ""); 
     static MYSQL_SYSVAR_STR(conn_host, conn_host, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "MariaDB external connection host",
            NULL, NULL, "127.0.0.1"); 
     static MYSQL_SYSVAR_UINT(conn_port, conn_port, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
           "MariaDB external connection port",
            NULL, NULL, 3306, 1000, 32000, 1);
     static MYSQL_SYSVAR_STR(conn_socket, conn_socket, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "MariaDB external connection host",
            NULL, NULL, ""); 
    
     static MYSQL_SYSVAR_STR(node_name, node_name, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Human readable instance name",
            NULL, NULL, ""); 
     static MYSQL_SYSVAR_STR(node_group, node_group, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Human readable instance group",
            NULL, NULL, ""); 
     static MYSQL_SYSVAR_STR(smtp_certificat, smtp_certificat, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Public Smtp TLS certificate",
            NULL, NULL, ""); 
     static MYSQL_SYSVAR_STR(notification_email, notification_email, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Notification email",
            NULL, NULL, "stephane@skysql.com");  
     static MYSQL_SYSVAR_STR(bo_password, bo_password, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Back Office credential",
            NULL, NULL, "");  
     static MYSQL_SYSVAR_STR(bo_user, bo_user, PLUGIN_VAR_READONLY | PLUGIN_VAR_RQCMDARG,
            "Back Office credential",
            NULL, NULL, "admin");  
    
     
    static struct st_mysql_sys_var* settings[] = {
        MYSQL_SYSVAR(server_uid),
        MYSQL_SYSVAR(node_address),
        MYSQL_SYSVAR(http_address),
        MYSQL_SYSVAR(port),
        MYSQL_SYSVAR(refresh_rate),
        MYSQL_SYSVAR(error_log),
        MYSQL_SYSVAR(send_mail),
        MYSQL_SYSVAR(aes_key),
        MYSQL_SYSVAR(salt_key),
        MYSQL_SYSVAR(hash_len),
        MYSQL_SYSVAR(conn_user),
        MYSQL_SYSVAR(conn_password),
        MYSQL_SYSVAR(conn_host),
        MYSQL_SYSVAR(conn_port),
        MYSQL_SYSVAR(conn_socket),
        MYSQL_SYSVAR(history_length),
        MYSQL_SYSVAR(history_index),
        MYSQL_SYSVAR(history_uptime),
        MYSQL_SYSVAR(smtp_address),
        MYSQL_SYSVAR(smtp_user),
        MYSQL_SYSVAR(smtp_password),
        MYSQL_SYSVAR(smtp_email_from),
        MYSQL_SYSVAR(smtp_email_to),
        MYSQL_SYSVAR(smtp_authentification),
        MYSQL_SYSVAR(node_group),
        MYSQL_SYSVAR(node_name),
        MYSQL_SYSVAR(smtp_certificat),
        MYSQL_SYSVAR(notification_email),
        MYSQL_SYSVAR(bo_user),
        MYSQL_SYSVAR(bo_password),
        MYSQL_SYSVAR(use_spider),
        MYSQL_SYSVAR(use_vmime),
        MYSQL_SYSVAR(interval_send_schema),
        MYSQL_SYSVAR(interval_send_query),
        MYSQL_SYSVAR(interval_send_replication),
        MYSQL_SYSVAR(interval_send_variable),
        MYSQL_SYSVAR(interval_send_status),
        MYSQL_SYSVAR(send_schema),
        MYSQL_SYSVAR(send_query),
        MYSQL_SYSVAR(send_replication),
        MYSQL_SYSVAR(send_variable),
        MYSQL_SYSVAR(send_status),
        MYSQL_SYSVAR(send_dictionary),
        MYSQL_SYSVAR(http_content),
        MYSQL_SYSVAR(interval_get_schema),
        MYSQL_SYSVAR(interval_get_query),
        MYSQL_SYSVAR(interval_get_explain),
        MYSQL_SYSVAR(interval_get_replication),
        MYSQL_SYSVAR(interval_get_variable),
        MYSQL_SYSVAR(interval_get_status),
     
        NULL
    };


    static struct st_mysql_information_schema http_monitor ={MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION};

} // namespace http_monitor

mysql_declare_plugin(http_monitor) {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
            &http_monitor::http_monitor,
            "HTTP_MONITOR",
            "Sergei Golubchik,Stephane Varoqui",
            "MariaDB Http Monitoring",
            PLUGIN_LICENSE_GPL,
            http_monitor::init,
            http_monitor::free,
            0x0101,
            NULL,
            http_monitor::settings,
            NULL,
            0
}
mysql_declare_plugin_end;
#ifdef MARIA_PLUGIN_INTERFACE_VERSION

maria_declare_plugin(http_monitor) {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
            &http_monitor::http_monitor,
            "HTTP_MONITOR",
            "Sergei Golubchik,Stephane Varoqui",
            "MariaDB Http Monitoring",
            PLUGIN_LICENSE_GPL,
            http_monitor::init,
            http_monitor::free,
            0x0101,
            NULL,
            http_monitor::settings,
            "10.0.2",
            MariaDB_PLUGIN_MATURITY_BETA
}
maria_declare_plugin_end;
#endif
