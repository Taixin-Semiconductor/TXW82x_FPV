#ifndef HEADER_CURL_CONFIG_TAIXIN_H
#define HEADER_CURL_CONFIG_TAIXIN_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/


/* ================================================================ */
/*           Hand crafted config file for TAIXIN SDK                */
/* ================================================================ */

#define HAVE_BOOL_T 1

#define BUILDING_LIBCURL 1

/* Define if you have the <arpa/inet.h> header file. */
/* #define HAVE_ARPA_INET_H 1 */

/* Define if you have the <fcntl.h> header file. */
//#define HAVE_FCNTL_H 1

/* Define if you have the <io.h> header file. */
//#define HAVE_IO_H 1

/* Define if you have the <locale.h> header file. */
//#define HAVE_LOCALE_H 1


/* Define if you have the <netdb.h> header file. */
/* #define HAVE_NETDB_H 1 */

/* Define if you have the <netinet/in.h> header file. */
/* #define HAVE_NETINET_IN_H 1 */

/* Define if you have the <signal.h> header file. */
//#define HAVE_SIGNAL_H 1

/* Define if you have the <ssl.h> header file. */
/* #define HAVE_SSL_H 1 */

/* Define if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define if you have the <sys/param.h> header file. */
/* #define HAVE_SYS_PARAM_H 1 */

/* Define if you have the <sys/select.h> header file. */
/* #define HAVE_SYS_SELECT_H 1 */

/* Define if you have the <sys/socket.h> header file. */
/* #define HAVE_SYS_SOCKET_H 1 */

/* Define if you have the <sys/sockio.h> header file. */
/* #define HAVE_SYS_SOCKIO_H 1 */

/* Define if you have the <sys/stat.h> header file. */
//#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file. */
/* #define HAVE_SYS_TIME_H 1 */

/* Define if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1


/* Define if you have the <termio.h> header file. */
/* #define HAVE_TERMIO_H 1 */

/* Define if you have the <termios.h> header file. */
/* #define HAVE_TERMIOS_H 1 */

/* Define if you have the <time.h> header file. */
//#define HAVE_TIME_H 1



/* ---------------------------------------------------------------- */
/*                        OTHER HEADER INFO                         */
/* ---------------------------------------------------------------- */

/* Define if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>. */
/* #define TIME_WITH_SYS_TIME 1 */

/* ---------------------------------------------------------------- */
/*                             FUNCTIONS                            */
/* ---------------------------------------------------------------- */

/* Define if you have the closesocket function. */
#define HAVE_CLOSESOCKET 1



/* Define to 1 if you have the `getpeername' function. */
#define HAVE_GETPEERNAME 1

/* Define to 1 if you have the getsockname function. */
#define HAVE_GETSOCKNAME 1

/* Define if you have the gethostname function. */
#define HAVE_GETHOSTNAME 1

/* Define if you have the gettimeofday function. */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the ioctlsocket function. */
#define HAVE_IOCTLSOCKET 1

/* Define if you have a working ioctlsocket FIONBIO function. */
#define HAVE_IOCTLSOCKET_FIONBIO 1

/* Define if you have the select function. */
#define HAVE_SELECT 1

#define HAVE_LONGLONG 1

/* Define if you have the setlocale function. */
//#define HAVE_SETLOCALE 1

/* Define if you have the setmode function. */
//#define HAVE_SETMODE 1

/* Define if you have the socket function. */
#define HAVE_SOCKET 1

/* Define if you have the strcasecmp function. */
#ifdef __MINGW32__
#define HAVE_STRCASECMP 1
#endif

/* Define if you have the strdup function. */
//#define HAVE_STRDUP 1

/* Define if you have the stricmp function. */
//#define HAVE_STRICMP 1

/* Define if you have the strtoll function. */
#if defined(__MINGW32__) || defined(__POCC__) || \
    (defined(_MSC_VER) && (_MSC_VER >= 1800))
//#define HAVE_STRTOLL 1
#endif

/* Define if you have the utime function. */
#ifndef __BORLANDC__
//#define HAVE_UTIME 1
#endif

/* Define if you have the recv function. */
#define HAVE_RECV 1

/* Define to the type of arg 1 for recv. */
#define RECV_TYPE_ARG1 SOCKET

/* Define to the type of arg 2 for recv. */
#define RECV_TYPE_ARG2 void *

/* Define to the type of arg 3 for recv. */
#define RECV_TYPE_ARG3 size_t

/* Define to the type of arg 4 for recv. */
#define RECV_TYPE_ARG4 int

/* Define to the function return type for recv. */
#define RECV_TYPE_RETV int

/* Define if you have the send function. */
#define HAVE_SEND 1

/* Define to the type of arg 1 for send. */
#define SEND_TYPE_ARG1 SOCKET

/* Define to the type qualifier of arg 2 for send. */
#define SEND_QUAL_ARG2 const

/* Define to the type of arg 2 for send. */
#define SEND_TYPE_ARG2 void *

/* Define to the type of arg 3 for send. */
#define SEND_TYPE_ARG3 size_t

/* Define to the type of arg 4 for send. */
#define SEND_TYPE_ARG4 int

/* Define to the function return type for send. */
#define SEND_TYPE_RETV int


