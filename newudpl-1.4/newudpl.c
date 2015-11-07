/***************************************************************************
 *
 *  Network Emulator With UDP Link
 *  newudpl.c
 *
 *  Copyright (c) 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#include "sysdep1.h"
#include "sysdep.h"
#include "notify.h"
#include "multimer.h"
#include "host2ip.h"
#include "queue.h"
#include "akiralib.h"
#include "akiramath.h"
#include "newudpl.h"

//#define DEBUG        1

/** string buffer size, no less then 128 */
#define STRSIZE      1024
/** buffer for sendto() recvfrom() sockets.
 * don't forget to chage MAX_QBUFSIZE at the same time */
#define BUFSIZE      16384
/** UDP segment size, no more then 2048 (not used)*/
#define MAX_UDP      1500
/** maximum delay time */
#define MAX_DELAYSEC 10
/** = 50 (Mbps) */
#define MAX_KBPS     50000
/** maximum queue buffer size */
#define MAX_QBUFSIZE 16384
/** denominator of the bit error rate */
#define BITERRDENOM  100000

/*** global Variables ******************************************************/

/** getopt() */
extern char *optarg;
/** getopt() */
extern int optind;

/** probability of Gilbert over all packet loss */
float gilOverAllLoss;

/*** private Variables *****************************************************/

int debug = 0;

/** counter of received UDP bytes, ID for notify() */
uint32_t rUdpC = 0;         
/** counter of send UDP bytes */
uint32_t sUdpC = 0;         

/** pointers to handle UDP packets queue */
UdpQueue uq;                    

/** struct to hold statistics */
Statistics g_stat;              
/** ran1() */
long idum;                      

/*** private functions prototyp for mian() ********************************/

int initOptions(Options * o, char *arg0);
int parseCmdLine(Options * o, int argc, char *argv[]);

int init(Options * options);
int destroy(void);

int printTitle(Options * options);
int printStatistics();

Notify_value readHandler(Notify_client client, int dummeyfd);
Notify_value sendHandler(Notify_client client);
Notify_value sigHandler(Notify_client client, int sig, Notify_signal_mode when);
Notify_value stdinHandler(Notify_client client, int dummeyfd);

/***************************************************************************
 *** Private function definition ***
 ***************************************************************************/

/** initialize options.
 * set the default value to struct Options */
int initOptions(Options * o, char *argv0)
{
  struct hostent *host;
  struct in_addr *local;
  char   *p;

  if ( (p = strrchr(argv0, '/')) != NULL) {
     argv0 = ++p;
  }

  /* get local IP address for default srce/dest host */
  if ((host = gethostbyname("localhost")) == NULL) {
    perror("initOptions(): gethostbyname()");
    return -1;
  }
  local = (struct in_addr *) host->h_addr_list[0];

  /* default values for all the parameters */
  strncpy(o->argv0, argv0, NAME_MAX - 1);
  o->argv0[NAME_MAX - 1] = '\0';
  o->readPort     = htons(41192);
  o->writPort     = htons(41193);                                 /* -p */
  o->srceHostIpN  = *local;
  o->srceHostPort = htons(41191);                                 /* -i */
  o->destHostIpN  = *local;
  o->destHostPort = htons(41194);                                 /* -o */
  o->speed        = 1000;       /* = 1(Mb/s) 1 - MAX_KBPS(kb/s) *//* -s */
  o->ether        = 10000;      /* = 10(Mb/s)  10M, 100M, 1G    *//* -e */
  o->inQBufSize   = 8192;       /* 1 - MAX_QBUFSIZE(bytes) */     /* -q */
  o->delay.tv_sec = 0;          /* 0 - MAX_DELAYSEC(sec) */       /* -d */
  o->delay.tv_usec= 0;          /* 000000 - 999999(usec) */       /* -d */
  o->randomLoss   = 0;          /* 0 - 99(%) */                   /* -L */
  o->gilCondLoss  = 0.0;        /* 0.0 - 1.0 */                   /* -C */
  o->gilUncondLoss= 0.0;        /* 0.0 - 1.0 */                   /* -U */
  o->bitError     = 0;          /* 0 - 99(1/BITERRDENOM) */       /* -B */
  o->outOfOrder   = 0;          /* 0 - 99(%) */                   /* -O */
  o->calcMode     = 0;
  o->verbose      = 0;
  o->rand_short   = 0;

  return 0;
} /* initOptions() */

/** initialize statistics.
 * set the default value to struct g_stat */
void initStat(void)
{
  struct timeval now_tv;

  gettimeofday(&now_tv, NULL);

  g_stat.startime.tv_sec  = 0;
  g_stat.startime.tv_usec = 0;
  g_stat.lasttime.tv_sec  = 0;
  g_stat.lasttime.tv_usec = 0;
  g_stat.prvRcv_tv.tv_sec = 0;
  g_stat.prvRcv_tv.tv_usec= 0;
  g_stat.prvIntervled_tv  = now_tv;
  g_stat.minSp            = 0;
  g_stat.maxSp            = 0;
  g_stat.rPacket          = 0;
  g_stat.sPacket          = 0;
  g_stat.rejPack          = 0;
  g_stat.rejBytes         = 0;
  g_stat.congPack         = 0;
  g_stat.congBytes        = 0;
  g_stat.randomLoss       = 0;
  g_stat.biterror         = 0;
  g_stat.outOfOrder       = 0;
} /* initStat() */

