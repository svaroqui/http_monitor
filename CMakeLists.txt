


IF(APPLE)
  # Workaround linker bug on OSX 10.7
  ADD_DEFINITIONS(-fno-common)
ENDIF()


#set(BUILD_SHARED_LIBS OFF)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgc -static-libc -static-libstdc++ -static")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC  -fexceptions" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -frtti -fexceptions" )

# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  CURL_LIBRARIES    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.


OPTION(
	HTTP_MONITOR_HAVE_CURL
	"Enable CURL support (requires Curl library)"
	OFF
)

IF(HTTP_MONITOR_HAVE_CURL)
FIND_PATH(CURL_INCLUDE_DIR NAMES curl/curl.h)
MARK_AS_ADVANCED(CURL_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(CURL_LIBRARY NAMES 
    curl
  # Windows MSVC prebuilts:
    curllib
    libcurl_imp
    curllib_static
)
MARK_AS_ADVANCED(CURL_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG CURL_LIBRARY CURL_INCLUDE_DIR)

IF(CURL_FOUND)
  SET(CURL_LIBRARIES ${CURL_LIBRARY})
  SET(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
  SET(HTTP_MONITOR_HAVE_CURL YES) 
ENDIF(CURL_FOUND)

ENDIF(HTTP_MONITOR_HAVE_CURL)


FIND_PACKAGE(Boost)
IF(NOT Boost_FOUND)
   MESSAGE(STATUS "Boost not found. Vmine will not be compiled")
  RETURN()
ENDIF()

IF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARY)
   # in cache already
   SET(gnutls_FIND_QUIETLY TRUE)
ENDIF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARY)

