http_monitor
============

MariaDB 10 http monitoring using mongoose

http_monitor.sql

Http monitor send trend using TLS email to scrambledb.org.  

To enable that feature copy the smtpd.crt to the datadir of your mariadb server. 
set http_monitor_smtp_certificat variable to the location of the certificat 

http_monitor_aes_key  -> aes_key to emcrypt email content                                                   

the host connection to monitor default to 127.0.0.1

http_monitor_conn_host
http_monitor_conn_password 
http_monitor_conn_port
http_monitor_conn_socket
http_monitor_conn_user

To send trace to error log
--------------------------  
http_monitor_error_log=ON   

Number of entry to keep in history and collection rate
------------------------------------------------------
http_monitor_history_length 5
http_monitor_refresh_rate  60
http_monitor_node_group=laptop 
http_monitor_node_name       | mymac                                                           |

| http_monitor_port            | 8080                                                            |
| http_monitor_server_uid      | zOBFqxNABETgELkfy0nYH8G+Olk=                                    |


To send email to my place 


| http_monitor_send_mail       | ON                                                              |
| http_monitor_smtp_address    | smtp://smtp.scrambledb.org:587                                  |
| http_monitor_smtp_certificat | /Users/svar/data/smtpd.crt                                      |
| http_monitor_smtp_email_from | monitor@scrambledb.org                                          |
| http_monitor_smtp_email_to   | support@scrambledb.org                                          |
| http_monitor_smtp_password   | support                                                         |
| http_monitor_smtp_user       | support@scrambledb.org             
 
