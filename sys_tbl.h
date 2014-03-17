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

#include <sql_class.h>
#include <mysql_time.h>
#include <my_global.h>

#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation
#endif



#define HTTP_NAME_LEN 256
#define HTTP_CONTENT_LEN 200000
#define HTTP_TYPE_LEN 128



class http_content_row : public ilink {
public:
    char name[HTTP_NAME_LEN];
    char content[HTTP_CONTENT_LEN];
    char type[HTTP_TYPE_LEN];
};


TABLE *open_sysTbl(THD *thd,  const char *dbName,const char *tblName,
                   int tblNameLen, Open_tables_backup *tblBackup,
                   my_bool enableWrite, int *error);
int getStatus(String *result);
String getVariables(); 
void close_sysTbl(THD *thd, TABLE *table, Open_tables_backup *tblBackup);
void loadHttpContent(THD *thd,TABLE *fromThisTable);
void loadHttpVariables(THD *thd,TABLE *fromThisTable);
http_content_row *getContent(const char *name);
void loadContent(THD *thd) ;
void freeContent();


#endif