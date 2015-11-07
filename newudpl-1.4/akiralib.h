#ifndef _AKIRA_LIB_H_
#define _AKIRA_LIB_H_
/***************************************************************************
 *  Utility functions C library
 *  akiralib.h
 *
 *  by Akira Tsukamoto
 ***************************************************************************/

#include "sysdep1.h"

/*** global Variables ******************************************************/

/* extern char _aki_1024_[1024]; */

/** string to long. 
 * extended version of ANSI strtol() 
 * ANSI strtol() will return -1 if the string contains 0 value
 * but str2l() will return 0 */
int str2l(long *result, const char *string, int radix, long min, long max);

/** open UDP Port.
 * open UDP port with specified port number and returns fd */
int openUDPPort(in_port_t port);

/** set sockaddr.
 * set internet socaddr(struct sockaddr_in) from IP and port number */
void setSockAddr(struct sockaddr_in *sAddr, struct in_addr *ip, in_port_t port);

/** get local hostname and ip number. */
in_addr_t getLHnameIp(char *hName, int len);

/** sockaddr to string.
 * convert sockaddr(struct sockaddr_in) to string
 * if host name was retrieved
 *   sockaddr -> www.suna-asobi.com(345.455.112.333):4123
 * if not
 *   sockaddr -> 345.455.112.333:4123  */
char *addr2str(char *string, int len, struct sockaddr_in *s);

/** add timeval a + b */
struct timeval addTimeval(struct timeval *ret, struct timeval *a,
                          struct timeval *b);

/** sub timeval a - b */
struct timeval subTimeval(struct timeval *ret, struct timeval *a,
                          struct timeval *b);

/** max timeval.
 * select the timeval whichever is larger */
struct timeval maxTimeval(struct timeval *ret, struct timeval *a,
                           struct timeval *b);

/** tv to double.
 * convert struct_timval to double **/
double tv2double(struct timeval tv);

#endif                          /* _AKIRA_LIB_H_ */
