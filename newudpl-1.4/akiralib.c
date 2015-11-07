/***************************************************************************
 *  Utility functions C library
 *  akiralib.c
 *
 *  by Akira Tsukamoto
 ***************************************************************************/

#include <stdio.h>              /* perror() */
#include <stdlib.h>
#include <string.h>             /* memset() */
#include <netinet/in.h>         /* in_port_t */
#include <sys/socket.h>         /* socket() */
#include <sys/utsname.h>        /* struct utsname */
#include <netdb.h>              /* gethostbyname() */
#include <arpa/inet.h>          /* inet_ntoa() */
#include "akiralib.h"

//extern        char    _aki_1024_[1024];

/***************************************************************************
 *** str2l() ***
 ** extended version of ANSI strtol() **
 ** ANSI strtol() will return -1 if the string contains 0 value
 ** but str2l() will return 0
 * -> const char *string  : string to convert
 *    int radix           : radix, see ANSI C or POSIX strtol() for details
 *    long min            : limit of minimum value
 *    long max            : limit of maximum value
 * <- long *longInt       : result value
 *    int str2l()         : OK: 0  error: -1
 ***************************************************************************/
int str2l(long *result, const char *string, int radix, long min, long max)
{
  long lInt;
  char *endptr;

  /* Since ANSI C strtol() returns 0 for error
     chack whether string contains 0 for the value. */

  if (result == NULL)
    return -1;
  if (string == NULL)
    return -1;
  if (radix < 0 || 36 < radix || radix == 1)
    return -1;
  if (min > max)
    return -1;

  lInt = strtol(string, &endptr, radix);

  /*  Check if the sting was 0 */
  if (lInt == 0) {
    if (string == endptr)
      return -1;                /* genuine error */
    if (*endptr != '\0')
      return -1;                /* Had invalid character? */
  }
  if (lInt < min || max < lInt)
    return -1;

  *result = lInt;
  return 0;
}

/***************************************************************************
 *** openUDPPort() ***
 ** open UDP port with specified port number and returns fd **
 * -> in_port_t port           : port number
 * <- int openUDPPort()        : OK: fd  error: -1
 ***************************************************************************/
int openUDPPort(in_port_t port)
{
  int sockfd;
  struct sockaddr_in sockAddr;
  const int reuse = 1;          /* SO_REUSEADDR */

  /* open socket */
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("openUDPPort(): socket()");
    return -1;
  }

  /* claer and set struct socket address */
  memset((char *) &sockAddr, 0, sizeof(sockAddr));
  sockAddr.sin_family = AF_INET;
  sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockAddr.sin_port = port;

  /* bind */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse,
             sizeof(reuse)) < 0) {
    perror("openUDPPort(): setsockopt()");
    return -1;
  }
  if (bind(sockfd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
    perror("openUDPPort(): bind()");
    return -1;
  }

  return sockfd;
}

/***************************************************************************
 *** setSockAddr() ***
 ** set internet sockaddr(struct sockaddr_in) from IP and port number **
 * -> struct in_addr *ip        : pointer to IP number(struct in_addr)
 *    in_port_t port            : port number
 * <- struct sockaddr_in *sAddr : pointer to sockAddr,  No error return
 ***************************************************************************/
void setSockAddr(struct sockaddr_in *sAddr, struct in_addr *ip, in_port_t port)
{
  memset((char *) sAddr, 0, sizeof(struct sockaddr_in));
  sAddr->sin_family = AF_INET;
  sAddr->sin_addr.s_addr = ip->s_addr;
  sAddr->sin_port = port;
}

/***************************************************************************
 *** getLHnameIp() ***
 ** get Localhost Host name and IP address **
 * -> char *hName             : pointer to string buff
 *    int len                 : size of sting buff
 * <- char *hName             : local host name terminated by '\0'
 *    in_addr_t getLHnameIp() : OK: IP address   error: -1
 ***************************************************************************/
in_addr_t getLHnameIp(char *hName, int len)
{

  /* impliment code when host name was not retrieved */
  struct hostent *hostEnt;
  struct utsname myname;
  struct in_addr *ipAddr;

  if (uname(&myname) < 0)
    return -1;
    
//  printf("localhost name: %s\n", myname.nodename);

  if ((hostEnt = gethostbyname(myname.nodename)) == NULL)
    return -1;
  ipAddr = (struct in_addr *) hostEnt->h_addr_list[0];
//  memcpy(&ipAddr, hostEnt->h_addr_list[0], hostEnt->h_length);
  if (strlen(myname.nodename) >= len)
    return -1;
  strcpy(hName, myname.nodename);

  return ipAddr->s_addr;
}

