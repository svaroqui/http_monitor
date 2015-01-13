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
#include "sys_tbl.h"

#ifdef HAVE_NETDB_H
#endif
#include <stdio.h>
#include <string.h>
#ifdef HTTP_MONITOR_HAVE_CURL
#include <curl/curl.h>
#include <base64.h>
#endif
#include <string>

#include "vmime/vmime.hpp"
#include "vmime/platforms/posix/posixHandler.hpp"

namespace http_monitor {

  

#define FROM    "<monitor@scrambledb.org>"
#define TO      "<support@scrambledb.org>"


#ifdef _WIN32
#include <ws2tcpip.h>
#define addrinfo ADDRINFOA
#endif


 
    static const uint FOR_READING = 0;
    static const uint FOR_WRITING = 1;
        
 
    class Server_smtp : public Server {
    protected:
        const LEX_STRING host, port, path;
   
        bool ssl;

        Server_smtp(LEX_STRING &url_arg, LEX_STRING &host_arg,
                LEX_STRING &port_arg, LEX_STRING &path_arg, bool ssl_arg) :
        Server(url_arg), host(host_arg), port(port_arg), path(path_arg), ssl(ssl_arg) {
        }

        ~Server_smtp() {
            my_free(host.str);
            my_free(port.str);
            my_free(path.str);
        }

    public:
        int send(const char* data, size_t data_length);
        const LEX_STRING getHost(){return this->host;}
        const LEX_STRING getPort(){return this->port;}
        const LEX_STRING getPath(){return this->path;} 
       
        friend Server* smtp_create(const char *url, size_t url_length);
    };
#ifdef HTTP_MONITOR_HAVE_CURL
    static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    struct upload_status {
        int lines_read;
    };
    static struct upload_status upload_ctx;
    
    static const char* mail_data;
    static size_t mail_data_length;

    
    static const int CHARS = 200; //Sending 54 chararcters at a time with \r , \n and \0 it becomes 57 
    static const int ADD_SIZE = 15; // ADD_SIZE for TO,FROM,SUBJECT,CONTENT-TYPE,CONTENT-TRANSFER-ENCODING,CONETNT-DISPOSITION and \r\n
    static const int SEND_BUF_SIZE = 54;
    const char *data;
    class attach_row : public ilink {
        public:
        String row;
        virtual ~attach_row(){}
    };      
    I_List<attach_row> attach_rows;
    static char (*fileBuf)[CHARS] = NULL;
static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};
#endif 
static vmime::shared_ptr <vmime::net::session> g_session
	= vmime::make_shared <vmime::net::session>();


class interactiveCertificateVerifier : public vmime::security::cert::defaultCertificateVerifier
{
public:

	void verify(vmime::shared_ptr <vmime::security::cert::certificateChain> chain, const vmime::string& hostname)
	{
		try
		{
			setX509TrustedCerts(m_trustedCerts);

			defaultCertificateVerifier::verify(chain, hostname);
		}
		catch (vmime::exceptions::certificate_verification_exception&)
		{
			return;
		}
	}

private:

	static std::vector <vmime::shared_ptr <vmime::security::cert::X509Certificate> > m_trustedCerts;
};





std::vector <vmime::shared_ptr <vmime::security::cert::X509Certificate> >
	interactiveCertificateVerifier::m_trustedCerts;


#ifdef HTTP_MONITOR_HAVE_CURL