/***************************************************************************/

/** get port numbers.
 * parse the commandline string and set readPort and writePort */
int getPorts(in_port_t * readPort, in_port_t * writePort, char *optarg)
{
  char *pStr, buff[STRSIZE];
  long port;

  /* Copy to temporary buffer because strtok() will destroy the string */
  if (strlen(optarg) >= STRSIZE) {
    fprintf(stderr, "getPorts(): String is too large: %s\n", optarg);
    return -1;
  }
  strcpy(buff, optarg);

  pStr = strtok(buff, ":");     /* first token (readPort) */
  if (*optarg == ':')
    goto getPorts_NEXTPORT;
  if (pStr != NULL) {
    if (str2l(&port, pStr, 0, 0x400, 0xFFFF) == -1)
      return -1;
    *readPort = htons(port);
  } else {
    fprintf(stderr, "Invalid port number: %s\n", optarg);
    return -1;
  }

  pStr = strtok(NULL, ":");     /* second token (writePort) */
  if (pStr != NULL) {
    getPorts_NEXTPORT:
    if (str2l(&port, pStr, 0, 0x400, 0xFFFF) == -1)
      return -1;
    *writePort = htons(port);
  }

  if (debug)
    printf("option: IP Port: %d:%d\n", *readPort, *writePort);
  return 0;
} /* getPorts() */

/** get host name and port number.
 * parse the commandline string and set hostIpN and hostPort */
int getHost(struct in_addr *hostIpN, in_port_t * hostPort, char *optarg)
{
  char *pStr, buff[STRSIZE];
  struct in_addr ip;
  long port;

  if (strlen(optarg) >= STRSIZE) {
    fprintf(stderr, "String is too large: %s\n", optarg);
    return -1;
  }
  strcpy(buff, optarg);

  pStr = strtok(buff, ":/");    /* first token (hostIpN) */
  if (pStr != NULL) {
    ip = host2ip(pStr);
    if (ip.s_addr == INADDR_ANY)
      return -1;
    *hostIpN = ip;
  }

  pStr = strtok(NULL, ":/");    /* second token (hostPort) */
  if (pStr != NULL) {
    if (*pStr == '*') {
      *hostPort = 0;
    } else if (str2l(&port, pStr, 0, 0, 0xFFFF) == -1)
      return -1;
    else
      *hostPort = htons(port);
  }

  if (debug)
    printf("option: IP and Port: %s(0x%lX):%d\n",
           inet_ntoa(*hostIpN), (unsigned long)hostIpN->s_addr, *hostPort);
  return 0;
} /* getHost() */

/** get second.
 * parse the commandline string and set struct timeval *s */
int getSec(struct timeval *s, char *optarg, long maxSec)
{
  char *pStr, buff[STRSIZE], tmp[STRSIZE];
  long sec, usec;

  if (strlen(optarg) >= STRSIZE) {
    fprintf(stderr, "String is too large: %s\n", optarg);
    return -1;
  }
  strcpy(buff, optarg);

  pStr = strtok(buff, ".");     /* fisrt token (sec) */
  if (pStr != NULL) {
    if (str2l(&sec, pStr, 10, 0, maxSec) == -1)
      return -1;
    s->tv_sec = sec;
  }

  pStr = strtok(NULL, ".");     /* secong token (usec) */
  if (pStr != NULL) {
    /* memcpy("000000", "12") = "120000" */
    strcpy(tmp, "000000");
    memcpy(tmp, pStr, strlen(pStr) * sizeof(char));
    if (str2l(&usec, tmp, 10, 0, 999999) == -1)
      return -1;
    s->tv_usec = usec;
  }

  if (debug)
    printf("option: timval: %ld.%06ld\n", sec, usec);
  return 0;
} /* getSec() */

/** get ether-net speed.
 * parse the commandline string and set ether-net speed */
int getEther(int *ether, char *optarg)
{
  long l;

  if (str2l(&l, optarg, 10, 1, 3) == -1)
    return -1;

  switch (l) {
  case 1:                       /* 10M(b/s) ether-net */
    *ether = 10000;
    break;
  case 2:                       /* 100M(b/s) ether-net */
    *ether = 100000;
    break;
  case 3:                       /* 1G(b/s) ether-net */
    *ether = 1000000;
    break;
  default:
    return -1;
  }

  return 0;
} /* getEther() */

/** usage.
 * Just prints out the usage */
