/* Copyright (c) 2012, 2013, Adrian M. Partl, eScience Group at the
   Leibniz Institut for Astrophysics, Potsdam

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

/*****************************************************************
 ********                    sys_tbl                       *******
 *****************************************************************
 *
 * common functions for handling system tables
 *
 *****************************************************************
 */

#define MYSQL_SERVER 1

#include <stdio.h>
#include <stdlib.h>
#include <mysql_version.h>
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

TABLE *open_sysTbl(THD *thd, const char *tblName,
        int tblNameLen, Open_tables_backup *tblBackup,
        my_bool enableWrite, int *error) {

    TABLE *table;
    TABLE_LIST tables;

    enum thr_lock_type writeLock;

    *error = 0;

    if (enableWrite == true)
        writeLock = TL_WRITE;
    else
        writeLock = TL_READ;

    tables.init_one_table("mysql", 5, tblName, tblNameLen, tblName, writeLock);

    if (!(table = open_log_table(thd, &tables, tblBackup))) {
        *error = my_errno;
        return NULL;
    }

    table->in_use = current_thd;

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
    mysql_mutex_lock(&LOCK_content);

    READ_RECORD read_record_info;
    init_read_record(&read_record_info, thd, fromThisTable, NULL, 1, 0, FALSE);

    fromThisTable->use_all_columns();
    //sql_print_error("loadHttpContent: use_all_columns");
    contents.empty();
    //sql_print_error("loadHttpContent: empty");
    while (!(error = read_record_info.read_record(&read_record_info))) {
        http_content_row *aRow = new http_content_row;
        //sql_print_error("loop:  read_record_info");
        fromThisTable->field[0]->val_str(&newString1);
        fromThisTable->field[1]->val_str(&newString2);
        aRow->name.copy(newString1);
        aRow->content.copy(newString2);
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

void loadContent(THD *thd) {
    int error;
    Open_tables_backup backup;
    TABLE *tbl = open_sysTbl(thd, "http_contents", strlen("http_contents"), &backup, false, &error);
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