std::string base64_encode(const ::std::string &bindata)
{
   using ::std::string;
   using ::std::numeric_limits;
   const ::std::size_t binlen = bindata.size();
   // Use = signs so the end is properly padded.
   string retval((((binlen + 2) / 3) * 4), '=');
   ::std::size_t outpos = 0;
   int bits_collected = 0;
   unsigned int accumulator = 0;
   const string::const_iterator binend = bindata.end();
 
   for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
      accumulator = (accumulator << 8) | (*i & 0xffu);
      bits_collected += 8;
      while (bits_collected >= 6) {
         bits_collected -= 6;
         retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
      }
   }
   if (bits_collected > 0) { // Any trailing bits that are missing.
      assert(bits_collected < 6);
      accumulator <<= 6 - bits_collected;
      retval[outpos++] = b64_table[accumulator & 0x3fu];
   }
   assert(outpos >= (retval.size() - 2));
   assert(outpos <= retval.size());
   return retval;
}

    size_t format_mail() {
        size_t len(0), buffer_size(0);

        int no_of_rows = mail_data_length / SEND_BUF_SIZE;
        //no_of_rows = 1;

        fileBuf = new char[ADD_SIZE + no_of_rows ][CHARS];
        String emailto;
        emailto.append("To: ");
        emailto.append(smtp_email_to);
        emailto.append("\n");
      //  strcpy(fileBuf[len++], "To: " TO "\n");
          strcpy(fileBuf[len++],emailto.c_ptr());
        
        buffer_size += strlen(fileBuf[len - 1]) + 1; // 1 for \0
        strcpy(fileBuf[len++], "From: " FROM "\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++], "Subject: HTTP Monitor\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++],"MIME-Version: 1.0\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++], "Content-type: multipart/mixed; boundary=\"separateur-mime\"\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++], "--separateur-mime\n");  
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++], "Content-type: text/plain; charset=ISO-8859-1\n\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++], "--separateur-mime\n");  
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++],"Content-Type: application/octet-stream;\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++]," name=status.txt\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++],"Content-Transfer-Encoding: 7bit\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++],"Content-Disposition: attachment;\n");
        buffer_size += strlen(fileBuf[len - 1]) + 1;
        strcpy(fileBuf[len++]," filename=status.txt\n\n"); //"double \n" necessaire sinon la prochaine donnee .
        buffer_size += strlen(fileBuf[len - 1]) + 1;

        char* temp_buf = new char[SEND_BUF_SIZE + 4]; //taking extra size of 4 bytes
        std::string encodedStr, temp_buf_str;
        int read(0);
        int lenatt(0);
        for (; lenatt < (unsigned) no_of_rows ; ++lenatt) {
         
            memcpy(temp_buf, mail_data +read, sizeof (char) * SEND_BUF_SIZE);
            read += SEND_BUF_SIZE;
            temp_buf[SEND_BUF_SIZE] = '\0';
            temp_buf_str = std::string(temp_buf);
           // encodedStr = base64_encode(temp_buf_str);
            memcpy(fileBuf[len++], temp_buf_str.c_str(), temp_buf_str.length() + 1);
            
            buffer_size += encodedStr.length() + 1; // 1 for \0

        }
        if (mail_data_length != (unsigned) read) {
            memcpy(temp_buf, &mail_data[read], sizeof (char) * (mail_data_length - read));
            temp_buf[mail_data_length - read] = '\0';
            temp_buf_str = std::string(temp_buf);
         //   encodedStr = base64_encode(temp_buf_str);
            memcpy(fileBuf[len++], temp_buf_str.c_str(), temp_buf_str.length() + 1);
            buffer_size += encodedStr.length() + 1; // 1 for \0

        }
        strcpy(fileBuf[len++], "\n\n");
        buffer_size += 2;
        strcpy(fileBuf[len++], "*END*");
        buffer_size += 6;

        delete[] temp_buf;
        return buffer_size;
    }

   

    
    size_t split_mail() {
        size_t len(0), buffer_size(0);

        int no_of_rows = mail_data_length / SEND_BUF_SIZE;
        //no_of_rows = 1;

        String emailto;
        emailto.append("To: ");
        emailto.append(smtp_email_to);
        emailto.append("\n");
        
        attach_row *aRow; 
        aRow = new attach_row;
        aRow->row.append(emailto);
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "From: " FROM "\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "Subject: HTTP Monitor\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "MIME-Version: 1.0\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "--separateur-mime\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "Content-type: text/plain; charset=ISO-8859-1\n\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "--separateur-mime\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "Content-Type: application/octet-stream;\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  " name=status.txt\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "Content-Transfer-Encoding: 7bit\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  "Content-Disposition: attachment;\n");
        attach_rows.push_back(aRow);
        aRow = new attach_row;
        aRow->row.append((char *)  " filename=status.txt\n\n");
        attach_rows.push_back(aRow);
        
        char* temp_buf = new char[SEND_BUF_SIZE + 1]; //taking extra size of 4 bytes
    
        int read(0);
        int lenatt(0);
        for (; lenatt < (unsigned) no_of_rows ; ++lenatt) {
         
            memcpy(temp_buf, mail_data +read, sizeof (char) * SEND_BUF_SIZE);
            read += SEND_BUF_SIZE;
            
            temp_buf[SEND_BUF_SIZE] = '\0';
            aRow = new attach_row;
            aRow->row.append((char *)temp_buf );
            attach_rows.push_back(aRow);
        }
        if (mail_data_length != (unsigned) read) {
            memcpy(temp_buf, &mail_data[read], sizeof (char) * (mail_data_length - read));
            temp_buf[mail_data_length - read] = '\0';
            aRow = new attach_row;
            aRow->row.append((char *)temp_buf );
            attach_rows.push_back(aRow);
        }
        aRow = new attach_row;
        aRow->row.append((char *)  "\n\n");
        attach_rows.push_back(aRow);
     
        delete[] temp_buf;
        return 0;
    }

    static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
        struct upload_status *upload_ctx = (struct upload_status *) userp;
        size_t len(0), buffer_size(0);
        if (error_log)
           sql_print_information("http_monitor *payload_source*: ask for '%lu ,  %lu", size,nmemb) ;
             
        
        if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
              if (error_log)
               sql_print_information("http_monitor *payload_source*: nothink quit") ;
            return 0;
        }

        

        data = fileBuf[upload_ctx->lines_read];

        if (strcmp(data, "*END*")) {
            size_t len = strlen(data);
            
            memcpy(ptr, data, len);
            upload_ctx->lines_read++;
             if (error_log) 
                sql_print_information("http_monitor *payload_source*: sending for '%lu ,  %s", len,data) ;
         

            return len;
        }
        
           sql_print_information("http_monitor *payload_source*: stopped send 0") ;
        
        return 0;
    }
    
    
    
      static size_t payload_source_vector(void *ptr, size_t size, size_t nmemb, void *userp) {
        struct upload_status *upload_ctx = (struct upload_status *) userp;
        size_t len(0), buffer_size(0);
        if (error_log)
           sql_print_information("http_monitor *payload_source*: ask for '%lu ,  %lu", size,nmemb) ;
             
        
        if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
              if (error_log)
               sql_print_information("http_monitor *payload_source*: nothink quit") ;
            return 0;
        }
         attach_row *aRow;
         I_List_iterator<attach_row> contentsIter(attach_rows);
         int ct=0;
         while ((aRow = contentsIter++)) {
               
             //   sql_print_information("http_monitor *payload_source*: testing for '%i ,ct %i,  %s", upload_ctx->lines_read ,ct , aRow->row.c_ptr()) ;
             
                if(upload_ctx->lines_read==ct) {
                 
                  
                        size_t len = strlen(data);
                        memcpy(ptr, aRow->row.c_ptr(), aRow->row.length());
                        upload_ctx->lines_read++;
                         if (error_log) 
                            sql_print_information("http_monitor *payload_source*: sending for '%u ,  %s",  aRow->row.length(),aRow->row.c_ptr()) ;
                        return aRow->row.length();
                  
                   }
                     ct++;
                 
         }

        
        sql_print_information("http_monitor *payload_source*: stopped send 0") ;
        
        return 0;
    }
    