void usage(char *argv0)
{
  fprintf(stdout, "\
Usage: %s [-[v|vv]] [-p [recv_port]:[send_port]]\n \
  [-i sorce_host[[:|/][port|*]]] [-o dest_host[[:|/]port]]\n \
  [-s link_speed] [-d delay] [-e Ethernet_speed] [-q queue_buf_size]\n \
  [-L random_packet_loss_rate] [-B bit_error_rate] [-O out_of_order_rate]\n",
  argv0);
} /* usage() */

/** parse command line.
 * this is the main getopt() routine */
int parseCmdLine(Options * o, int argc, char *argv[])
{
  int c;
  int errF = 0;
  long l;
  double d;

  while ( (c = getopt(argc, argv, "p:i:o:d:s:e:q:B:L:O:C:U:mvhz")) != EOF) {
    switch (c) {
    case 'p':
      if (getPorts(&o->readPort, &o->writPort, optarg) == -1)
        errF = -1;
      break;
    case 'i':
      if (getHost(&o->srceHostIpN, &o->srceHostPort, optarg) == -1)
        errF = -1;
      break;
    case 'o':
      if (getHost(&o->destHostIpN, &o->destHostPort, optarg) == -1)
        errF = -1;
      break;
    case 'd':
      if (getSec(&o->delay, optarg, MAX_DELAYSEC) == -1)
        errF = -1;
      break;
    case 's':
      if (str2l(&l, optarg, 10, 1, MAX_KBPS) == -1)
        errF = -1;
      else
        o->speed = (uint32_t)l;
      break;
    case 'e':
      if (getEther(&o->ether, optarg) == -1)
        errF = -1;
      break;
    case 'q':
      if (str2l(&l, optarg, 0, 1, MAX_QBUFSIZE) == -1)
        errF = -1;
      else
        o->inQBufSize = (int)l;
      break;
    case 'B':
      if (str2l(&l, optarg, 10, 0, BITERRDENOM - 1) == -1)
        errF = -1;
      else
        o->bitError = (int)l;
      break;
    case 'L':
      if (str2l(&l, optarg, 10, 0, 99) == -1)
        errF = -1;
      else
        o->randomLoss = (int)l;
      break;
    case 'C':
      if ( (d = strtod(optarg, NULL)) == 0)
        errF = -1;
      else
        o->gilCondLoss = (float)d;
      break;
    case 'U':
      if ( (d = strtod(optarg, NULL)) == 0)
        errF = -1;
      else
        o->gilUncondLoss = (float)d;
      break;
    case 'O':
      if (str2l(&l, optarg, 10, 0, 99) == -1)
        errF = -1;
      else
        o->outOfOrder = (int)l;
      break;
    case 'm':
      o->calcMode = 1;
      break;
    case 'v':
      o->verbose++;
      break;
    case '?':
    case 'h':
      usage(o->argv0);
      exit(1);
      break;
    case 'z':                   /* for debugging */
      debug = 1;
      break;
    }                           /* switch */

    if (errF == -1) {
      usage(o->argv0);
      fprintf(stderr, "%s: Invalid option -%c %s\n", o->argv0, c, optarg);
      exit(1);
    }
  }                             /* while */

  if (optind != argc) {
    usage(o->argv0);
    exit(1);
  }

  return 0;
} /* parseCmdLine() */

/***************************************************************************/
/** initialize.
 * constructor for main().
 * this will prepare all the intial setup for main()
 * opening UDP port, creating queue end etc */
int init(Options * options)
{
  Options *o = options;
  time_t t;

  uq.o = options;
  initStat();

  /* initial random functions */
  idum = -(time(NULL));
  ran1(&idum);
  srand((unsigned int) time(&t));
  gilOverAllLoss = (o->gilUncondLoss * (1 - o->gilCondLoss))
                    / (1 - o->gilUncondLoss);

  /* evaluate limit of RAND_MAX */
  if ((ULONG_MAX / 100) > RAND_MAX)
     o->rand_short = 1;     /* assume RAND_MAX is not long */

  /** Open UDP **/
//  startupSocket();        /* for WinNT */

  /* read port */
  if ((uq.readfd = openUDPPort(o->readPort)) < 0) {
    fprintf(stderr, "%s: Error open UDP port: %d.\n", o->argv0, o->readPort);
    return -1;
  }
  /* write port */
  if ((uq.writfd = openUDPPort(o->writPort)) < 0) {
    fprintf(stderr, "%s: Error open UDP port: %d.\n", o->argv0, o->writPort);
    return -1;
  }

  /** Create Queue **/
  if ((uq.queue = openQueue()) == NULL) {
    fprintf(stderr, "%s: Could not create Queue.\n", o->argv0);
    return -1;
  }
  uq.qBufSize = o->inQBufSize;
  uq.qUsed = 0;

  return 0;
} /* init() */

/** destroy.
 * destructor for main().
 * this will close all the setup for main() 
 * closing UDP port, queue */
int destroy(void)
{
  if (close(uq.readfd) < 0) {
    perror("close(uq.readfd)");
    return -1;
  }
  if (close(uq.writfd) < 0) {
    perror("close(uq.writfd)");
    return -1;
  }
  closeQueue(uq.queue);
  return 0;
} /* destroy() */

