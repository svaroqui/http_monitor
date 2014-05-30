# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  CURL_LIBRARIES    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.

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
ENDIF(CURL_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/sql
                    ${CMAKE_BINARY_DIR}/pcre
                    ${CMAKE_SOURCE_DIR}/pcre
                    ${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/libmysql
                    ${CMAKE_SOURCE_DIR}/client
                    ${SSL_INCLUDE_DIRS}
                    ${CURL_INCLUDE_DIR}
                   
)

SET(HTTP_MONITOR_SOURCES mongoose.c http_monitor.cc sender_thread.cc http_thread.cc
    url_base.cc url_http.cc url_smtp.cc url_mysql.cc utils.cc sys_tbl.cc)


ADD_DEFINITIONS(${SSL_DEFINES})
ADD_DEFINITIONS(-DMYSQL_SUPPORT)
#ADD_DEFINITIONS(-DDMONGOOSE_USE_POST_SIZE_LIMIT=65000)
#ADD_DEFINITIONS(-DMONGOOSE_USE_STACK_SIZE=128000)
INCLUDE_DIRECTORIES("../../include/mysql")
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (netdb.h HAVE_NETDB_H)
IF(HAVE_NETDB_H)
  ADD_DEFINITIONS(-DHAVE_NETDB_H)
ENDIF(HAVE_NETDB_H)

IF(WIN32)
  MYSQL_ADD_PLUGIN(HTTP_MONITOR ${HTTP_MONITOR_SOURCES}
    LINK_LIBRARIES ${SSL_LIBRARIES} ${CURL_LIBRARIES} mysqlclient 
    STATIC_ONLY DEFAULT)
ELSE(WIN32)
  MYSQL_ADD_PLUGIN(HTTP_MONITOR ${HTTP_MONITOR_SOURCES}
    LINK_LIBRARIES ${SSL_LIBRARIES} ${CURL_LIBRARIES} mysqlclient)
ENDIF(WIN32)


