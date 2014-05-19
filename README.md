http_monitor
============

MariaDB 10 http monitoring using mongoose

Install
------- 

Download MariaDB 10 tar.gz source code at https://downloads.mariadb.org/ 
Download http_monitor tar.gz and install it under the plugin directory of the server 

Compile MariaDB as usual.
# cmake . 
# make install 
 
load the provided http_monitor.sql into the mysql system schema 
# mysql -uroot -p mysql < http_monitor.sql

MariaDB> INSTALL PLUGIN http_monitor SONAME 'http_monitor.so'; 

Http monitor send trend of changes using TLS email to scrambledb.org.  
Copy the smtpd.crt to the datadir of your mariadb server. 
set http_monitor_smtp_certificat variable to the location of the certificat file. 

MariaDB instance to monitor 
---------------------------
http_monitor_conn_host
http_monitor_conn_password 
http_monitor_conn_port
http_monitor_conn_socket
http_monitor_conn_user

Log
---  
http_monitor_error_log=ON   
    Verbose information trace to the MariaDB error log 

Collection 
----------
http_monitor_history_length=100
    Number of entry to keep in history queue 
http_monitor_refresh_rate=10
    Collect rate in second

Tail us who you are? 
--------------------   
http_monitor_node_group=laptop 
http_monitor_node_name=mymac
http_monitor_server_uid=zOBFqxNABETgELkfy0nYH8G+Olk=
    An auto generated unique machine idendifier  
http_monitor_bo_user
http_monitor_bo_password
http_monitor_notification_email
    Email adress to send notification

Internal Web Server parameter 
-----------------------------
http_monitor_port=8080  

Sending history to us
---------------------
http_monitor_send_mail=ON   
http_monitor_smtp_address=smtp://smtp.scrambledb.org:587
http_monitor_smtp_certificat=/Users/svar/data/smtpd.crt
http_monitor_smtp_email_from=monitor@scrambledb.org
http_monitor_smtp_email_to=support@scrambledb.org
http_monitor_smtp_password=support
http_monitor_smtp_user=support@scrambledb.org 
 