/***************************************************************************/
/** print title.
 * Just print out the title */
int printTitle(Options * options)
{
  Options *o = options;
  char localHname[STRSIZE], sHost[STRSIZE], dHost[STRSIZE];
  int strsize = STRSIZE;
  struct in_addr localH;
  in_addr_t localHIp;
  struct sockaddr_in s, d;

  s.sin_addr = o->srceHostIpN;
  s.sin_port = o->srceHostPort;
  d.sin_addr = o->destHostIpN;
  d.sin_port = o->destHostPort;

  if ((localHIp = getLHnameIp(localHname, STRSIZE)) == -1) {
    fprintf(stderr, "%s: Could not retrieve localhost IP.\n", o->argv0);
    return -1;
  }
  localH.s_addr = localHIp;

  putchar('\n');
  puts  ("Network Emulator With UDP Link");
  puts  (" Copyright (c) 2001 by Columbia University; all rights reserved");
  putchar('\n');
  puts  ("Link established:");
  printf("  %s ->\n", addr2str(sHost, strsize, &s));
  printf("          %s(%s)/%d\n", localHname, inet_ntoa(localH),
                                  ntohs(o->readPort));
  printf("  /%d ->\n", ntohs(o->writPort));
  printf("          %s\n", addr2str(dHost, strsize, &d));
  putchar('\n');
  printf("emulating speed  : %lu(kb/s)\n", (unsigned long)o->speed);
  printf("delay            : %ld.%06ld(sec)\n", 
                            o->delay.tv_sec, o->delay.tv_usec);
  printf("ether net        : %dM(b/s)\n", o->ether / 1000);
  printf("queue buffer size: %d(bytes)\n", o->inQBufSize);
  putchar('\n');
  puts  ("error rate");
  printf("    random packet loss: %d(1/100 per packet)\n", o->randomLoss);
  if (o->gilCondLoss != 0.0) {
  printf("    Gilbert packet loss:\n");
  printf("         cond: %f  uncond: %f  over all: %f\n",
           o->gilCondLoss, o->gilUncondLoss, gilOverAllLoss);
  }
  printf("    bit error         : %d(1/%d per bit)\n", o->bitError,
                                                       BITERRDENOM);
  printf("    out of order      : %d(1/100 per packet)\n", o->outOfOrder);
  putchar('\n');

  return 0;
} /* printTitle() */

/** print statistics.
 * print out all the statistics after stopping this emulator */
int printStatistics()
{
  double s;
  struct timeval tv;

  s = tv2double(subTimeval(&tv, &g_stat.lasttime, &g_stat.startime));

  putchar('\n');
  puts  ("--- Statistics ---");
  printf("total time    : %f(sec)\n", s);
  printf("      received: %d(packets)  %lu(bytes)\n",
                          g_stat.rPacket, (unsigned long)rUdpC);
  printf("      send    : %d(packets)  %lu(bytes)\n",
                          g_stat.sPacket, (unsigned long)sUdpC);
  printf("rejected from invalid host: %d(packets) %d(bytes)\n",
                                      g_stat.rejPack, g_stat.rejBytes);
  putchar('\n');
  printf("dropped by congestion: %d(packets) %d(bytes)\n", g_stat.congPack,
                                                           g_stat.congBytes);
  putchar('\n');

  if (rUdpC == 0)
    goto STATISTIC_END;
  puts  ("created error");
  printf("    random packet loss: %d(packets)  %0.2f(1/100 per packet)\n",
         g_stat.randomLoss, (g_stat.randomLoss * 100) / (float)g_stat.sPacket);
  printf("    bit error         : %d(bits)     %0.2f(1/%d per bit)\n",
         g_stat.biterror, (g_stat.biterror * BITERRDENOM) / (float)(sUdpC * 8),
         BITERRDENOM);
  printf("    out of order      : %d(packets)  %0.2f(1/100 per packet)\n",
         g_stat.outOfOrder, (g_stat.outOfOrder * 100) / (float)g_stat.sPacket);
  putchar('\n');

  puts  ("transferred speed");
  printf("    receive: min %.3f(kb/s)  max %.3f(kb/s)\n",
         g_stat.minSp / 1000.0, g_stat.maxSp / 1000.0);
  printf("    send   : average %.3f(kb/s)\n", (sUdpC * 8 / s) / 1000.0);
  putchar('\n');

  STATISTIC_END:
  return 0;
} /* printStatistics() */

/***************************************************************************/

/** increment qUsed.
 * qUsed(counter) + bytes(received bytes) */
int incQUsed(int *qUsed, int bytes, int qBufSize)
{
  if ((*qUsed + bytes) > qBufSize) {
    return -1;
  }
  *qUsed = *qUsed + bytes;
  return 0;
} /* incQUsed() */

/** check host.
 * checks the source host of the packet is a valid host */