#endif 

    /**
      create a Server_smtp object out of the url, if possible.

      @note
      Arbitrary limitations here.

      The url must be smtp[s]://hostname[:port]
      No username:password@ or ?script=parameters are supported.

      But it's ok. This is not a generic purpose www browser - it only needs to be
      good enough to POST the data to mariadb.org.
     */
    Server* smtp_create(const char *url, size_t url_length) {
        const char *s;
        LEX_STRING full_url = {const_cast<char*> (url), url_length};
        LEX_STRING host, port, path;
        bool ssl = false;
        if (is_prefix(url, "smtp://"))
            s = url + 7;
#ifdef HAVE_OPENSSL
        else if (is_prefix(url, "smtps://")) {
            ssl = true;
            s = url + 8;
        }
#endif
        else {
            sql_print_error("http_monitor plugin: wrong prefix ");
            return NULL;
        }
        for (url = s; *s && *s != ':' && *s != '/'; s++) /* no-op */;
        host.str = const_cast<char*> (url);
        host.length = s - url;

        if (*s == ':') {
            for (url = ++s; *s && *s >= '0' && *s <= '9'; s++) /* no-op */;
            port.str = const_cast<char*> (url);
            port.length = s - url;
        } else {
            if (ssl) {
                port.str = const_cast<char*> ("587");
                port.length = 3;
            } else {
                port.str = const_cast<char*> ("25");
                port.length = 2;
            }
        }

        if (*s == 0) {
            path.str = const_cast<char*> ("/");
            path.length = 1;
        } else {
            path.str = const_cast<char*> (s);
            path.length = strlen(s);
        }

        if (!host.length || !port.length) {
            sql_print_error("http_monitor plugin: wrong length ");
            return NULL;
        }
        host.str = my_strndup(host.str, host.length, MYF(MY_WME));
        port.str = my_strndup(port.str, port.length, MYF(MY_WME));

        if (!host.str || !port.str) {
            my_free(host.str);
            my_free(port.str);
            my_free(path.str);
            sql_print_error("http_monitor plugin: wrong string ");
            return NULL;
        }

        return new Server_smtp(full_url, host, port, path, ssl);
    }
    
    

    int Server_smtp::send(const char* data, size_t data_length) {

        if (! http_monitor::use_vmime) {
            
            
        
 #ifdef HTTP_MONITOR_HAVE_CURL        
        CURL *curl;
        CURLcode res = CURLE_OK;
        struct curl_slist *recipients = NULL;
        //struct fileBuf_upload_status file_upload_ctx;

        mail_data = data;
        mail_data_length = data_length;
        format_mail();
        //split_mail();
        upload_ctx.lines_read = 0;
        // file_upload_ctx.lines_read = 0;
        curl = curl_easy_init();
        if (curl) {
            /* Set username and password */
            if (smtp_authentification){
                 curl_easy_setopt(curl, CURLOPT_USERNAME,smtp_user);
                curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_password);
            }
            String mail_server=0;
            mail_server.append(full_url.str);
            curl_easy_setopt(curl, CURLOPT_URL, mail_server.c_ptr());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);
            curl_easy_setopt(curl, CURLOPT_CAINFO, smtp_certificat);
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_email_from);
            recipients = curl_slist_append(recipients, smtp_email_to);
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
          //  curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source_vector);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            if (error_log)
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

            /* Send the message */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if (res != CURLE_OK)
                sql_print_error("curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

            /* Free the list of recipients */
            curl_slist_free_all(recipients);

            /* Always cleanup */
            curl_easy_cleanup(curl);
        }
        if(error_log)
                sql_print_information("http_monitor plugin: mail report to '%s' was sent",
                full_url.str);
        attach_rows.empty();
        return (int) res;
 #endif   

        }
        else {
            
            try
	{
		std::locale::global(std::locale(""));
	}
	catch (std::exception &)
	{
		std::setlocale(LC_ALL, "");
	}

	try
	{
		
            
            
                vmime::shared_ptr <vmime::net::transport> tr;
                String mail_server=0;
                mail_server.append(full_url.str);
            
                vmime::utility::url url(mail_server.c_ptr());
            	tr = g_session->getTransport(url);
		// Enable TLS support if available
		tr->setProperty("connection.tls", true);
		// Set the time out handler
	//	tr->setTimeoutHandlerFactory(vmime::make_shared <vmime::net::timeoutHandlerFactory>());
                if (smtp_authentification){
                  tr->setProperty("auth.username", smtp_user);
                  tr->setProperty("auth.password", smtp_password);
                  tr->setProperty("options.need-authentication", true);
                } else
                {
                    
                   tr->setProperty("options.need-authentication", false);
                }    
                
                
                tr->setCertificateVerifier
			(vmime::make_shared <interactiveCertificateVerifier>());
                vmime::messageBuilder mb;
                
                mb.setExpeditor(vmime::mailbox(smtp_email_from));
                vmime::addressList to;
		to.appendAddress(vmime::make_shared <vmime::mailbox>(smtp_email_to));
                mb.setRecipients(to);
                mb.setSubject(vmime::text("HTTP Monitor"));

                 mb.getTextPart()->setText(vmime::make_shared <vmime::stringContentHandler>(
			data));            
 
 
          /*      vmime::shared_ptr <vmime::fileAttachment> a = vmime::make_shared <vmime::fileAttachment >
		(
			
                          
                        vmime::word("name") ,
			vmime::mediaType("application/octet-stream"),   // content type
			  vmime::text("json")            // description
		);

		a->getFileInfo().setFilename("monitor-infos.json");
		a->getFileInfo().setCreationDate(vmime::datetime("30 Apr 2003 14:30:00 +0200"));

                
		mb.appendAttachment(a);
           * /
                
/*		// Construction
		vmime::shared_ptr <vmime::message> msg = mb.construct();

		// Raw text generation
		vmime::string dataToSend = msg->generate();

		std::cout << "Generated message:" << std::endl;
		std::cout << "==================" << std::endl;
		std::cout << std::endl;
		std::cout << dataToSend << std::endl;
*/
                if(error_log)  sql_print_information("http_monitor plugin: vmine before connect") ;
                
                tr->connect();
                if(error_log) sql_print_information("http_monitor plugin: vmine before send") ;
  
                tr->send(mb.construct());
  	}
	// VMime exception
	catch (vmime::exception& e)
	{
		std::cout << "vmime::exception: " << e.what() << std::endl;
		//throw;
	}
	// Standard exception
	catch (std::exception& e)
	{
		std::cout << "std::exception: " << e.what() << std::endl;
		//throw;
	}

	std::cout << std::endl;
        }
         return (int) 0;        
            
       
    }
    
   
    
} // namespace http_monitor

