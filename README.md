http_monitor
============

MariaDB 10 http monitoring using mongoose and dynamic columns 

Install
-------

Download MariaDB 10 tar.gz source code at https://downloads.mariadb.org/ 
Download http_monitor tar.gz and install it under the plugin directory of the server 

The plugin required following dependencies  
libxml2
libcurl  
libssl 
libgsasl 
libgnutls
libiconv
libicu

Compile MariaDB 
 
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/mariadb-monitoring-10.0.15 -DWITH_JEMALLOC=yes -DWITH_SSL=yes  .
make install  
 
Minimum configuration for the monitored backend
----------------------------------------------
performance_schema=1

Define a user with super privilege on all backend you need to monitor  
 
MariaDB> INSTALL PLUGIN http_monitor SONAME 'http_monitor.so'; 
 
Configuration 
-------------

Define all backend MariaDB servers to be monitored:
  
http_monitor_node_address=mysql://192.168.0.202:5054/backend2,mysql://192.168.0.203:5012/backend1,mysql://192.168.0.203:5054/backend3

The monitor use a connection to himself same user should be define in the monitor database and on remote backends, make sure that required information to etablish the connection locally is define.

- http_monitor_conn_host 
- http_monitor_conn_password 
- http_monitor_conn_port 
- http_monitor_conn_socket 
- http_monitor_conn_user 
 
Log 
---
- http_monitor_error_log=ON  
    Verbose information trace to the MariaDB error log 
 
Caching Collection 
------------------
Monitor is not sending trends to the back office for every collection, they are cached in the monitor for while and store only changing values. The default values should be fine but can be adjusted when workload consume to many bandwidth.  

- http_monitor_history_length=100  
    Number of entry to keep in history queue 
- http_monitor_refresh_rate=10  
    Collect rate in second

Who you are? 
-----------
The "Back Office" www.scrambledb.org keep track of trends in some encrypted database. 
We need a way to contact you to send alerts!
     
- http_monitor_node_group=laptop
- http_monitor_node_name=mymac

- http_monitor_server_uid 
   An auto generated unique machine idendifier 
   Used as password to the back office  
   Used as encrypted and hashing key 
 
- http_monitor_notification_email 
   Email adress to send notification 
 
Internal Web Server parameter 
-----------------------------
- http_monitor_port=8080
 
Saas monitoring 
----------------
Http monitor send trend of changes using TLS email to scrambledb.org. 

- http_monitor_send_mail=ON   
- http_monitor_smtp_address=smtp://smtp.scrambledb.org:587 
- http_monitor_smtp_certificat=/Users/svar/data/smtpd.crt 
- http_monitor_smtp_email_from=monitor@scrambledb.org 
- http_monitor_smtp_email_to=support@scrambledb.org 
- http_monitor_smtp_password=support 
- http_monitor_smtp_user=support@scrambledb.org 