int checkHost(struct sockaddr_in *sockAddr, struct in_addr *ip,
              in_port_t port)
{
  if (sockAddr->sin_addr.s_addr != ip->s_addr)
    return -1;
  if (port != 0) {
    if (sockAddr->sin_port != port)
      return -1;
  }

  return 0;
} /* checkHost() */

/** assemble item.
 * store the content of the packet to QItems
 * this will be a node for the queue */
QItems *assembleItem(char *rBuff, int rBytes)
{
  QItems *qItem;

  qItem = (QItems *) malloc(sizeof(QItems));
  if (qItem == NULL) {
    perror("assembleItem(): malloc()");
    return NULL;
  }
  qItem->data = (char *) malloc(sizeof(char) * rBytes);
  if (qItem->data == NULL) {
    perror("assembleItem(): malloc()");
    return NULL;
  }
  memcpy(qItem->data, rBuff, rBytes);
  qItem->bytes = rBytes;
  return qItem;
} /* assembleItem() */

/** calculate depature time.
 * make a departure time for received packet
 * very important function in this emulator */
void calcDepTime(struct timeval *dept_tv, int bytes, uint32_t speed,
                 struct timeval *delay_tv, struct timeval *now_tv)
{
  struct timeval intervl_tv   = { 0, 0 };
  struct timeval intervled_tv = { 0, 0 };
  struct timeval delayed_tv   = { 0, 0 };
  struct timeval trans_tv     = { 0, 0 };
  uint32_t trans_d = 0;   /* u_sec */

  /**
   *  Caluculate Interval time
   *  sp (kb/s)
   *  = (sp*1000 / 8) / 1000,000 (bytes/microsec)
   *  = (1 / ((sp*1000 / 8) / 1000,000)) * bytes (microsec) <- interval time
   *  = (8000 * bytes) / sp
   */
  if ( (intervl_tv.tv_usec = (8000L * bytes) / speed) > 1000000) {
    intervl_tv.tv_sec  += (intervl_tv.tv_usec / 1000000);
    intervl_tv.tv_usec %= 1000000L;
  }

  if (debug) {
    puts("  Interval time:");
    printf("    bytes: %d  link speed: %lu(kb/s)\n",
                       bytes, (unsigned long)speed);
    printf("    %ld.%06ld(sec)\n", intervl_tv.tv_sec, intervl_tv.tv_usec);
  }

  switch (uq.o->calcMode) {
  default:
  case 0:
    addTimeval(&intervled_tv, &g_stat.prvIntervled_tv, &intervl_tv);
    break;
  case 1:
    addTimeval(&intervled_tv, now_tv, &intervl_tv);
    break;
  }
  
  /**
   * Calculate Transmission Delay
   * actualy it is a differential trans-delay 
   * packet size (bits) / ether-net speed (b/s)
   * - packet size (bits) / emulating link speed (b/s)
   *  e.g ether-net speed: 10M(b/s)
   */
  
  if (uq.o->ether > speed) { 
    /* this is just a trick to not to over flow, long */
    trans_d = bytes * 8 * ((uint32_t)uq.o->ether - speed);
    if (trans_d < 4294967)
      trans_d = trans_d * 1000 / ((uint32_t)uq.o->ether * speed);
    else
      trans_d = ((trans_d / (uint32_t)uq.o->ether) * 1000 / speed);
  }
  if (trans_d > 1000000) {
    trans_tv.tv_sec  += trans_d / 1000000;
    trans_tv.tv_usec %= 1000000L;
  } else 
    trans_tv.tv_usec = trans_d;

  if (debug) {
    puts("  Transmission Delay:");
    printf("    bytes: %d  link speed: %lu(kb/s)  ether: %d\n",
                       bytes, (unsigned long)speed, uq.o->ether);
    printf("    %ld.%06ld(sec)\n", trans_tv.tv_sec, trans_tv.tv_usec);
  }

  addTimeval(&delayed_tv, now_tv, delay_tv);
  maxTimeval(&delayed_tv, &g_stat.prvIntervled_tv, &delayed_tv);
  addTimeval(dept_tv, &delayed_tv, &trans_tv);

  if (debug) {
    puts("  Delay:");
    printf("    %ld.%06ld(sec)\n", delay_tv->tv_sec, delay_tv->tv_usec);
    puts("  prev interval:");
    printf("    %ld.%06ld(sec)\n", g_stat.prvIntervled_tv.tv_sec,
                                   g_stat.prvIntervled_tv.tv_usec);
    puts("  now:");
    printf("    %ld.%06ld(sec)\n", now_tv->tv_sec, now_tv->tv_usec);
    puts("  Departure time:");
    printf("    %ld.%06ld(sec)\n", dept_tv->tv_sec, dept_tv->tv_usec);
  }

  g_stat.prvIntervled_tv = intervled_tv;
} /* calcDepTime() */

/***************************************************************************/
/** read handler.
 * writeFd handler (manupilate packet in the queue and create error
 * and send out UDP packets) */
