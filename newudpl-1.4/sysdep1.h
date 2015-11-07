#ifndef _SYSDEP1_H_
#define _SYSDEP1_H_

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <signal.h>             /* SIGINT */

#define _POSIX_SOURCE 1

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <time.h>               /* time() */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 255
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef HAVE_SOCKLEN_T
typedef unsigned int   socklen_t;
#endif
#ifndef HAVE_IN_ADDR_T
typedef unsigned long  in_addr_t;
#endif
#ifndef HAVE_IN_PORT_T
typedef unsigned short in_port_t;
#endif
#ifndef HAVE_UINT8_T
typedef unsigned char  uint8_t;
#endif
#ifndef HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif
#ifndef HAVE_UINT32_T
typedef unsigned long  uint32_t;
#endif

#endif                          /* _SYSDEP1_H_ */
