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

#define MYSQL_SERVER 1


#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql_version.h>
#include <sql_parse.h>
//#include <sql_class.h>
//#include <sql_base.h>
//#include <sql_time.h>
#include <sql_string.h>
#include <sql_list.h>
#include <records.h>
#include <mysql/plugin.h>
#include <key.h>
#include <log.h>
//#include <sql_insert.h>
#include "sys_tbl.h"
#include "http_monitor.h"
#include <string>
#include <iostream>


namespace http_monitor {

#define DYNAMIC_STR_ERROR_MSG "Couldn't perform DYNAMIC_STRING operation"
#define EX_MYSQLERR 2
#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation
#endif

mysql_mutex_t LOCK_content;
 I_List<http_content_row> contents;

/* connection parameters */
const char *default_charset= (char*) MYSQL_AUTODETECT_CHARSET_NAME;
static int interval=0;


//int checkContentExisist(http_content_row *thisRow);
//void loadContent();

/*

TABLE *open_sysTbl(THD *thd,const char *dbName, const char *tblName,
        int tblNameLen, Open_tables_backup *tblBackup,
        my_bool enableWrite, int *error) {

    TABLE *table;
    TABLE_LIST tables;

    enum thr_lock_type writeLock;

 *error = 0;
  sql_print_error("opensysTbl: start");
    if (enableWrite == true)
        writeLock = TL_WRITE;
    else
        writeLock = TL_READ;
  sql_print_error("opensysTbl: End lock ");
    tables.init_one_table(dbName, strlen(dbName), tblName, tblNameLen, tblName, writeLock);
   sql_print_error("opensysTbl: initonetable %s",tblName);
  // if (strcmp(dbName, "information_schema")==0)  {
   if (!(table = open_log_table(thd, &tables, tblBackup))) {
 *error = my_errno;
        return NULL;
    }
      sql_print_error("opensysTbl: open_log_table %s",tblName);
    table->in_use = current_thd;
   // }
sql_print_error("opensysTbl: in_use %s",tblName);
    return table;
}

void close_sysTbl(THD *thd, TABLE *table, Open_tables_backup *tblBackup) {
    if (!thd->in_multi_stmt_transaction_mode())
        thd->mdl_context.release_transactional_locks();
    else
        thd->mdl_context.release_statement_locks();

    close_log_table(thd, tblBackup);
}

void loadHttpContent(THD *thd, TABLE *fromThisTable) {
    int error;
    mysql_mutex_lock(&LOCK_content);

    READ_RECORD read_record_info;
    init_read_record(&read_record_info, thd, fromThisTable, NULL, 1, 0, FALSE);

    fromThisTable->use_all_columns();
    //sql_print_error("loadHttpContent: use_all_columns");
    freeContent();
    contents.empty();
    //sql_print_error("loadHttpContent: empty");
    while (!(error = read_record_info.read_record(&read_record_info))) {
        http_content_row *aRow = new http_content_row;
   String newString1;
   String newString2;
   String newString3;
          
        //sql_print_error("loop:  read_record_info");
        fromThisTable->field[0]->val_str(&newString1);
        fromThisTable->field[1]->val_str(&newString2);
        fromThisTable->field[2]->val_str(&newString3);
        //fromThisTable->field[0]->val_str(&aRow->name);
        //fromThisTable->field[1]->val_str(&aRow->content);
        //fromThisTable->field[2]->val_str(&aRow->type);
       //   strcpy(aRow->name,newString1.c_ptr());
      //   strcpy(aRow->content,newString2.c_ptr());
      //   strcpy(aRow->type,newString3.c_ptr());
        aRow->name.append(newString1);
        aRow->content.append(newString2);
      aRow->type.append(newString3);
      
        contents.push_back(aRow);
        
    }
     
    end_read_record(&read_record_info);

#ifdef __QQUEUE_DEBUG__
    fprintf(stderr, "loadQqueueQueueRow: content\n");

    http_content_row *aRow;
    I_List_iterator<http_content_row> contentsIter(contents);
    while (aRow = contentsIter++) {
        fprintf(stderr, "id: %i name: %s priority: %i timeout: %i\n", aRow->id, aRow->name,
                aRow->priority, aRow->timeout);
    }

    fprintf(stderr, "loadQqueueQueueRow: end\n");
#endif

    mysql_mutex_unlock(&LOCK_content);
}

 */
int loadHttpContentConn() {
    MYSQL_ROW row;
    MYSQL conn;
    MYSQL_RES *result;
    int status;

    freeContent();

    mysql_init(&conn);
    conn.options.max_allowed_packet=32000000 ;
  //  conn.options.max_packet_size
    if (!mysql_real_connect(&conn,
            http_monitor::conn_host,
            http_monitor::conn_user,
            http_monitor::conn_password,
            "mysql",
            http_monitor::conn_port,
            http_monitor::conn_socket,
            CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS )) {
            sql_print_error( "Can't connect to server: %s\n",
                mysql_error(&conn));
        return false;
    }

    updateContent(&conn);
    status = mysql_query(&conn, "SELECT * FROM http_contents");
    if (status) {
        sql_print_error("Could not execute statement(s)");
        mysql_close(&conn);
        return 3;
    }

    result = mysql_store_result(&conn);

    int ct = 0;
    if (result) {

        while ((row = mysql_fetch_row(result))) {
            ct++;
            ulong *lengths = mysql_fetch_lengths(result);

            http_content_row *aRow = new http_content_row;
            aRow->name.assign( row[0],lengths[0]);
            aRow->content.assign(row[1],lengths[1]);
            aRow->type.assign(row[2],lengths[2]);
             if (error_log)
            sql_print_information("Http Monitor * loadHttpContentConn * : Loading %s length %lu",aRow->name.c_str(),lengths[1] );
            contents.push_back(aRow);
            //aRow->content.length += mysql_real_escape_string(&conn,&aRow->content.str[ aRow->content.length],row[1],length);
            //

        }
        if (error_log)
        sql_print_information("Http Monitor * loadHttpContentConn * : Number of element loaded:%d ", ct);
        mysql_free_result(result);

    }
    mysql_close(&conn);
    //sql_print_error("loadHttpContentConn 6");
    return 0;

}



int runQuery(THD *thd,  char *query ) {
    
    thd->set_query(query, strlen(query));
    MYSQL_QUERY_START(query, thd->thread_id,
                      (char *) (thd->db ? thd->db : ""),
                      &thd->security_ctx->priv_user[0],
                      (char *) thd->security_ctx->host_or_ip);
    Parser_state parser_state;
    
    if ( parser_state.init(thd, thd->query(), thd->query_length())) {
       sql_print_error("error: executing query");
    }
    thd->init_for_queries();
    mysql_parse(thd, thd->query(), strlen(thd->query()), &parser_state);
    return 0;  
}


int runQueryConn(MYSQL* conn,  String *query ) {
    if(conn == (MYSQL*) NULL) return 1;
    int status =0; 
    if (error_log)
      sql_print_information("Http Monitor *runQueryConn*: %s",query->c_ptr() );
    status = mysql_query(conn,query->c_ptr());
    return status;  
}

String getStatusHistoryPosition(MYSQL* conn) {
    MYSQL_ROW row;
    MYSQL_RES *result=NULL;
    String query=0;
 
    query.append((char*) "SELECT COALESCE((CAST(SUBSTRING_INDEX(max(CONCAT( COLUMN_GET(status, 'date' as char ), '_' ,LPAD(id,5,'0'))),'_', -1) as unsigned ) +1) MOD ");
    query.append_ulonglong(http_monitor::history_length);
    query.append( (char*) ",1) AS pos FROM mysql.http_status_history;");
    String pos;
    pos.append("1");
    if(!runQueryConn(conn,&query))
    {
       
    result = mysql_store_result(conn);
    if (result ) {
    
    if((row = mysql_fetch_row(result))){
         ulong *lengths = mysql_fetch_lengths(result);
        
         pos.free();
         pos.append(row[0],lengths[0]);
       
        if (error_log)
         sql_print_information ("Http monitor *getStatusHistoryPosition* : processing queue position %s", pos.c_ptr());
     
    }
    mysql_free_result(result); 
    }
    }   
  
    return  pos;
}



void finish_with_error(MYSQL *conn, String *query)
{
  sql_print_error("Error: running %s,%s", query->c_ptr(), mysql_error(conn));
  mysql_close(conn);
          
}

int updateContent(MYSQL* conn) {
    
  //  http_monitor::Server **todo= (http_monitor::Server**)alloca(http_monitor::servers_count*sizeof(http_monitor::Server*));
  //  memcpy(todo, http_monitor::servers, http_monitor::servers_count*sizeof(http_monitor::Server*));
   
     
    I_List<http_query> http_queries;
   
    String pos;
    pos.append(getStatusHistoryPosition(conn).c_ptr()); 
    http_monitor::history_index = atol(pos.c_ptr()) ;
    
    
    
    http_query *aRow; 
    aRow = new http_query;
    aRow->query.append((char *) "SET sql_log_bin=0");
    http_queries.push_back(aRow);
    
    
    aRow = new http_query;
    aRow->query.append((char *) "SET group_concat_max_len=32*1024*1024");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *)  "REPLACE INTO mysql.http_contents SELECT '/variables',  (SELECT CONCAT('{\"variables\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',VARIABLE_NAME,'\",\"value\":\"', VARIABLE_VALUE,'\"}'    ) separator ',\\n'),'\\n]}' ) FROM information_schema.GLOBAL_VARIABLES WHERE VARIABLE_NAME<>'FT_BOOLEAN_SYNTAX' ) , 'text/plain' ;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/status',  (SELECT CONCAT('{\"status\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',VARIABLE_NAME,'\",\"value\":\"', VARIABLE_VALUE,'\"}'    ) separator ',\\n'),'\\n]}') FROM information_schema.GLOBAL_STATUS), 'text/plain' ;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/plugins',  (SELECT CONCAT('{\"plugins\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',PLUGIN_NAME,'\",\"value\":\"', PLUGIN_VERSION,'\",\"status\":\"',PLUGIN_STATUS,'\"}'    ) separator ',\\n'),'\\n]}') FROM information_schema.ALL_PLUGINS), 'text/plain' ;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/queries',  (SELECT CONCAT('{\"queries\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"SCHEMA_NAME\":\"',SCHEMA_NAME,'\","
            "\"DIGEST\":\"', DIGEST,'\","
            "\"DIGEST_TEXT\":\"', DIGEST_TEXT,'\","
            "\"COUNT_STAR\":\"', COUNT_STAR,'\","
            "\"SUM_TIMER_WAIT\":\"', SUM_TIMER_WAIT,'\","
            "\"MIN_TIMER_WAIT\":\"', MIN_TIMER_WAIT,'\","
            "\"AVG_TIMER_WAIT\":\"', AVG_TIMER_WAIT,'\","
            "\"MAX_TIMER_WAIT\":\"', MAX_TIMER_WAIT,'\","
            "\"SUM_LOCK_TIME\":\"', SUM_LOCK_TIME,'\","
            "\"SUM_ERRORS\":\"', SUM_ERRORS,'\","
            "\"SUM_WARNINGS\":\"', SUM_WARNINGS,'\","
            "\"SUM_ROWS_AFFECTED\":\"', SUM_ROWS_AFFECTED,'\","
            "\"SUM_ROWS_SENT\":\"', SUM_ROWS_SENT,'\","
            "\"SUM_ROWS_EXAMINED\":\"', SUM_ROWS_EXAMINED,'\","
            "\"SUM_CREATED_TMP_DISK_TABLES\":\"', SUM_CREATED_TMP_DISK_TABLES,'\","
            "\"SUM_CREATED_TMP_TABLES\":\"', SUM_CREATED_TMP_TABLES,'\","
            "\"SUM_SELECT_FULL_JOIN\":\"', SUM_SELECT_FULL_JOIN,'\","
            "\"SUM_SELECT_FULL_RANGE_JOIN\":\"', SUM_SELECT_FULL_RANGE_JOIN,'\","
            "\"SUM_SELECT_RANGE\":\"', SUM_SELECT_RANGE,'\","
            "\"SUM_SELECT_RANGE_CHECK\":\"', SUM_SELECT_RANGE_CHECK,'\","
            "\"SUM_SELECT_SCAN\":\"', SUM_SELECT_SCAN,'\","
            "\"SUM_SORT_MERGE_PASSES\":\"', SUM_SORT_MERGE_PASSES,'\","
            "\"SUM_SORT_RANGE\":\"', SUM_SORT_RANGE,'\","
            "\"SUM_SORT_ROWS\":\"', SUM_SORT_ROWS,'\","
            "\"SUM_SORT_SCAN\":\"', SUM_SORT_SCAN,'\","
            "\"SUM_NO_INDEX_USED\":\"', SUM_NO_INDEX_USED,'\","
            "\"SUM_NO_GOOD_INDEX_USED\":\"', SUM_NO_GOOD_INDEX_USED,'\","
            "\"FIRST_SEEN\":\"', FIRST_SEEN,'\","
            "\"LAST_SEEN\":\"',LAST_SEEN,'\"}'    ) SEPARATOR ',\\n'),'\\n]}') FROM performance_schema.events_statements_summary_by_digest), 'text/plain' ;");
    http_queries.push_back(aRow);
    
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/process',  (SELECT CONCAT('{\"process\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"ID\":\"',COALESCE(ID,''),'\","
            "\"USER\":\"', COALESCE(USER,''),'\","
            "\"HOST\":\"', COALESCE(HOST,''),'\","
            "\"DB\":\"', COALESCE(DB,''),'\","
            "\"COMMAND\":\"', COALESCE(COMMAND,''),'\","
            "\"TIME\":\"', COALESCE(TIME,''),'\","
            "\"STATE\":\"', COALESCE(STATE,''),'\","
            "\"INFO\":\"', REPLACE(COALESCE(INFO,''),'\"','\\\\\\\"'),'\","
            "\"TIME_MS\":\"', COALESCE(TIME_MS,''),'\","
            "\"STAGE\":\"', COALESCE(STAGE,''),'\","
            "\"MAX_STAGE\":\"', COALESCE(MAX_STAGE,''),'\","
            "\"PROGRESS\":\"', COALESCE(PROGRESS,''),'\","
            "\"MEMORY_USED\":\"', COALESCE(MEMORY_USED,''),'\","
            "\"EXAMINED_ROWS\":\"', COALESCE(EXAMINED_ROWS,''),'\","
            "\"QUERY_ID\":\"',COALESCE(QUERY_ID,''),'\"}'    ) SEPARATOR ',\\n'),'\\n]}') FROM information_schema.PROCESSLIST), 'text/plain' ;");
    http_queries.push_back(aRow);
    
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/tablestat',  (SELECT CONCAT('{\"tablestat\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_SCHEMA\":\"',COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"ROWS_READ\":\"', COALESCE(ROWS_READ,''),'\","
            "\"ROWS_CHANGED\":\"', COALESCE(ROWS_CHANGED,''),'\","
            "\"ROWS_CHANGED_X_INDEXES\":\"',COALESCE(ROWS_CHANGED_X_INDEXES,''),'\"}'    ) separator ',\\n'),'\\n]}') FROM information_schema.TABLE_STATISTICS), 'text/plain' ;");
    http_queries.push_back(aRow);
   
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/indexstat',  (SELECT CONCAT('{\"indexstat\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_SCHEMA\":\"',COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"INDEX_NAME\":\"', COALESCE(INDEX_NAME,''),'\","
            "\"ROWS_READ\":\"',COALESCE(ROWS_READ,''),'\"}'    ) SEPARATOR ',\\n'),'\\n]}') FROM information_schema.INDEX_STATISTICS), 'text/plain' ;"  );
    http_queries.push_back(aRow);
   
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/tables',  (SELECT CONCAT('{\"tables\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_CATALOG\":\"',COALESCE(TABLE_CATALOG,''),'\","
            "\"TABLE_SCHEMA\":\"', COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"TABLE_TYPE\":\"', COALESCE(TABLE_TYPE,''),'\","
            "\"ENGINE\":\"', COALESCE(ENGINE,''),'\","
            "\"VERSION\":\"', COALESCE(VERSION,''),'\","
            "\"ROW_FORMAT\":\"', COALESCE(ROW_FORMAT,''),'\","
            "\"TABLE_ROWS\":\"', COALESCE(TABLE_ROWS,''),'\","
            "\"AVG_ROW_LENGTH\":\"', COALESCE(AVG_ROW_LENGTH,''),'\","
            "\"DATA_LENGTH\":\"', COALESCE(DATA_LENGTH,''),'\","
            "\"INDEX_LENGTH\":\"', COALESCE(INDEX_LENGTH,''),'\","
            "\"DATA_FREE\":\"', COALESCE(DATA_FREE,''),'\","
            "\"TOTAL_SIZE\":\"', COALESCE(DATA_FREE,0)+COALESCE(INDEX_LENGTH,0)+ COALESCE(DATA_LENGTH,0),'\","
            "\"DATA_PCT\":\"',CAST(COALESCE(DATA_LENGTH,'') / (SELECT SUM(DATA_FREE + INDEX_LENGTH + DATA_LENGTH)  AS TOT FROM information_schema.TABLES LIMIT 1) *100 AS DECIMAL(10,2)),'\","
            "\"INDEX_PCT\":\"',CAST(COALESCE(INDEX_LENGTH,'') / (SELECT SUM(DATA_FREE + INDEX_LENGTH + DATA_LENGTH) AS TOT FROM information_schema.TABLES LIMIT 1) *100 AS DECIMAL(10,2)),'\","
            "\"AUTO_INCREMENT\":\"', COALESCE(AUTO_INCREMENT,''),'\","
            "\"CREATE_TIME\":\"', COALESCE(CREATE_TIME,''),'\","
            "\"CHECK_TIME\":\"', COALESCE(CHECK_TIME,''),'\","
            "\"TABLE_COLLATION\":\"', COALESCE(TABLE_COLLATION,''),'\","
            "\"CHECKSUM\":\"',COALESCE(CHECKSUM,''),'\"}'    )  ORDER BY DATA_FREE + INDEX_LENGTH+ DATA_LENGTH DESC SEPARATOR ',\\n'),'\\n]}')  FROM information_schema.TABLES  ), 'text/plain' ;");
    http_queries.push_back(aRow);
   
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/toptables',  (SELECT CONCAT('{\"tables\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_CATALOG\":\"',COALESCE(TABLE_CATALOG,''),'\","
            "\"TABLE_SCHEMA\":\"', COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"TABLE_ROWS\":\"', COALESCE(TABLE_ROWS,''),'\","
            "\"AVG_ROW_LENGTH\":\"', COALESCE(AVG_ROW_LENGTH,''),'\","
            "\"DATA_LENGTH\":\"', COALESCE(DATA_LENGTH,''),'\","
            "\"INDEX_LENGTH\":\"', COALESCE(INDEX_LENGTH,''),'\","
            "\"TOTAL_SIZE\":\"', COALESCE(DATA_FREE,0)+COALESCE(INDEX_LENGTH,0)+ COALESCE(DATA_LENGTH,0),'\","
            "\"DATA_PCT\":\"',CAST(COALESCE(DATA_LENGTH,'') / (SELECT SUM(DATA_FREE + INDEX_LENGTH + DATA_LENGTH)  AS TOT FROM information_schema.TABLES LIMIT 1) *100 AS DECIMAL(10,2)),'\","
            "\"INDEX_PCT\":\"',CAST(COALESCE(INDEX_LENGTH,'') / (SELECT SUM(DATA_FREE + INDEX_LENGTH + DATA_LENGTH) AS TOT FROM information_schema.TABLES LIMIT 1) *100 AS DECIMAL(10,2)),'\","
            "\"CHECKSUM\":\"',COALESCE(CHECKSUM,''),'\"}'    )  ORDER BY DATA_FREE + INDEX_LENGTH+ DATA_LENGTH DESC  SEPARATOR ',\\n'),'\\n]}')  FROM (SELECT * FROM  information_schema.TABLES ORDER BY DATA_FREE + INDEX_LENGTH+ DATA_LENGTH DESC LIMIT 10) AS TMP  ), 'text/plain' ;");
    http_queries.push_back(aRow);
   
    
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT * FROM information_schema.global_status;" );
    http_queries.push_back(aRow);
   
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_COUNT_STAR_', DIGEST) as VARIABLE_NAME , COUNT_STAR AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_ROWS_SENT_', DIGEST) as VARIABLE_NAME , SUM_ROWS_SENT AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
   
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_ROWS_EXAMINED_', DIGEST) as VARIABLE_NAME , SUM_ROWS_EXAMINED AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_LOCK_TIME_', DIGEST) as VARIABLE_NAME , SUM_LOCK_TIME AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_WARNINGS_', DIGEST) as VARIABLE_NAME , SUM_WARNINGS AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_ROWS_AFFECTED_', DIGEST) as VARIABLE_NAME , SUM_ROWS_AFFECTED AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_CREATED_TMP_DISK_TABLES_', DIGEST) as VARIABLE_NAME , SUM_CREATED_TMP_DISK_TABLES AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_CREATED_TMP_TABLES_', DIGEST) as VARIABLE_NAME , SUM_CREATED_TMP_TABLES AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_FULL_JOIN_', DIGEST) as VARIABLE_NAME , SUM_SELECT_FULL_JOIN AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_FULL_RANGE_JOIN_', DIGEST) as VARIABLE_NAME , SUM_SELECT_FULL_RANGE_JOIN AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_RANGE_', DIGEST) as VARIABLE_NAME , SUM_SELECT_RANGE AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_RANGE_CHECK_', DIGEST) as VARIABLE_NAME , SUM_SELECT_RANGE_CHECK AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_FULL_RANGE_JOIN_', DIGEST) as VARIABLE_NAME , SUM_SELECT_FULL_RANGE_JOIN AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SELECT_SCAN_', DIGEST) as VARIABLE_NAME , SUM_SELECT_SCAN AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SORT_MERGE_PASSES_', DIGEST) as VARIABLE_NAME , SUM_SORT_MERGE_PASSES AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SORT_RANGE_', DIGEST) as VARIABLE_NAME , SUM_SORT_RANGE AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_SORT_SCAN_', DIGEST) as VARIABLE_NAME , SUM_SORT_SCAN AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_NO_INDEX_USED_', DIGEST) as VARIABLE_NAME , SUM_NO_INDEX_USED AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_current_status SELECT CONCAT('QUERY_SUM_NO_GOOD_INDEX_USED_', DIGEST) as VARIABLE_NAME , SUM_NO_GOOD_INDEX_USED AS VARIABLE_VALUE FROM performance_schema.events_statements_summary_by_digest;");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_status_history SELECT " );
    aRow->query.append( pos);
    aRow->query.append(",COLUMN_CREATE('date', NOW() AS datetime );");
    http_queries.push_back(aRow);
    
    
    aRow = new http_query;
    aRow->query.append((char *) "INSERT INTO  mysql.http_status_history SELECT " );
    aRow->query.append( pos);
    aRow->query.append((char *) ",NULL FROM mysql.http_last_status ps JOIN mysql.http_current_status cs ON cs.VARIABLE_NAME=ps.VARIABLE_NAME LEFT JOIN mysql.http_status_state ss ON ss.VARIABLE_NAME=ps.VARIABLE_NAME WHERE ((cs.VARIABLE_VALUE - ps.VARIABLE_VALUE) <>0 OR ss.VARIABLE_NAME IS NOT NULL)  ON DUPLICATE KEY UPDATE status=COLUMN_ADD(status, cs.VARIABLE_NAME,  IF(ss.VARIABLE_NAME IS NOT NULL,cs.VARIABLE_VALUE,(cs.VARIABLE_VALUE - ps.VARIABLE_VALUE)));");
    http_queries.push_back(aRow);
    
    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_last_status SELECT * FROM mysql.http_current_status;");
    http_queries.push_back(aRow);
   
    aRow = new http_query;
    aRow->query.append((char *)  "REPLACE INTO mysql.http_contents SELECT '/comdml',"
                "  (SELECT CONCAT('{\"server\":\"");
    aRow->query.append( http_monitor::server_uid_buf);
       aRow->query.append((char *)  "\",\"comdml\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"DATE\":\"',UNIX_TIMESTAMP(COLUMN_GET(status,'date' as datetime))*1000,'\","   
            "\"HANDLER_COMMIT\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_COMMIT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_DELETE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_DELETE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_DISCOVER\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_DISCOVER' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_EXTERNAL_LOCK\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_EXTERNAL_LOCK' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_ICP_ATTEMPTS\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_ICP_ATTEMPTS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_ICP_MATCH\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_ICP_MATCH' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_MRR_INIT\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_MRR_INIT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_MRR_KEY_REFILLS\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_MRR_KEY_REFILLS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_MRR_ROWID_REFILLS\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_MRR_ROWID_REFILLS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_PREPARE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_PREPARE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_KEY\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_KEY' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_NEXT\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_NEXT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_PREV\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_PREV' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_RND\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_RND' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_RND_DELETED\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_RND_DELETED' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_READ_RND_NEXT\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_READ_RND_NEXT' as INTEGER)/COLUMN_GET(status,'UPTIME' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_ROLLBACK\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_ROLLBACK' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_SAVEPOINT\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_SAVEPOINT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_SAVEPOINT_ROLLBACK\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_SAVEPOINT_ROLLBACK' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_TMP_UPDATE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_TMP_UPDATE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_TMP_WRITE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_TMP_WRITE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_UPDATE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_UPDATE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"HANDLER_WRITE\":\"',ABS(COALESCE(COLUMN_GET(status,'HANDLER_WRITE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"QCACHE_HITS\":\"',ABS(COALESCE(COLUMN_GET(status,'QCACHE_HITS' as INTEGER)/COLUMN_GET(status,'UPTIME' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"QCACHE_INSERTS\":\"',ABS(COALESCE(COLUMN_GET(status,'QCACHE_INSERTS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"QCACHE_LOWMEM_PRUNES\":\"',ABS(COALESCE(COLUMN_GET(status,'QCACHE_LOWMEM_PRUNES' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"QCACHE_NOT_CACHED\":\"',ABS(COALESCE(COLUMN_GET(status,'QCACHE_NOT_CACHED' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"SUBQUERY_CACHE_HIT\":\"',ABS(COALESCE(COLUMN_GET(status,'SUBQUERY_CACHE_HIT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"SUBQUERY_CACHE_MISS\":\"',ABS(COALESCE(COLUMN_GET(status,'SUBQUERY_CACHE_MISS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER)),'\","
            "\"MYISAM\":\"', COALESCE(COLUMN_GET(status,'KEY_READS' as INTEGER)/COLUMN_GET(status,'KEY_READS_REQUEST' as INTEGER)*100,100),'\","
            "\"QCACHE\":\"', COALESCE(COLUMN_GET(status,'SUBQUERY_CACHE_HIT' as INTEGER)/(COLUMN_GET(status,'KEY_READS_REQUEST' as INTEGER)+COLUMN_GET(status,'COM_SELECT' as INTEGER))*100,100),'\","
            "\"SUBQUERY\":\"', COALESCE(COLUMN_GET(status,'SUBQUERY_CACHE_MISS' as INTEGER)/COLUMN_GET(status,'SUBQUERY_CACHE_HIT' as INTEGER)*100,100),'\","
            "\"INNODB\":\"', COALESCE(COLUMN_GET(status,'INNODB_BUFFER_POOL_READS' as INTEGER)/COLUMN_GET(status,'INNODB_BUFFER_POOL_READ_REQUESTS' as INTEGER)*100,100),'\","
            "\"ARIA\":\"', COALESCE(COLUMN_GET(status,'ARIA_PAGECACHE_READS' as INTEGER)/COLUMN_GET(status,'ARIA_PAGECACHE_READ_REQUESTS' as INTEGER)*100,100),'\","
            "\"OPENED_TABLES\":\"',COALESCE(COLUMN_GET(status,'OPENED_TABLES' as INTEGER),0),'\","
            "\"OPEN_TABLES\":\"',COALESCE(COLUMN_GET(status,'OPEN_TABLES' as INTEGER),0),'\","
            "\"OPENED_TABLE_DEFINITIONS\":\"',COALESCE(COLUMN_GET(status,'OPENED_TABLE_DEFINITIONS' as INTEGER),0),'\","
            "\"OPEN_TABLE_DEFINITIONS\":\"',COALESCE(COLUMN_GET(status,'OPEN_TABLE_DEFINITIONS' as INTEGER),0),'\","
            "\"OPENED_FILES\":\"',COALESCE(COLUMN_GET(status,'OPENED_FILES' as INTEGER),0),'\","
            "\"OPEN_FILES\":\"',COALESCE(COLUMN_GET(status,'OPEN_FILES' as INTEGER),0),'\","
            "\"BINLOG_CACHE_DISK_USE\":\"',COALESCE(COLUMN_GET(status,'BINLOG_CACHE_DISK_USE' as INTEGER),0),'\","
            "\"BINLOG_CACHE_USE\":\"',COALESCE(COLUMN_GET(status,'BINLOG_CACHE_USE' as INTEGER),0),'\","
            "\"BINLOG_STMT_CACHE_DISK_USE\":\"',COALESCE(COLUMN_GET(status,'BINLOG_STMT_CACHE_DISK_USE' as INTEGER),0),'\","
            "\"BINLOG_STMT_CACHE_USE\":\"',COALESCE(COLUMN_GET(status,'BINLOG_STMT_CACHE_USE' as INTEGER),0),'\","
            "\"THREADS_RUNNING\":\"',COALESCE(COLUMN_GET(status,'THREADS_RUNNING' as INTEGER),0),'\","
            "\"THREADS_CACHED\":\"',COALESCE(COLUMN_GET(status,'THREADS_CACHED' as INTEGER),0),'\","
            "\"THREADS_CREATED\":\"',COALESCE(COLUMN_GET(status,'THREADS_CREATED' as INTEGER),0),'\","
            "\"THREADS_CONNECTED\":\"',COALESCE(COLUMN_GET(status,'THREADS_CONNECTED' as INTEGER),0),'\","     
            "\"COM_SELECT\":\"',COALESCE(COLUMN_GET(status,'QUESTIONS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"QUESTIONS\":\"',COALESCE(COLUMN_GET(status,'QUESTIONS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"QUERIES\":\"',COALESCE(COLUMN_GET(status,'QUERIES' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"INNODB_PCT_DIRTY\":\"', COALESCE(COLUMN_GET(status,'INNODB_BUFFER_POOL_PAGES_DIRTY' as INTEGER)/COLUMN_GET(status,'INNODB_BUFFER_POOL_PAGES_TOTAL' as INTEGER)*100,100),'\","
            "\"INNODB_PCT_DATA\":\"', COALESCE(COLUMN_GET(status,'INNODB_BUFFER_POOL_PAGES_DATA' as INTEGER)/COLUMN_GET(status,'INNODB_BUFFER_POOL_PAGES_TOTAL' as INTEGER)*100,100),'\","
            "\"INNODB_CHECPOINT_AGE\":\"', COALESCE(COLUMN_GET(status,'INNODB_LSN_CURRENT' as INTEGER) - COLUMN_GET(status,'INNODB_LSN_LAST_CHECKPOINT' as INTEGER)*100,100),'\","
           
            "\"INNODB_ROW_LOCK_WAITS\":\"',COALESCE(COLUMN_GET(status,'INNODB_ROW_LOCK_WAITS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"INNODB_LOG_WAITS\":\"',COALESCE(COLUMN_GET(status,'INNODB_LOG_WAITS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"INNODB_DEADLOCKS\":\"',COALESCE(COLUMN_GET(status,'INNODB_DEADLOCKS' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"INNODB_IBUF_SIZE\":\"',COALESCE(COLUMN_GET(status,'INNODB_IBUF_SIZE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"INNODB_IBUF_MERGES\":\"',COALESCE(COLUMN_GET(status,'INNODB_IBUF_MERGES' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
       
            "\"COM_UPDATE\":\"',COALESCE(COLUMN_GET(status,'COM_UPDATE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_DELETE\":\"',COALESCE(COLUMN_GET(status,'COM_DELETE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_REPLACE\":\"',COALESCE(COLUMN_GET(status,'COM_SELECT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_UPDATE_MULTI\":\"',COALESCE(COLUMN_GET(status,'COM_UPDATE_MULTI' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_INSERT\":\"',COALESCE(COLUMN_GET(status,'COM_INSERT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\"}'    )   separator ',\\n'),'\\n]}')" 
            "FROM mysql.http_status_history ORDER BY COLUMN_GET(status,'date' as datetime) ), 'text/plain';");
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/history',"
              " (SELECT CONCAT('{\"server\":\"");  
    aRow->query.append( http_monitor::server_uid_buf);
    aRow->query.append((char *)"\",\"instance_name\":\"");
    aRow->query.append( http_monitor::node_name);
    aRow->query.append((char *)"\",\"instance_group\":\"");
    aRow->query.append( http_monitor::node_group);
    
    aRow->query.append((char *)"\",\"history\":\"'"
     ", HEX(COMPRESS(GROUP_CONCAT(COLUMN_JSON(status)  separator ',\\n'))) ,'\"}')  FROM mysql.http_status_history ORDER BY COLUMN_GET(status,'date' as datetime) ),  'text/plain'"); 
    http_queries.push_back(aRow);

    aRow = new http_query;
    aRow->query.append((char *) "REPLACE INTO mysql.http_contents SELECT '/template',  (SELECT CONCAT('{\"server\":\"");
    aRow->query.append( http_monitor::server_uid_buf);
    aRow->query.append((char *)"\",\"bo_user\":\"");
    aRow->query.append( http_monitor::bo_user);
    aRow->query.append((char *)"\",\"bo_password\":\"");
    aRow->query.append( http_monitor::bo_password);
    aRow->query.append((char *)"\",\"notification_email\":\"");
    aRow->query.append( http_monitor::notification_email);
    aRow->query.append((char *) "\",\"queries\":[\\n' ,"
            " HEX(COMPRESS(GROUP_CONCAT(CONCAT('{"
            "\"SCHEMA_NAME\":\"',SCHEMA_NAME,'\","
            "\"DIGEST\":\"', DIGEST,'\","
            "\"DIGEST_TEXT\":\"', DIGEST_TEXT,'\","
            "\"FIRST_SEEN\":\"', FIRST_SEEN,'\","
            "\"LAST_SEEN\":\"',LAST_SEEN,'\"}'    ) SEPARATOR ',\\n'))),'\\n]}') FROM performance_schema.events_statements_summary_by_digest), 'text/plain' ;");
    http_queries.push_back(aRow);
    
    http_query *query;
    I_List_iterator<http_query> contentsIter(http_queries);
    while ((query = contentsIter++)) {
         if(runQueryConn( conn ,&query->query)){
           finish_with_error(conn, &query->query);  
           return 0;
       } 
    } 
     
    return 0;
}