Notify_value readHandler(Notify_client client, int dummeyfd)
{
  struct sockaddr_in readAddr;
  socklen_t readAddrLen;
  char rBuff[BUFSIZE], hname[MAXHOSTNAMELEN + 32];
  int rBytes;                   /* MAXHOSTNAMELEN in ????.h */
  QItems *qItem;
  struct timeval dept_tv, now_tv, interval_tv;
  uint32_t recvSp;

  /* read received packet */
  readAddrLen = (socklen_t) sizeof(readAddr);
  rBytes = recvfrom(uq.readfd, rBuff, sizeof(rBuff),
                    0, (struct sockaddr *) &readAddr, &readAddrLen);
  if (rBytes < 0) {
    perror("readHandler(): recvfrom()");
    notify_stop();
    return NOTIFY_IGNORED;
  }

  if (uq.o->verbose >= 2) {
    printf("received: recv counter: %lu  size: %d(bytes)\n",
                                    (unsigned long)rUdpC, rBytes);
  }

  /* check the source host, if it does not match, discard this packet */
  if (checkHost(&readAddr, &uq.o->srceHostIpN, uq.o->srceHostPort) < 0) {
    if (uq.o->verbose) {
      printf("!Packet from invalid host: %s\n",
             addr2str(hname, MAXHOSTNAMELEN + 32, &readAddr));
      printf("  discarded packet: recv counter: %lu  size: %d(bytes)\n",
             (unsigned long)rUdpC, rBytes);
    }
    g_stat.rejPack++;
    g_stat.rejBytes += rBytes;    /* received UDP bytes before discarding any */
    return NOTIFY_DONE;
  }

  /* check the packet size, compare to queueing buffer size */
  if (rBytes > uq.o->inQBufSize) {
    if (uq.o->verbose) {
      puts  ("!Packet is larger then the queueing buffer.");
      printf("  packet size: %d(bytes)  buffer size: %d(bytes)\n",
                rBytes, uq.o->inQBufSize);
      printf("  discarded packet: recv counter: %lu  size: %d(bytes)\n",
                  (unsigned long)rUdpC, rBytes);
    }
    g_stat.congPack++;
    g_stat.congBytes += rBytes;
    return NOTIFY_DONE;
  }

  gettimeofday(&now_tv, NULL);

  /* calc the receiving speed */
  /* g_stat.prvRcv_tv.tv_sec == 0 && g_stat.prvRcv_tv.tv_usec == 0 */
  if ( (g_stat.prvRcv_tv.tv_sec && g_stat.prvRcv_tv.tv_usec) == 0) {
    /* initialize */
    g_stat.prvRcv_tv = now_tv;
    g_stat.minSp = MAX_KBPS;
  } else {
    subTimeval(&interval_tv, &now_tv, &g_stat.prvRcv_tv);
    recvSp = ((unsigned) rBytes * 8 * 1000000)
              / (interval_tv.tv_sec * 1000000 + interval_tv.tv_usec);
    if (recvSp < g_stat.minSp)
      g_stat.minSp = recvSp;
    if (recvSp > g_stat.maxSp)
      g_stat.maxSp = recvSp;
    g_stat.prvRcv_tv = now_tv;
  }

  /* check congestion, if queue buffer is full, discard packet */
  if (incQUsed(&uq.qUsed, rBytes, uq.qBufSize) < 0) {
    if (uq.o->verbose) {
      puts  ("!Congestion:");
      printf("  discarded packet: recv counter: %lu  size: %d(bytes)\n",
                  (unsigned long)rUdpC, rBytes);
    }
    g_stat.congPack++;
    g_stat.congBytes += rBytes;
    return NOTIFY_DONE;       /* buffer overflowed and discard packet, return */
  }

  if (rUdpC == 0) {
    g_stat.startime = now_tv;
    if (uq.o->verbose >= 2) {
      puts("  this is the first packet:");
    } else {
      puts("Received the first packet:");
    }
  }

  /* store packet into the queue */
  qItem = assembleItem(rBuff, rBytes);
  if (addQueue(uq.queue, qItem) == NULL) {
    fprintf(stderr, "%s: readHandler(): addQueue():\n", uq.o->argv0);
  }

  /* departure time */
  calcDepTime(&dept_tv, rBytes, uq.o->speed, &uq.o->delay, &now_tv);

  rUdpC += rBytes;              /* add to read UDP counter */
  g_stat.rPacket++;

  /* Set timer handler (sending UDP packets) */
  timer_set(&dept_tv, sendHandler, (Notify_client) rUdpC, 0);

  return NOTIFY_DONE;
} /* readHandler() */

/***************************************************************************/

/** decrement qUsed.
 * qUsed(counter) - bytes(sending bytes) */
int decQUsed(int *qUsed, int bytes, int qBufSize)
{
  if (*qUsed > qBufSize)
    return -1;
  if ((*qUsed - bytes) < 0) {
    return -1;
  }
  *qUsed = *qUsed - bytes;
  return 0;
} /* decQUsed() */