/* Define to 1 if you have the signal function. */
//#define HAVE_SIGNAL 1

/* ---------------------------------------------------------------- */
/*                       TYPEDEF REPLACEMENTS                       */
/* ---------------------------------------------------------------- */

/* Define if in_addr_t is not an available 'typedefed' type. */
#define in_addr_t uint32_t

/* Define if ssize_t is not an available 'typedefed' type. */
#if 0 //ndef _SSIZE_T_DEFINED //taixin
#  if defined(__POCC__) || defined(__MINGW32__)
#  elif defined(_WIN64)
#    define _SSIZE_T_DEFINED
#    define ssize_t __int64
#  else
#    define _SSIZE_T_DEFINED
#    define ssize_t int
#  endif
#endif

/* ---------------------------------------------------------------- */
/*                            TYPE SIZES                            */
/* ---------------------------------------------------------------- */

/* Define to the size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* Define to the size of `long long', as computed by sizeof. */
/* #define SIZEOF_LONG_LONG 8 */

/* Define to the size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Define to the size of `size_t', as computed by sizeof. */
#if defined(_WIN64)
#  define SIZEOF_SIZE_T 8
#else
#  define SIZEOF_SIZE_T 4
#endif

/* Define to the size of `curl_off_t', as computed by sizeof. */
#define SIZEOF_CURL_OFF_T 8

/* ---------------------------------------------------------------- */
/*               BSD-style lwIP TCP/IP stack SPECIFIC               */
/* ---------------------------------------------------------------- */

/* Define to use BSD-style lwIP TCP/IP stack. */
#define USE_LWIPSOCK 1

#ifdef USE_LWIPSOCK
#  undef USE_WINSOCK
#  undef HAVE_WINSOCK2_H
#  undef HAVE_WS2TCPIP_H
#  undef HAVE_GETHOSTNAME
#  undef LWIP_POSIX_SOCKETS_IO_NAMES
#  undef RECV_TYPE_ARG1
#  undef RECV_TYPE_ARG3
#  undef SEND_TYPE_ARG1
#  undef SEND_TYPE_ARG3
#  define HAVE_FREEADDRINFO
#  define HAVE_GETADDRINFO
#  define HAVE_GETHOSTBYNAME_R
#  define HAVE_GETHOSTBYNAME_R_6
#  define LWIP_POSIX_SOCKETS_IO_NAMES 0
#  define RECV_TYPE_ARG1 int
#  define RECV_TYPE_ARG3 size_t
#  define SEND_TYPE_ARG1 int
#  define SEND_TYPE_ARG3 size_t
#endif

/* ---------------------------------------------------------------- */
/*                          STRUCT RELATED                          */
/* ---------------------------------------------------------------- */

/* Define if you have struct sockaddr_storage. */
#if !defined(__SALFORDC__) && !defined(__BORLANDC__)
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#endif

/* Define if you have struct timeval. */
#define HAVE_STRUCT_TIMEVAL 1

/* Define if struct sockaddr_in6 has the sin6_scope_id member. */
#define HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID 1

#define CURL_OS "TAIXIN-OSAL"

/* Name of package */
#define PACKAGE "curl"

/* If you want to build curl with the built-in manual */
#define USE_MANUAL 1

#if defined(__POCC__) || defined(USE_IPV6)
#  define ENABLE_IPV6 1
#endif

/////////////////////////////////////////////////////////////////
/***************************************************************/
/*      Disable or Enable some features                       */
/***************************************************************/
#define CURL_DISABLE_MIME
#define CURL_DISABLE_HSTS
#define CURL_DISABLE_DOH
#define CURL_DISABLE_PROXY
#define CURL_DISABLE_DICT
#define CURL_DISABLE_FILE
#define CURL_DISABLE_FTP
#define CURL_DISABLE_GOPHER
#define CURL_DISABLE_IMAP
#define CURL_DISABLE_LDAP
#define CURL_DISABLE_LDAPS
#define CURL_DISABLE_MQTT
#define CURL_DISABLE_POP3
#define CURL_DISABLE_RTSP
#define CURL_DISABLE_SMB
#define CURL_DISABLE_SMTP
#define CURL_DISABLE_TELNET
#define CURL_DISABLE_TFTP
#define CURL_DISABLE_COOKIES
#define CURL_DISABLE_SOCKETPAIR
#define CURL_DISABLE_ALTSVC
#define CURL_DISABLE_VERBOSE_STRINGS
#define CURL_DISABLE_DEPRECATION
#define CURL_DISABLE_GETOPTIONS
#define CURL_DISABLE_SHUFFLE_DNS
#define CURL_DISABLE_NETRC
#define CURL_DISABLE_PARSEDATE
#define CURL_DISABLE_PROGRESS_METER
#define CURL_DISABLE_NTLM
#define CURL_DISABLE_DIGEST_AUTH
#define CURL_DISABLE_FORM_API

//#define CURL_DISABLE_HTTP
//#define CURL_DISABLE_CRYPTO_AUTH
//#define CURL_DISABLE_HTTP_AUTH

#define USE_MBEDTLS
//#define USE_OPENSSL
//#define USE_WOLFSSL
//#define USE_GNUTLS

#endif
