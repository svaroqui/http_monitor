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

#ifndef __MYSQL_SYS_TBL__
#define __MYSQL_SYS_TBL__

#define MYSQL_SERVER 1


//#include <sql_class.h>
#include <mysql_time.h>
#include <my_global.h>
//#include <sql_common.h>
#include <mysql.h>

#include <string>
#include <iostream>
#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation
#endif



namespace http_monitor {

class http_content_row : public ilink {
public:
    std::string name;
    std::string content;
    std::string type;
    virtual ~http_content_row(){
     //    free( &content); 
    }
};


class http_query : public ilink {
public:
    String query;
    virtual ~http_query(){
     //    free( &content); 
    }
};


/*TABLE *open_sysTbl(THD *thd,  const char *dbName,const char *tblName,
                   int tblNameLen, Open_tables_backup *tblBackup,
                   my_bool enableWrite, int *error);
void close_sysTbl(THD *thd, TABLE *table, Open_tables_backup *tblBackup);
void loadHttpContent(THD *thd,TABLE *fromThisTable);*/

http_content_row *getContent(String *name);
void loadContent(THD *thd) ;
void freeContent();
int updateContent(MYSQL *conn);
int loadHttpContentConn();
static my_bool sql_connect(MYSQL *mysql, uint wait);
void finish_with_error(MYSQL *conn, String *query);
std::string remove_letter( std::string str, char c );
#endif
}