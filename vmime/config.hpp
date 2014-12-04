//
// This file was automatically generated by CMake.
//

#ifndef VMIME_CONFIG_HPP_INCLUDED
#define VMIME_CONFIG_HPP_INCLUDED


#include "vmime/export.hpp"


// Name of package
#define VMIME_PACKAGE "vmime"

// Version number of package
#define VMIME_VERSION "0.9.2"
#define VMIME_API "0.0.0"

// Target OS and architecture
#define VMIME_TARGET_ARCH "x86_64"
#define VMIME_TARGET_OS "Darwin"

// Set to 1 if debugging should be activated
#define VMIME_DEBUG 1

// Byte order (set one or the other, but not both!)
#define VMIME_BYTE_ORDER_BIG_ENDIAN     0
#define VMIME_BYTE_ORDER_LITTLE_ENDIAN  1

// Generic types
#define VMIME_HAVE_CSTDINT 0
#if VMIME_HAVE_CSTDINT
#	include <cstdint>
#endif

// -- 8-bit
typedef signed char vmime_int8;
typedef unsigned char vmime_uint8;
// -- 16-bit
typedef signed short vmime_int16;
typedef unsigned short vmime_uint16;
// -- 32-bit
typedef signed int vmime_int32;
typedef unsigned int vmime_uint32;
// -- 64-bit
typedef signed long vmime_int64;
typedef unsigned long vmime_uint64;

#define VMIME_HAVE_SIZE_T 1

// Charset conversion support
#define VMIME_CHARSETCONV_LIB_IS_ICONV 0
#define VMIME_CHARSETCONV_LIB_IS_ICU 1
#define VMIME_CHARSETCONV_LIB_IS_WIN 0

// Options
// -- File-system support
#define VMIME_HAVE_FILESYSTEM_FEATURES 1
// -- SASL support
#define VMIME_HAVE_SASL_SUPPORT 1
// -- TLS/SSL support
#define VMIME_HAVE_TLS_SUPPORT 1
#define VMIME_TLS_SUPPORT_LIB_IS_GNUTLS 1
#define VMIME_TLS_SUPPORT_LIB_IS_OPENSSL 0
#define VMIME_HAVE_GNUTLS_PRIORITY_FUNCS 1
// -- Messaging support
#define VMIME_HAVE_MESSAGING_FEATURES 1
// -- Messaging protocols
#define VMIME_HAVE_MESSAGING_PROTO_POP3 1
#define VMIME_HAVE_MESSAGING_PROTO_SMTP 1
#define VMIME_HAVE_MESSAGING_PROTO_IMAP 1
#define VMIME_HAVE_MESSAGING_PROTO_MAILDIR 1
#define VMIME_HAVE_MESSAGING_PROTO_SENDMAIL 1
// -- Platform-specific code
#define VMIME_PLATFORM_IS_POSIX 1
#define VMIME_PLATFORM_IS_WINDOWS 0
#define VMIME_HAVE_PTHREAD 1
#define VMIME_HAVE_GETADDRINFO 1
#define VMIME_HAVE_GETTID 0
#define VMIME_HAVE_SYSCALL 1
#define VMIME_HAVE_SYSCALL_GETTID 1
#define VMIME_HAVE_GMTIME_S 0
#define VMIME_HAVE_GMTIME_R 1
#define VMIME_HAVE_LOCALTIME_S 0
#define VMIME_HAVE_LOCALTIME_R 1
#define VMIME_HAVE_MLANG 0
#define VMIME_SHARED_PTR_USE_CXX 0
#define VMIME_SHARED_PTR_USE_BOOST 1


#define VMIME_SENDMAIL_PATH "/usr/sbin/sendmail"


#endif // VMIME_CONFIG_HPP_INCLUDED