/***************************************************************************
 *** addr2str() ***
 ** convert sockaddr(struct sockaddr_in) to string
 ** if host name was retrieved
 **   sockaddr -> "www.yahoo.com(123.111.112.333)/4123"
 ** if not
 **   sockaddr -> "123.111.112.333/4123"
 * -> struct sockaddr_in *s           : sockaddr
 * <- char *addr2str(), char* string  : string,  No error return
 ***************************************************************************/
char *addr2str(char *string, int len, struct sockaddr_in *s)
{
#if 0
  struct hostent *hostEnt;

  hostEnt =
      gethostbyaddr((const char *) &s->sin_addr, sizeof(struct in_addr),
                    AF_INET);
  if (hostEnt != NULL) {
    if (ntohs(s->sin_port) != 0) {
      sprintf(_aki_1024_, "%s(%s)/%d",
              hostEnt->h_name, inet_ntoa(s->sin_addr), ntohs(s->sin_port));
    } else {
      sprintf(_aki_1024_, "%s(%s)/*****",
              hostEnt->h_name, inet_ntoa(s->sin_addr));
    }
  } else {
    if (ntohs(s->sin_port) != 0) {
      sprintf(_aki_1024_, "%s/%d", inet_ntoa(s->sin_addr), ntohs(s->sin_port));
    } else {
      sprintf(_aki_1024_, "%s/*****", inet_ntoa(s->sin_addr));
    }
  }

  if (strlen(_aki_1024_) >= len) {
    return NULL;
  }

  strcpy(string, _aki_1024_);
  return string;
#else
  struct hostent *hostEnt;
  char temp[1024];

  hostEnt =
      gethostbyaddr((const char *) &s->sin_addr, sizeof(struct in_addr),
                    AF_INET);
  if (hostEnt != NULL) {
    if (ntohs(s->sin_port) != 0) {
      sprintf(temp, "%s(%s)/%d",
              hostEnt->h_name, inet_ntoa(s->sin_addr), ntohs(s->sin_port));
    } else {
      sprintf(temp, "%s(%s)/*****",
              hostEnt->h_name, inet_ntoa(s->sin_addr));
    }
  } else {
    if (ntohs(s->sin_port) != 0) {
      sprintf(temp, "%s/%d", inet_ntoa(s->sin_addr), ntohs(s->sin_port));
    } else {
      sprintf(temp, "%s/*****", inet_ntoa(s->sin_addr));
    }
  }

  if (strlen(temp) >= len) {
    return NULL;
  }

  strcpy(string, temp);
  return string;
}
#endif

/*** addTimeval() ***/
struct timeval addTimeval(struct timeval *ret, struct timeval *a,
                          struct timeval *b)
{
  ret->tv_usec = a->tv_usec + b->tv_usec;
  if (ret->tv_usec > 1000000) {
    ret->tv_sec = a->tv_sec + b->tv_sec + 1;
    ret->tv_usec -= 1000000;
  } else {
    ret->tv_sec = a->tv_sec + b->tv_sec;
  }
  return *ret;
}                               /* addTimeval() */

/*** subTimeval() ***/
struct timeval subTimeval(struct timeval *ret, struct timeval *a,
                          struct timeval *b)
{
  if (a->tv_usec < b->tv_usec) {
    ret->tv_usec = (a->tv_usec + 1000000) - b->tv_usec;
    ret->tv_sec = a->tv_sec - b->tv_sec - 1;
  } else {
    ret->tv_usec = a->tv_usec - b->tv_usec;
    ret->tv_sec = a->tv_sec - b->tv_sec;
  }
  return *ret;
}                               /* subTimeval() */

/*** maxTimeval() ***/
struct timeval maxTimeval(struct timeval *ret, struct timeval *a,
                           struct timeval *b)
{
  if (a->tv_sec > b->tv_sec) {
    *ret = *a;
  } else if (a->tv_sec < b->tv_sec) {
    *ret = *b;
  } else {                      /* a->tv_sec == b->tv_sec */
    if (a->tv_usec > b->tv_usec) {
      *ret = *a;
    } else {
      *ret = *b;
    }
  }
  return *ret;
}                               /* maxTimeval() */

/*** tv2double() ***/
double tv2double(struct timeval tv)
{
  double d;

  d = tv.tv_usec * 0.000001;
  d += tv.tv_sec;
  return d;
}                               /* tv2double() */
