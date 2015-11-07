#ifndef SYSDEP_H_
#define SYSDEP_H_
/***************************************************************************
 *
 *  System Dependency file *
 *  sysdep.h
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

/** system dependency file *
 * This file is intended to hide all the differences in many platforms.
 * If your system does not support newer standards and need to
 * add typedef or functions, please add them at appropiate 
 * header section.
 *  e.g. 
 *    uint16_t should be in <stdint.h> 
 *      which was defined by C99.
 *    gettimeofdya() should be in <sys/time.h> 
 *      which was driven from BSD.
 */

/*** standard switches ***********************************************/
#if 0
#define _POSIX_SOURCE 1
#define _XOPEN_SOURCE 500
#endif

/*** config.h files **************************************************/
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_MSC_VER)
#include "config_msc.h"
#endif

#if defined(__BORLANDC__)
#include "config_bc.h"
#endif

/*** etc *************************************************************/
/* Determine if the C(++) compiler requires complete function prototype  */
#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) \
    || defined(WIN32)
#define __P(x) x
#else
#define __P(x) ()
#endif
#endif

/*** unistd.h ********************************************************/
#ifdef HAVE_WIN32STD_H
#include "win32std.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_GETOPT
#include "win32_stdc_on.h"
#include "getopt.h"
#include "win32_stdc_off.h"
#endif

/*** sys/param.h *****************************************************/
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/*** limits.h ********************************************************/
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

/*** sys/types.h *****************************************************/
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/*** stdint.h C99 ****************************************************/
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef __STDC__
#define MAX32U 0xFFFFFFFFU
#else
#define MAX32U 0xFFFFFFFF
#endif
#define MAX32  0x8FFFFFFF

#ifndef HAVE_INT8_T
typedef signed char       int8_t;  /* some system might have signed char */
typedef unsigned char     u_int8_t;
/* 16 bit machines */
#if UINT_MAX == 0xFFFF
typedef int               int16_t;
typedef long              int32_t;
typedef unsigned int      u_int16_t;
typedef unsigned long     u_int32_t;
/* 32 bit machines */
#elif ULONG_MAX == MAX32U
typedef short             int16_t;
typedef long              int32_t;
typedef unsigned short    u_int16_t;
typedef unsigned long     u_int32_t;
#if !defined(NOLONGLONG)
typedef long long int     int64_t;
typedef unsigned long long u_int64_t;
#elif defined(WIN32)
typedef INT64             int64_t;
typedef UINT64            u_int64_t;
#endif /* NOLONGLONG */
/* 64 bit machines? */
#else
typedef short             int16_t;
typedef int               int32_t;
typedef long              int64_t;
typedef unsigned short    u_int16_t;
typedef unsigned int      u_int32_t;
typedef unsigned long     u_int64_t;
#endif /* UINT_MAX */
#endif /* HAVE_INT8_T */

#ifndef HAVE_UINT8_T
typedef u_int8_t  uint8_t;
#endif
#ifndef HAVE_UINT16_T
typedef u_int16_t uint16_t;
#endif
#ifndef HAVE_UINT32_T
typedef u_int32_t uint32_t;
#endif

/*** sys/utsname.h ***************************************************/
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifndef HAVE_UNAME
#include "uname.h"
#endif

/*** sys/select.h ****************************************************/
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/*** sys/socket.h ****************************************************/
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef HAVE_SOCKLEN_T
typedef u_int32_t socklen_t;
#endif

/*** netinet/in.h ****************************************************/
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifndef HAVE_IN_ADDR_T
typedef u_int32_t in_addr_t;
#endif
#ifndef HAVE_IN_PORT_T
typedef u_int16_t in_port_t;
#endif

/*** netdb.h *********************************************************/
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

/*** arpa/inet.h *****************************************************/
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/*** signal.h ********************************************************/
#include <signal.h>

/*** sys/timeb.h *****************************************************/
#include <sys/timeb.h>

/*** sys/time.h & time.h *********************************************/
#if defined(HAVE_SYS_TIME_H) && defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif /* HAVE_SYS_TIME_H  && TIME_WITH_SYS_TIME */

#ifndef HAVE_GETTIMEOFDAY
#include "gettimeofday.h"
#endif

/*** stdlib.h ********************************************************/
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/*** string.h ********************************************************/
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*** stdio.h *********************************************************/
#include <stdio.h>

/*** stdlib.h ********************************************************/
#include <assert.h>

/*** errno.h *********************************************************/
#include <errno.h>

/*** math.h **********************************************************/
#include <math.h>

/*** rpcsvc/ypclnt.h *************************************************/
#ifdef HAVE_RPCSVC_YPCLNT_H
#include <rpcsvc/ypclnt.h>
#endif

/*** etc *************************************************************/
#ifndef HAVE_STARTUPSOCKET
#define startupSocket()
#endif

#ifndef HAVE_CLOSESOCKET
#define closesocket close
#endif

#ifndef SIGBUS
#define SIGBUS SIGINT
#endif

#ifndef SIGHUP
#define SIGHUP SIGINT
#endif

#ifndef SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef SIGKILL
#define SIGKILL SIGTERM
#endif

#ifndef __STDC__
#define __STDC__ 1
#endif

/** anything additional, put it here *********************************/

/* These are just for shutting up error message 
 * when compiling multimer.c on win32 
 */
#ifndef HAVE_SETITIMER
#ifndef  ITIMER_REAL            /* mutimer.c */
#define  ITIMER_REAL     0      /* Decrements in real time */
#endif

struct  itimerval {
        struct  timeval it_interval;    /* timer interval */
        struct  timeval it_value;       /* current value */
};
#endif /* HAVE_SETITIMER */

/* I just use this for temporary buffer */
#ifndef MAXBUFF
#define MAXBUFF 2048
#endif

#endif /* _SYSDEP_H_ */