/** disassemble item.
 * unpack a packet from qItem */
void disAssembleItem(char *wBuff, int *wBytes, QItems * qItem)
{
  *wBytes = qItem->bytes;
  memcpy(wBuff, qItem->data, *wBytes);

  free(qItem->data);
  free(qItem);
} /* disAssembleItem() */

/** is packet loss.
 * Check this is a packet for packet loss. If so then discard it **/
int isPacketLoss(Queue * queue, int randomLoss)
{

  if (uq.o->randomLoss != 0) {
  if (uq.o->rand_short == 1) {
    if (((long)rand() * 100) / RAND_MAX >= randomLoss)
      return 0;
  } else {
    if (((rand() / (float)RAND_MAX) * 100.0) >= (float)randomLoss)
      return 0;
  }
  }

  if (uq.o->gilCondLoss > 0.0) {
    if (gilbRand(uq.o->gilCondLoss, gilOverAllLoss) == 0)
      return 0;
  }

  return 1;
} /* isPacketLoss() */

/** is bit error.
 * Check this is a packet for bit error. If so then corrupt the packet **/
int isBitError(char *wBuff, int wBytes, int bitError)
{
  int i, seqBitNum, byteNum, bitToCrpt, corruptBits = 0;

  corruptBits += bnldev(bitError / (float) BITERRDENOM, wBytes * 8, &idum);
  if (corruptBits <= 0)
    return 0;

  for (i = 0; i < corruptBits; i++) {
    seqBitNum = rand() * wBytes * 8 / RAND_MAX;
    byteNum = seqBitNum / wBytes;
    bitToCrpt = 1 << (seqBitNum % wBytes);
    wBuff[byteNum] ^= bitToCrpt;
  }

  return corruptBits;
} /* isBitError() */

/** is out of order.
 * Check this is a packet for out of order. If so then change the order of the 
 * packet in the queue **/
int isOutOfOrder(Queue * queue, int outOfOrder)
{
  QItems *qItem;

  if (uq.o->rand_short == 1) {
    if (((long)rand() * 100) / RAND_MAX >= outOfOrder)
      return 0;
  } else {
    if (((rand() / (float)RAND_MAX) * 100.0) >= (float)outOfOrder)
      return 0;
  }

  if (countQueue(queue) < 2) {
    if (debug)
      puts("Queue is too small to make out of order");
    return 0;
  }

  if (getQueue(queue, &qItem) == NULL) {
    fprintf(stderr, "%s: isOutOfOrder(): getQueue():\n", uq.o->argv0);
  }
  if (addQueue(queue, qItem) == NULL) {
    fprintf(stderr, "%s: isOutOfOrder(): addQueue():\n", uq.o->argv0);
  }

  return 1;
} /* isOutOfOrder() */

/***************************************************************************/
/** send handler.
 * readFd handler (reading and queueing UDP packets) */
Notify_value sendHandler(Notify_client client)
{
  struct sockaddr_in writAddr;
  char wBuff[BUFSIZE];
  int wBytes, written, corruptBits;
  QItems *qItem;
  struct timeval now_tv;

  /* get a sending packet from the queue */
  if (getQueue(uq.queue, &qItem) == NULL) {
    fprintf(stderr, "%s: sendHandler(): getQueue():\n", uq.o->argv0);
  }
  disAssembleItem(wBuff, &wBytes, qItem);

  gettimeofday(&now_tv, NULL);

  /*
   * generate errors to packet
   */

  /* random packet loss */
  if (uq.o->randomLoss != 0 || uq.o->gilCondLoss > 0.0) {  
    if (isPacketLoss(uq.queue, uq.o->randomLoss)) {
      if (uq.o->verbose) {
        puts  ("Packet loss:");
        printf("  discarded packet: send counter: %lu  size: %d(bytes)\n",
                  (unsigned long)sUdpC, wBytes);
      }
      /* decrease the counter of the queue buffer utilization */
      if (decQUsed(&uq.qUsed, wBytes, uq.qBufSize) < 0) {
        fprintf(stderr, "%s: decQUsed(): uq.qUsed: %d wBytes: %d.\n",
                uq.o->argv0, uq.qUsed, wBytes);
        notify_stop();
        return NOTIFY_IGNORED;
      }
      g_stat.randomLoss++;
      return NOTIFY_DONE;
    }
  }

  if (uq.o->bitError != 0) {    /* bit error */
    if ( (corruptBits = isBitError(wBuff, wBytes, uq.o->bitError)) > 0) {
      if (uq.o->verbose) {
        printf("Bit error:  corrupt bits: %d\n", corruptBits);
        printf("  corrupted packet: send counter: %lu  size: %d(bytes)\n",
                  (unsigned long)sUdpC, wBytes);
      }
      g_stat.biterror += corruptBits;
    }
  }

  if (uq.o->outOfOrder != 0) {  /* out of order */
    if (isOutOfOrder(uq.queue, uq.o->outOfOrder)) {
      if (uq.o->verbose) {
        puts  ("Out of order:");
        printf("  packet: send counter: %lu  size: %d(bytes)\n",
                  (unsigned long)sUdpC, wBytes);
      }
      g_stat.outOfOrder++;
    }
  }                             /* end of generating errors */

  /* send a packet */
  setSockAddr(&writAddr, &uq.o->destHostIpN, uq.o->destHostPort);
  written = sendto(uq.writfd, (const char *) wBuff, wBytes,
                   0, (struct sockaddr *) &writAddr, sizeof(writAddr));
  if (written != wBytes) {
    perror("sendHandler(): sendto()");
    notify_stop();
    return NOTIFY_IGNORED;
  }

  if (uq.o->verbose >= 2) {
    printf("send    : send counter: %lu  size: %d(bytes)\n",
                                    (unsigned long)sUdpC, wBytes);
  }

  /* decrease the counter of the queue buffer utilization */
  if (decQUsed(&uq.qUsed, written, uq.qBufSize) < 0) {
    fprintf(stderr, "%s: decQUsed(): uq.qUsed: %d wBytes: %d.\n",
            uq.o->argv0, uq.qUsed, wBytes);
    notify_stop();
    return NOTIFY_IGNORED;
  }

  sUdpC += written;
  g_stat.sPacket++;
  g_stat.lasttime = now_tv;

  return NOTIFY_DONE;
} /* sendHandler() */

