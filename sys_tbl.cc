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

#include <stdio.h>
#include <stdlib.h>
#include <mysql_version.h>
#include <sql_parse.h>
#include <sql_class.h>
#include <sql_base.h>
#include <sql_time.h>
#include <records.h>
#include <mysql/plugin.h>
#include <mysql.h>
#include <key.h>
#include <sql_insert.h>
#include "sys_tbl.h"


#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation
#endif

mysql_mutex_t LOCK_content;
I_List<http_content_row> contents;

int checkContentExisist(http_content_row *thisRow);
void loadContent();

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
    String newString1;
    String newString2;
    String newString3;
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
          
        //sql_print_error("loop:  read_record_info");
         fromThisTable->field[0]->val_str(&newString1);
        fromThisTable->field[1]->val_str(&newString2);
        fromThisTable->field[2]->val_str(&newString3);
      //   strcpy(aRow->name,newString1.c_ptr());
      //   strcpy(aRow->content,newString2.c_ptr());
      //   strcpy(aRow->type,newString3.c_ptr());
        aRow->name.copy(newString1);
        aRow->content.copy(newString2);
        aRow->type.copy(newString3);
      
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

void loadContent(THD *thd) {
    int error;
    
    thd->client_capabilities |= CLIENT_MULTI_STATEMENTS;
   
    runQuery(thd,(char *) "set group_concat_max_len=1024*1024");
    runQuery(thd,(char *) "replace into mysql.http_contents select '/variables',  (select CONCAT('{\"variables\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',VARIABLE_NAME,'\",\"value\":\"', VARIABLE_VALUE,'\"}'    ) separator ',\\n'),'\\n]}' ) from information_schema.GLOBAL_VARIABLES WHERE VARIABLE_NAME<>'FT_BOOLEAN_SYNTAX' ) , 'text/plain' ;");
    runQuery(thd,(char *) "replace into mysql.http_contents select '/status',  (select CONCAT('{\"status\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',VARIABLE_NAME,'\",\"value\":\"', VARIABLE_VALUE,'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.GLOBAL_STATUS), 'text/plain' ;");
    runQuery(thd,(char *) "replace into mysql.http_contents select '/plugins',  (select CONCAT('{\"plugins\":[\\n' , GROUP_CONCAT(CONCAT('{\"name\":\"',PLUGIN_NAME,'\",\"value\":\"', PLUGIN_VERSION,'\",\"status\":\"',PLUGIN_STATUS,'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.ALL_PLUGINS), 'text/plain' ;");
    runQuery(thd,(char *) "replace into mysql.http_contents select '/queries',  (select CONCAT('{\"queries\":[\\n' ,"
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
            "\"LAST_SEEN\":\"',LAST_SEEN,'\"}'    ) separator ',\\n'),'\\n]}') from performance_schema.events_statements_summary_by_digest), 'text/plain' ;");
   
   runQuery(thd,(char *) "replace into mysql.http_contents select '/process',  (select CONCAT('{\"process\":[\\n' ,"
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
            "\"QUERY_ID\":\"',COALESCE(QUERY_ID,''),'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.PROCESSLIST), 'text/plain' ;");
     runQuery(thd,(char *) "replace into mysql.http_contents select '/tablestat',  (select CONCAT('{\"tablestat\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_SCHEMA\":\"',COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"ROWS_READ\":\"', COALESCE(ROWS_READ,''),'\","
            "\"ROWS_CHANGED\":\"', COALESCE(ROWS_CHANGED,''),'\","
            "\"ROWS_CHANGED_X_INDEXES\":\"',COALESCE(ROWS_CHANGED_X_INDEXES,''),'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.TABLE_STATISTICS), 'text/plain' ;");
     runQuery(thd,(char *) "replace into mysql.http_contents select '/indexstat',  (select CONCAT('{\"indexstat\":[\\n' ,"
            "GROUP_CONCAT(CONCAT('{"
            "\"TABLE_SCHEMA\":\"',COALESCE(TABLE_SCHEMA,''),'\","
            "\"TABLE_NAME\":\"', COALESCE(TABLE_NAME,''),'\","
            "\"INDEX_NAME\":\"', COALESCE(INDEX_NAME,''),'\","
            "\"ROWS_READ\":\"',COALESCE(ROWS_READ,''),'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.INDEX_STATISTICS), 'text/plain' ;");
     runQuery(thd,(char *) "replace into mysql.http_contents select '/tables',  (select CONCAT('{\"tables\":[\\n' ,"
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
            "\"DATA_FREE\":\"', COALESCE(DATA_FREE,''),'\","
            "\"AUTO_INCREMENT\":\"', COALESCE(AUTO_INCREMENT,''),'\","
            "\"CREATE_TIME\":\"', COALESCE(CREATE_TIME,''),'\","
            "\"CHECK_TIME\":\"', COALESCE(CHECK_TIME,''),'\","
            "\"TABLE_COLLATION\":\"', COALESCE(TABLE_COLLATION,''),'\","
            "\"CHECKSUM\":\"',COALESCE(CHECKSUM,''),'\"}'    ) separator ',\\n'),'\\n]}') from information_schema.TABLES), 'text/plain' ;");
   
         runQuery(thd,(char *) "select @index:=COALESCE((CAST(SUBSTRING_INDEX(max(CONCAT( COLUMN_GET(status, 'date' as char ), '_' ,LPAD(id,5,'0'))),'_', -1) as unsigned ) +1) mod 128,1) from mysql.http_status_history;");
         runQuery(thd,(char *) "replace into mysql.http_status_history select @index,column_create('date', NOW() as datetime );");
         runQuery(thd,(char *) "insert into  mysql.http_status_history select @index, null from mysql.http_last_status ps join information_schema.global_status cs on cs.VARIABLE_NAME=ps.VARIABLE_NAME left join mysql.http_status_state ss on ss.VARIABLE_NAME=ps.VARIABLE_NAME where ((cs.VARIABLE_VALUE - ps.VARIABLE_VALUE) <>0 or ss.VARIABLE_NAME IS NOT NULL)  on duplicate key update  status= column_add(status, cs.VARIABLE_NAME,  IF(ss.VARIABLE_NAME IS NOT NULL,cs.VARIABLE_VALUE,(cs.VARIABLE_VALUE - ps.VARIABLE_VALUE))   );");
         runQuery(thd,(char *) "replace into mysql.http_last_status select * from information_schema.global_status;");
         runQuery(thd,(char *) "replace into mysql.http_contents select '/comdml',  (select CONCAT('{\"comdml\":[\\n' ,"
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
            "\"COM_UPDATE\":\"',COALESCE(COLUMN_GET(status,'COM_UPDATE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_DELETE\":\"',COALESCE(COLUMN_GET(status,'COM_DELETE' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_REPLACE\":\"',COALESCE(COLUMN_GET(status,'COM_SELECT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_UPDATE_MULTI\":\"',COALESCE(COLUMN_GET(status,'COM_UPDATE_MULTI' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\","
            "\"COM_INSERT\":\"',COALESCE(COLUMN_GET(status,'COM_INSERT' as INTEGER),'1')/COLUMN_GET(status,'UPTIME' as INTEGER),'\"}'    )   separator ',\\n'),'\\n]}') from mysql.http_status_history ORDER BY COLUMN_GET(status,'date' as datetime) ), 'text/plain';");
         
      
         
         
        Open_tables_backup backup;
        TABLE *tbl = open_sysTbl(thd, "mysql","http_contents", strlen("http_contents"), &backup, false, &error);
        loadHttpContent(thd, tbl);
        close_sysTbl(thd, tbl, &backup);
    
    
}

http_content_row *getContent(const char *name) {

    http_content_row *aRow;
    I_List_iterator<http_content_row> contentsIter(contents);
    while ((aRow = contentsIter++)) {
        if (strcmp(aRow->name.ptr(), name) == 0) {
            return aRow;
        }
    }
    return NULL;
}

void freeContent() {

    http_content_row *aRow;
    I_List_iterator<http_content_row> contentsIter(contents);
    while ((aRow = contentsIter++)) {
        delete aRow;
    }
   
}


