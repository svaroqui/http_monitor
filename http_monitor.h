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
#include <sql_class.h>
#include "mongoose.h"
//#include <openssl/ssl.h>
//#include <openssl/err.h>

namespace http_monitor {



int fill_plugin_version(THD *thd, TABLE_LIST *tables);
int fill_http_monitor(THD *thd, TABLE_LIST *tables, COND *cond);
int fill_misc_data(THD *thd, TABLE_LIST *tables);
int fill_linux_info(THD *thd, TABLE_LIST *tables);
static int begin_request_handler(struct mg_connection *conn); 

int calculate_server_uid(char *);
int prepare_linux_info();

pthread_handler_t background_thread(void *arg);
pthread_handler_t background_http_thread(void *arg);



/**
  The class for storing urls to send report data to.

  Constructors are private, the object should be created with create() method.
  send() method does the actual sending.
*/

class Server {
  protected:
  Server(LEX_STRING &server_arg) : full_url(server_arg) {}
  const LEX_STRING full_url;

  public:
  virtual ~Server() { my_free(full_url.str); }

  const char *uri()   { return full_url.str; }
  size_t uri_length() { return full_url.length; }
  virtual int send(const char* data, size_t data_length) =  0;
  virtual const LEX_STRING getHost() = 0;
  virtual const LEX_STRING getPort() = 0;
  virtual const LEX_STRING getPath() = 0;
  
  static Server* create(const char *server, size_t server_length);
};

Server* http_create(const char *url, size_t url_length);
Server* smtp_create(const char *url, size_t url_length);
Server* mysql_create(const char *url, size_t url_length);


extern String HTTP_REPORT;
extern int GALERA_STATUS;
extern int REPL_STATUS;
extern int HISTO_INDEX;

static const int SERVER_UID_SIZE= 29;
extern char server_uid_buf[SERVER_UID_SIZE+1];
extern char *port;
extern char *conn_user;
extern char *conn_password;
extern char *conn_host;
extern char *conn_socket;
extern char *smtp_user;
extern char *smtp_password;
extern char *smtp_email_from;
extern char *smtp_email_to;
extern char *smtp_certificat; 
extern char smtp_authentification;
extern char *notification_email;
extern char *bo_user;
extern char *bo_password;
extern char *node_name;
extern char *node_group;

extern char *aes_key;
extern char *salt_key;
extern char use_aes_encrypt;
extern char error_log;
extern char send_mail;
extern char use_spider;
extern char use_vmime;
extern char first_run;

extern uint conn_port;
extern ulong history_length;
extern ulong history_index;
extern ulong history_uptime;
extern ulong interval_send_query;
extern ulong interval_send_schema;
extern ulong interval_send_replication;
extern ulong interval_send_variable;
extern ulong interval_send_status;

extern ulong interval_get_query;
extern ulong interval_get_explain;
extern ulong interval_get_schema;
extern ulong interval_get_replication;
extern ulong interval_get_variable;
extern ulong interval_get_status;
extern char send_query;
extern char send_schema;
extern char send_replication;
extern char send_variable;
extern char send_status;
extern char http_content;
extern char send_dictionary;
    
extern ST_SCHEMA_TABLE *i_s_http_monitor;
extern ulong send_timeout, refresh_rate;
extern Server **mysql_servers;
extern uint mysql_servers_count;
extern Server **smtp_servers;
extern uint smtp_servers_count;
extern Server **http_servers;
extern uint http_servers_count;
/* these are used to communicate with the background thread */
extern mysql_mutex_t sleep_mutex;
extern mysql_cond_t sleep_condition;
extern volatile bool shutdown_plugin;
//Byte array of bitmap of 184 x 120 px:
		

} // namespace http_monitor