/** flush queue.
 * stop receiving anymore packets and flushes the packtes in the qeueu */
void flushQueue()
{
  struct timeval timeout;
  int i;

  /* very rough way but flushing queue buffer */
  /* clear readHandler to stop receiving packets */
  for (i = 0; i <= (rUdpC + 100); i++)
    notify_set_input_func( i, NULL, uq.readfd);

  if (timer_pending() != 0) {
    puts   ("  Wait while flushing packets in the buffer.");
    putchar('\n');
  }
  for (i = 0;; i++) {
    timer_get(&timeout);
    if (timer_pending() == 0)
      return;
  }
  return;
} /*** flushQueue() ***/

/** signal handler.
 * traps ctr-c and stop the emulator */
Notify_value sigHandler(Notify_client client, int sig,
                        Notify_signal_mode mode)
{
  putchar('\n');
  puts("!Terminated by ctr-c.");
  putchar('\n');
  puts  ("Closing network emulation.");

  flushQueue();
  notify_stop();

  return NOTIFY_DONE;
} /* sigHandler() */

/** stdin handler.
 * traps stdin and stop the emulator by the user's command */
Notify_value stdinHandler(Notify_client client, int dummeyfd)
{
  char buff[STRSIZE];

  fgets(buff, STRSIZE, stdin);

  if (strcmp("quit\n", buff) == 0)
    goto TERMINATE;
  if (strcmp("q\n", buff) == 0)
    goto TERMINATE;
  if (strcmp("stop\n", buff) == 0)
    goto TERMINATE;
  if (strcmp("exit\n", buff) == 0)
    goto TERMINATE;
  return NOTIFY_IGNORED;

  TERMINATE:
  printf("!Terminated by command: %s\n", buff);
  putchar('\n');
  puts  ("Closing network emulation.");

  flushQueue();
  notify_stop();

  return NOTIFY_DONE;
} /* stdinHandler() */

/***************************************************************************
 *** main ***
 ***************************************************************************/
/** main.
 * everything starts here */
int main(int argc, char *argv[])
{
  Options options;
  int i;
  Notify_client client = 0;

  /* Parse command line arguments */
  if (initOptions(&options, argv[0]) < 0) {
    fprintf(stderr, "%s: Initialize error.\n", argv[0]);
    return -1;
  }
  if (parseCmdLine(&options, argc, argv) < 0) {
    return -1;
  }

  /** Now it is the real main routine **/
  /* Initialize */
  if (init(&options) < 0) {
    fprintf(stderr, "%s: Initialize error.\n", uq.o->argv0);
    return -1;
  }

  /* Print title */
  if (printTitle(&options) < 0) {
    fprintf(stderr, "%s: Error: printing title.\n", uq.o->argv0);
    return -1;
  }

  /* Set stdin handler (command from keybord) */
  notify_set_input_func(client, stdinHandler, fileno(stdin));

  /* Set signal handler (terminating) */
  notify_set_signal_func(client, sigHandler, SIGINT, NOTIFY_ASYNC);

  /* Set readFd handler (reading and queueing UDP packets) */
  notify_set_input_func((Notify_client) rUdpC, readHandler, uq.readfd);

  if ( (i = notify_start()) == -1) {     /* != NOTIFY_OK, bug notify_start()? */
//    fprintf(stderr, "%s: Notifier error %d.\n", argv[0], i);
//    return -1;
  }

  printStatistics();

  if (destroy() < 0) {
    fprintf(stderr, "%s: Could not destroy all the handler.\n", argv[0]);
    return -1;
  }

  return 0;
} /* main() */