IF (NOT WIN32)
   # try using pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   # also fills in GNUTLS_DEFINITIONS, although that isn't normally useful
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_GNUTLS gnutls)
   SET(GNUTLS_DEFINITIONS ${PC_GNUTLS_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(GNUTLS_INCLUDE_DIR gnutls/gnutls.h
   HINTS
   ${PC_GNUTLS_INCLUDEDIR}
   ${PC_GNUTLS_INCLUDE_DIRS}
   )

FIND_LIBRARY(GNUTLS_LIBRARY NAMES gnutls libgnutls
   HINTS
   ${PC_GNUTLS_LIBDIR}
   ${PC_GNUTLS_LIBRARY_DIRS}
   )

MARK_AS_ADVANCED(GNUTLS_INCLUDE_DIR GNUTLS_LIBRARY)


# handle the QUIETLY and REQUIRED arguments and set GNUTLS_FOUND to TRUE if 
# all listed variables are TRUE
 

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GnuTLS DEFAULT_MSG GNUTLS_LIBRARY GNUTLS_INCLUDE_DIR)
IF(GNUTLS_FOUND)
  SET(GNUTLS_LIBRARIES    ${GNUTLS_LIBRARY})
  SET(GNUTLS_INCLUDE_DIRS ${GNUTLS_INCLUDE_DIR})
  
ENDIF(GNUTLS_FOUND)

# INCLUDE(FindGSasl.cmake)

OPTION(
	VMIME_HAVE_SASL_SUPPORT
	"Enable SASL support (requires GNU SASL library)"
	ON
)

IF(VMIME_HAVE_SASL_SUPPORT)


IF (GSASL_INCLUDE_DIR AND GSASL_LIBRARIES)
   # in cache already
   SET(GSasl_FIND_QUIETLY TRUE)
ENDIF (GSASL_INCLUDE_DIR AND GSASL_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_GSASL libgsasl)
   SET(GSASL_DEFINITIONS ${PC_GSASL_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(GSASL_INCLUDE_DIR gsasl.h
   HINTS
   ${PC_GSASL_INCLUDEDIR}
   ${PC_GSASL_INCLUDE_DIRS}
   PATH_SUFFIXES gsasl
   )

FIND_LIBRARY(GSASL_LIBRARIES NAMES gsasl libgsasl
    HINTS
    ${PC_GSASL_LIBDIR}
    ${PC_GSASL_LIBRARY_DIRS}
  )

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set GSASL_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GSASL DEFAULT_MSG GSASL_LIBRARIES GSASL_INCLUDE_DIR)

MARK_AS_ADVANCED(GSASL_INCLUDE_DIR GSASL_LIBRARIES)


ENDIF()



# FIND_PATH( ICONV_INCLUDE_DIR NAMES iconv.h )
# MARK_AS_ADVANCED( ICONV_INCLUDE_DIR )

# FIND_LIBRARY( ICONV_LIBRARY NAMES iconv )
# MARK_AS_ADVANCED( ICONV_LIBRARY )

# IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARY)
#  SET(ICONV_FOUND 1)
#  SET(ICONV_LIBRARIES ${ICONV_LIBRARY})
#  SET(ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
# ELSE(ICONV_INCLUDE_DIR AND ICONV_LIBRARY)
#  SET(ICONV_FOUND 0)
#  SET(ICONV_LIBRARIES)
#  SET(ICONV_INCLUDE_DIRS)
# ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARY)

# IF(NOT ICONV_FOUND)
#  SET(ICONV_DIR_MESSAGE
#    "Iconv was not found. Make sure ICONV_LIBRARY and ICONV_INCLUDE_DIR areset.")
#    IF(NOT ICONV_FIND_QUIETLY)
#      MESSAGE(STATUS ${ICONV_DIR_MESSAGE})
#    ELSE(NOT ICONV_FIND_QUIETLY)
#      IF(ICONV_FIND_REQUIRED)
#        MESSAGE(FETAL_ERROR ${ICONV_DIR_MESSAGE})
#      ENDIF(ICONV_FIND_REQUIRED)
#    ENDIF(NOT ICONV_FIND_QUIETLY)
# ENDIF(NOT ICONV_FOUND)

# - Find the ICU includes and library
# This module defines
#  ICU_INCLUDE_DIRS: Where to find unicode/ustring.h
#  ICU_LIBRARIES: The libraries to link against to use ICU
#  ICU_FOUND: ICU has been found

 find_program(ICU_CONFIG icu-config)
 if(ICU_CONFIG)
  execute_process(
    COMMAND ${ICU_CONFIG} --ldflags
    OUTPUT_VARIABLE ICU_LIBRARY
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(STRIP ${ICU_LIBRARY} ICU_LIBRARY)
endif()

find_path(ICU_INCLUDE_DIR unicode/ustring.h
  ${CMAKE_INSTALL_PREFIX}/include
  /usr/local/include
  /usr/include
)

 if(ICU_INCLUDE_DIR AND ICU_LIBRARY)
  set(ICU_INCLUDE_DIRS ${ICU_INCLUDE_DIR})
  set(ICU_LIBRARIES ${ICU_LIBRARY})
  set(ICU_FOUND YES)
 else()
  set(ICU_FOUND NO)
endif()

if(ICU_FOUND)
  if(NOT ICU_FIND_QUIETLY)
    message(STATUS "Found ICU!")
  endif()
 else()
  if(ICU_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find ICU library")
  endif()
 endif()

 mark_as_advanced(ICU_CONFIG ICU_INCLUDE_DIR ICU_LIBRARY)

# copy source

##############################################################################
# Charset conversion library

INCLUDE(FindIconv.cmake)
#INCLUDE(FindICU.cmake)

#FIND_PACKAGE(ICU QUIET)

IF(WIN32)
	SET(VMIME_CHARSETCONV_LIB_DETECTED "win")
ELSEIF(ICU_LIBRARIES)
	SET(VMIME_CHARSETCONV_LIB_DETECTED "icu")
ELSEIF(ICONV_FOUND)
	SET(VMIME_CHARSETCONV_LIB_DETECTED "iconv")
ENDIF()

SET(
	VMIME_CHARSETCONV_LIB
	"${VMIME_CHARSETCONV_LIB_DETECTED}"
	CACHE
	STRING
	"Library to use for charset conversion"
)
SET_PROPERTY(
	CACHE
	VMIME_CHARSETCONV_LIB
	PROPERTY STRINGS win iconv icu
)


IF(VMIME_CHARSETCONV_LIB STREQUAL "iconv")

	INCLUDE_DIRECTORIES(
                vmime
		${INCLUDE_DIRECTORIES}
		${ICONV_INCLUDE_DIR}
	)

	IF(VMIME_BUILD_SHARED_LIBRARY)
		TARGET_LINK_LIBRARIES(
                        vmime
			${TARGET_LINK_LIBRARIES}
			${ICONV_LIBRARIES} STATIC 
		)
	ENDIF()

	SET(VMIME_PKGCONFIG_LIBS "${VMIME_PKGCONFIG_LIBS} ${ICONV_LIBRARIES}")
	SET(VMIME_PKGCONFIG_CFLAGS "${VMIME_PKGCONFIG_CFLAGS} -I${ICONV_INCLUDE_DIR}")

	SET(VMIME_CHARSETCONV_LIB_IS_ICONV "ON")
	SET(VMIME_CHARSETCONV_LIB_IS_ICU "OFF")
	SET(VMIME_CHARSETCONV_LIB_IS_WIN "OFF")

ELSEIF(VMIME_CHARSETCONV_LIB STREQUAL "icu")

	INCLUDE_DIRECTORIES(
                vmime
		${INCLUDE_DIRECTORIES}
		${ICU_INCLUDE_DIRS}
	)

	IF(VMIME_BUILD_SHARED_LIBRARY)
		TARGET_LINK_LIBRARIES(
                        vmime
			${TARGET_LINK_LIBRARIES}
			${ICU_LIBRARIES} STATIC
		)
	ENDIF()

	SET(VMIME_PKGCONFIG_LIBS "${VMIME_PKGCONFIG_LIBS} ${ICU_LIBRARIES}")
	SET(VMIME_PKGCONFIG_CFLAGS "${VMIME_PKGCONFIG_CFLAGS} -I${ICU_INCLUDE_DIRS}")

	SET(VMIME_CHARSETCONV_LIB_IS_ICONV "OFF")
	SET(VMIME_CHARSETCONV_LIB_IS_ICU "ON")
	SET(VMIME_CHARSETCONV_LIB_IS_WIN "OFF")

ELSEIF(VMIME_CHARSETCONV_LIB STREQUAL "win")

	SET(VMIME_CHARSETCONV_LIB_IS_ICONV "OFF")
	SET(VMIME_CHARSETCONV_LIB_IS_ICU "OFF")
	SET(VMIME_CHARSETCONV_LIB_IS_WIN "ON")

ELSE()

	MESSAGE(FATAL_ERROR "No charset conversion library was selected/found")

ENDIF()

# copy source



INCLUDE_DIRECTORIES( BEFORE ${Boost_INCLUDE_DIRS}
                     BEFORE ${GNUTLS_INCLUDE_DIRS}
                      BEFORE ${GSASL_INCLUDE_DIRS}
                #      BEFORE ${ICONV_INCLUDE_DIRS}
                #      BEFORE ${ICU_INCLUDE_DIRS}
                    BEFORE ${INCLUDE_DIRECTORIES}
#                    ${CMAKE_SOURCE_DIR}/sql
                    ${CMAKE_BINARY_DIR}/pcre
                    ${CMAKE_SOURCE_DIR}/pcre
                    ${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/libmysql
                    ${CMAKE_SOURCE_DIR}/client
                    ${SSL_INCLUDE_DIRS}
                    ${CURL_INCLUDE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/contrib          # for "contrib/"
                    ${CMAKE_CURRENT_SOURCE_DIR}/      # for "vmime/   
                           
)



SET(LIBVMINE_SOURCES 
vmime/address.cpp
vmime/addressList.cpp
vmime/attachmentHelper.cpp
vmime/base.cpp
vmime/body.cpp
vmime/bodyPart.cpp
vmime/bodyPartAttachment.cpp
vmime/charset.cpp
vmime/charsetConverter.cpp
vmime/charsetConverter_iconv.cpp
vmime/charsetConverter_icu.cpp
vmime/charsetConverter_idna.cpp
vmime/charsetConverter_win.cpp
vmime/charsetConverterOptions.cpp
vmime/component.cpp
vmime/constants.cpp
vmime/contentDisposition.cpp
vmime/contentDispositionField.cpp
vmime/contentHandler.cpp
vmime/contentTypeField.cpp
vmime/context.cpp
vmime/dateTime.cpp
vmime/defaultAttachment.cpp
vmime/disposition.cpp
vmime/emailAddress.cpp
vmime/emptyContentHandler.cpp
vmime/encoding.cpp
vmime/exception.cpp
vmime/fileAttachment.cpp
vmime/fileContentHandler.cpp
vmime/generatedMessageAttachment.cpp
vmime/generationContext.cpp
vmime/header.cpp
vmime/headerField.cpp
vmime/headerFieldFactory.cpp
vmime/headerFieldValue.cpp
vmime/htmlTextPart.cpp
vmime/mailbox.cpp
vmime/mailboxField.cpp
vmime/mailboxGroup.cpp
vmime/mailboxList.cpp
vmime/mdn/MDNHelper.cpp
vmime/mdn/MDNInfos.cpp
vmime/mdn/receivedMDNInfos.cpp
vmime/mdn/sendableMDNInfos.cpp
vmime/mediaType.cpp
vmime/message.cpp
vmime/messageBuilder.cpp
vmime/messageId.cpp
vmime/messageIdSequence.cpp
vmime/messageParser.cpp
vmime/misc/importanceHelper.cpp
vmime/net/defaultConnectionInfos.cpp
vmime/net/defaultTimeoutHandler.cpp
vmime/net/events.cpp
vmime/net/fetchAttributes.cpp
vmime/net/folder.cpp
vmime/net/folderAttributes.cpp
vmime/net/imap/IMAPCommand.cpp
vmime/net/imap/IMAPConnection.cpp
vmime/net/imap/IMAPFolder.cpp
vmime/net/imap/IMAPFolderStatus.cpp
vmime/net/imap/IMAPMessage.cpp
vmime/net/imap/IMAPMessagePart.cpp
vmime/net/imap/IMAPMessagePartContentHandler.cpp
vmime/net/imap/IMAPMessageStructure.cpp
vmime/net/imap/IMAPServiceInfos.cpp
vmime/net/imap/IMAPSStore.cpp
vmime/net/imap/IMAPStore.cpp
vmime/net/imap/IMAPTag.cpp
vmime/net/imap/IMAPUtils.cpp
vmime/net/maildir/format/courierMaildirFormat.cpp
vmime/net/maildir/format/kmailMaildirFormat.cpp
vmime/net/maildir/maildirFolder.cpp
vmime/net/maildir/maildirFolderStatus.cpp
vmime/net/maildir/maildirFormat.cpp
vmime/net/maildir/maildirMessage.cpp
vmime/net/maildir/maildirMessagePart.cpp
vmime/net/maildir/maildirMessageStructure.cpp
vmime/net/maildir/maildirServiceInfos.cpp
vmime/net/maildir/maildirStore.cpp
vmime/net/maildir/maildirUtils.cpp
vmime/net/message.cpp
vmime/net/messageSet.cpp
vmime/net/pop3/POP3Command.cpp
vmime/net/pop3/POP3Connection.cpp
vmime/net/pop3/POP3Folder.cpp
vmime/net/pop3/POP3FolderStatus.cpp
vmime/net/pop3/POP3Message.cpp
vmime/net/pop3/POP3Response.cpp
vmime/net/pop3/POP3ServiceInfos.cpp
vmime/net/pop3/POP3SStore.cpp
vmime/net/pop3/POP3Store.cpp
vmime/net/pop3/POP3Utils.cpp
vmime/net/sendmail/sendmailServiceInfos.cpp
vmime/net/sendmail/sendmailTransport.cpp
vmime/net/service.cpp
vmime/net/serviceFactory.cpp
vmime/net/serviceInfos.cpp
vmime/net/session.cpp
vmime/net/smtp/SMTPChunkingOutputStreamAdapter.cpp
vmime/net/smtp/SMTPCommand.cpp
vmime/net/smtp/SMTPCommandSet.cpp
vmime/net/smtp/SMTPConnection.cpp
vmime/net/smtp/SMTPExceptions.cpp
vmime/net/smtp/SMTPResponse.cpp
vmime/net/smtp/SMTPServiceInfos.cpp
vmime/net/smtp/SMTPSTransport.cpp
vmime/net/smtp/SMTPTransport.cpp
vmime/net/tls/gnutls/TLSProperties_GnuTLS.cpp
vmime/net/tls/gnutls/TLSSession_GnuTLS.cpp
vmime/net/tls/gnutls/TLSSocket_GnuTLS.cpp
vmime/net/tls/openssl/OpenSSLInitializer.cpp
vmime/net/tls/openssl/TLSProperties_OpenSSL.cpp
vmime/net/tls/openssl/TLSSession_OpenSSL.cpp
vmime/net/tls/openssl/TLSSocket_OpenSSL.cpp
vmime/net/tls/TLSProperties.cpp
vmime/net/tls/TLSSecuredConnectionInfos.cpp
vmime/net/tls/TLSSession.cpp
vmime/net/tls/TLSSocket.cpp
vmime/net/tracer.cpp
vmime/net/transport.cpp
vmime/object.cpp
vmime/parameter.cpp
vmime/parameterizedHeaderField.cpp
vmime/parsedMessageAttachment.cpp
vmime/parsingContext.cpp
vmime/path.cpp
vmime/plainTextPart.cpp
vmime/platform.cpp
vmime/platforms/posix/posixChildProcess.cpp
vmime/platforms/posix/posixCriticalSection.cpp
vmime/platforms/posix/posixFile.cpp
vmime/platforms/posix/posixHandler.cpp
vmime/platforms/posix/posixSocket.cpp
vmime/platforms/windows/windowsCriticalSection.cpp
vmime/platforms/windows/windowsFile.cpp
vmime/platforms/windows/windowsHandler.cpp
vmime/platforms/windows/windowsSocket.cpp
vmime/propertySet.cpp
vmime/relay.cpp
vmime/security/cert/certificateChain.cpp
vmime/security/cert/defaultCertificateVerifier.cpp
vmime/security/cert/gnutls/X509Certificate_GnuTLS.cpp
vmime/security/cert/openssl/X509Certificate_OpenSSL.cpp
vmime/security/cert/X509Certificate.cpp
vmime/security/defaultAuthenticator.cpp
vmime/security/digest/md5/md5MessageDigest.cpp
vmime/security/digest/messageDigest.cpp
vmime/security/digest/messageDigestFactory.cpp
vmime/security/digest/sha1/sha1MessageDigest.cpp
vmime/security/sasl/builtinSASLMechanism.cpp
vmime/security/sasl/defaultSASLAuthenticator.cpp
vmime/security/sasl/SASLContext.cpp
vmime/security/sasl/SASLMechanismFactory.cpp
vmime/security/sasl/SASLSession.cpp
vmime/security/sasl/SASLSocket.cpp
vmime/streamContentHandler.cpp
vmime/stringContentHandler.cpp
vmime/text.cpp
vmime/textPartFactory.cpp
vmime/utility/datetimeUtils.cpp
vmime/utility/encoder/b64Encoder.cpp
vmime/utility/encoder/binaryEncoder.cpp
vmime/utility/encoder/eightBitEncoder.cpp
vmime/utility/encoder/encoder.cpp
vmime/utility/encoder/encoderFactory.cpp
vmime/utility/encoder/noopEncoder.cpp
vmime/utility/encoder/qpEncoder.cpp
vmime/utility/encoder/sevenBitEncoder.cpp
vmime/utility/encoder/uuEncoder.cpp
vmime/utility/filteredStream.cpp
vmime/utility/inputStream.cpp
vmime/utility/inputStreamAdapter.cpp
vmime/utility/inputStreamByteBufferAdapter.cpp
vmime/utility/inputStreamPointerAdapter.cpp
vmime/utility/inputStreamSocketAdapter.cpp
vmime/utility/inputStreamStringAdapter.cpp
vmime/utility/inputStreamStringProxyAdapter.cpp
vmime/utility/outputStream.cpp
vmime/utility/outputStreamAdapter.cpp
vmime/utility/outputStreamByteArrayAdapter.cpp
vmime/utility/outputStreamSocketAdapter.cpp
vmime/utility/outputStreamStringAdapter.cpp
vmime/utility/parserInputStreamAdapter.cpp
vmime/utility/path.cpp
vmime/utility/progressListener.cpp
vmime/utility/random.cpp
vmime/utility/seekableInputStreamRegionAdapter.cpp
vmime/utility/stream.cpp
vmime/utility/streamUtils.cpp
vmime/utility/stringProxy.cpp
vmime/utility/stringUtils.cpp
vmime/utility/sync/criticalSection.cpp
vmime/utility/url.cpp
vmime/utility/urlUtils.cpp
vmime/word.cpp
vmime/wordEncoder.cpp
)


#ADD_CONVENIENCE_LIBRARY(vmime ${LIBVMINE_SOURCES} STATIC ${SSL_LIBRARIES}  ${ICU_LIBRARY}  ${ICONV_LIBRARIES}  ${GSASL_LIBRARIES}  ${GNUTLS_LIBRARIES})


# Solaris needs to link some network libraries
#TARGET_LINK_LIBRARIES(vmime ${SSL_LIBRARIES} ${ICONV_LIBRARIES} ${ICU_LIBRARY} ${GSASL_LIBRARIES}  )

##TARGET_LINK_LIBRARIES(vmime ${TARGET_LINK_LIBRARIES} ${SSL_LIBRARIES} ${GSASL_LIBRARIES}  )


LIST(APPEND VMIME_LIBRARY_GENERATED_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/vmime/config.hpp")








SET(HTTP_MONITOR_SOURCES mongoose.c http_monitor.cc sender_thread.cc http_thread.cc
    url_base.cc url_http.cc url_smtp.cc url_mysql.cc utils.cc sys_tbl.cc )


ADD_DEFINITIONS(${SSL_DEFINES})
ADD_DEFINITIONS(-DMYSQL_SUPPORT)
ADD_DEFINITIONS(-DVMIME_TLS_SUPPORT_LIB_IS_OPENSSL=ON)
ADD_DEFINITIONS(-DVMIME_TLS_SUPPORT_LIB_IS_GNUTLS=OFF)

#ADD_DEFINITIONS(-DDMONGOOSE_USE_POST_SIZE_LIMIT=65000)
#ADD_DEFINITIONS(-DMONGOOSE_USE_STACK_SIZE=128000)

add_definitions(-DMYSQL_SUPPORT)
IF(NOT UNIX)
    INCLUDE_DIRECTORIES("../../include/mysql")
ENDIF(NOT UNIX)

CHECK_INCLUDE_FILES (netdb.h HAVE_NETDB_H)
IF(HAVE_NETDB_H)
  ADD_DEFINITIONS(-DHAVE_NETDB_H)
ENDIF(HAVE_NETDB_H)

IF(WIN32)
  MYSQL_ADD_PLUGIN(HTTP_MONITOR ${HTTP_MONITOR_SOURCES}
    LINK_LIBRARIES ${SSL_LIBRARIES} ${CURL_LIBRARIES} mysqlclient vmime )
ELSE(WIN32)
  MYSQL_ADD_PLUGIN(HTTP_MONITOR ${HTTP_MONITOR_SOURCES}  ${LIBVMINE_SOURCES}
     LINK_LIBRARIES ${SSL_LIBRARIES}  ${GSASL_LIBRARIES} ${ICONV_LIBRARIES} ${ICU_LIBRARY}  ${CURL_LIBRARIES}  ${GNUTLS_LIBRARIES}   STATIC_ONLY DEFAULT   )


ENDIF(WIN32)


##############################################################################
# Messaging features

# Module
OPTION(
	VMIME_HAVE_MESSAGING_FEATURES
	"Enable messaging features (connection to IMAP, POP3, SMTP...)"
	ON
)

# Protocols
OPTION(
	VMIME_HAVE_MESSAGING_PROTO_POP3
	"Enable POP3 protocol"
	OFF
)

OPTION(
	VMIME_HAVE_MESSAGING_PROTO_SMTP
	"Enable SMTP protocol"
	ON
)

OPTION(
	VMIME_HAVE_MESSAGING_PROTO_IMAP
	"Enable IMAP protocol"
	OFF
)

OPTION(
	VMIME_HAVE_MESSAGING_PROTO_MAILDIR
	"Enable Maildir protocol"
	OFF
)

OPTION(
	VMIME_HAVE_MESSAGING_PROTO_SENDMAIL
	"Enable Sendmail protocol"
	ON
)

OPTION(
	VMIME_HAVE_FILESYSTEM_FEATURES
	"Enable file-system features (required for file attachments and Maildir)"
	OFF
)