void loadContent(THD *thd) {
    int error;
    
        loadHttpContentConn();
        /*
        Open_tables_backup backup;
        TABLE *tbl = open_sysTbl(thd, "mysql","http_contents", strlen("http_contents"), &backup, false, &error);
        loadHttpContent(thd, tbl);
        close_sysTbl(thd, tbl, &backup);
    */
    
}

http_content_row *getContent(String *name) {

    http_content_row *aRow;
    I_List_iterator<http_content_row> contentsIter(contents);
    while ((aRow = contentsIter++)) {
        if (aRow->name.compare(name->c_ptr()) == 0) {
            return aRow;
        }
    }
    return NULL;
}

void freeContent() {

    http_content_row *aRow, *aPrevious;
    aPrevious=0;
    I_List_iterator<http_content_row> contentsIter(contents);
    int ct=0;
    while ((aRow = contentsIter++)) {
        ct++;
       if(aPrevious) delete aPrevious;
       aPrevious=aRow;            
    }
    if(aPrevious) delete aPrevious;
    if (error_log)
    sql_print_information("Http Monitor *freeContent*: number of deleted %d ",ct);
    contents.empty();
   
}


/*

http_content_row *getContent(const char *name) {
    MYSQL_ROW row;
    MYSQL * conn = NULL;
    MYSQL_RES *result;
    int status;
    sql_print_error("loadHttpContentConn 1");
    http_content_row *aRow = new http_content_row;
    //freeContent();
    //contents.empty();
    host= (char*) "127.0.0.1";
    user= (char*) "root";
    opt_password= (char*)  "";
    sql_print_error("loadHttpContentConn 2");
      conn = mysql_init(NULL);
        sql_print_error("loadHttpContentConn 3");
  if (!mysql_real_connect(conn,
                          host,
                          user,
                          opt_password,
                          "mysql",
                          3306,
                          NULL,
                          CLIENT_MULTI_STATEMENTS))
  {
      sql_print_error("loadHttpContentConn 3 ");
        sql_print_error("loadHttpContentConn %s\n ",mysql_error(conn));
    return NULL;
  }
    
 
    sql_print_error("loadHttpContentConn 1");
    String query;
    query.append("SELECT * FROM http_contents where name='");
    query.append(name);
    query.append("'");
            
    status = mysql_query(conn, query.c_ptr());
     sql_print_error("loadHttpContentConn 2");
     
    if (status) {
      sql_print_error("Could not execute statement(s)");
        mysql_close(conn);
        return NULL;
    }
 sql_print_error("loadHttpContentConn 3");
     
    result = mysql_store_result(conn);
    if (result) {

        while ((row = mysql_fetch_row(result))) {
            sql_print_error("loadHttpContentConn 4");
            
            strcpy(aRow->name,row[0]);
            //strcpy(aRow->content,row[1]);
            strcpy(aRow->type,row[2]);
            
        // 
        //    aRow->name.append(row[0]);
        //    aRow->content.append(row[1]);
         //   aRow->type.append(row[2]);
        }
        
         sql_print_error("loadHttpContentConn 5");
        mysql_free_result(result);
       
 
    } else {
       
            return NULL;
       
    }

    
    mysql_close(conn);
sql_print_error("loadHttpContentConn 6");
     return aRow;

}

*/
}