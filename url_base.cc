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

namespace http_monitor {

Server* http_create(const char *url, size_t url_length);
Server* smtp_create(const char *url, size_t url_length);
Server* mysql_create(const char *url, size_t url_length);
/**
  creates an Server object out of an url, if possible.

  This is done by invoking corresponding creator functions
  of the derived classes, until the first not NULL result.
*/
Server* Server::create(const char *url, size_t url_length)
{
  url= my_strndup(url, url_length, MYF(MY_WME));
   
  if (!url){
    sql_print_information("http_monitor plugin: return null url from base");
    return NULL;
  }
   if (( is_prefix(url, "mysql://") || is_prefix(url, "mysqls://"))  ) {
       Server *self= mysql_create(url, url_length);
       if (!self)
               my_free(const_cast<char*>(url));
        return self;
   }
   if (( is_prefix(url, "smtp://") || is_prefix(url, "smtps://"))  ) {
       Server *self= smtp_create(url, url_length);
       if (!self)
               my_free(const_cast<char*>(url));
       return self;
   
   }
   if (( is_prefix(url, "http://") || is_prefix(url, "https://"))  ) {
       Server *self= http_create(url, url_length);
       if (!self)
               my_free(const_cast<char*>(url));
       return self;
   
   }
 // sql_print_information("http_monitor plugin: return self  from base  ");
  return NULL;
}

} // namespace feedback
