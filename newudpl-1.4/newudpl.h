#ifndef _NEWUDPL_H_
#define _NEWUDPL_H_
/***************************************************************************
 *
 *  Network Emulator With UDP Link
 *  newudpl.h
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *
 ***************************************************************************/

/** Options options.
 * contains all the options setting from command line */
struct options_s {
  /** using it for error message */
  char argv0[NAME_MAX];                  
  /** UDP port for read */
  in_port_t readPort;           
  /** UDP port for write */
  in_port_t writPort;           
  /** source host ip and port numbers */
  struct in_addr srceHostIpN;   
  in_port_t srceHostPort;
  /** destination host ip and port */
  struct in_addr destHostIpN;   
  in_port_t destHostPort;
  /** link speed (kb/s) */
  uint32_t speed;               
  /** ehternet speed (kb/s) */
  int ether;               
  /** delay time */
  struct timeval delay;         
  /** Queue buffer size for reading packets */
  int inQBufSize;               
  /** probability of bit error */
  int bitError;                 
  /** % of random packet loss */
  int randomLoss;               
  /** probability of Gilbert packet loss */
  float gilCondLoss;               
  /** probability of Gilbert packet loss */
  float gilUncondLoss;
  /** % of outOfOrder */
  int outOfOrder;               
  /** mode of calculating departure time */
  int calcMode;                 
  /** degree of verbose mode */
  int verbose;
  /** the size of RAND_MAX */
  int rand_short;               
};
typedef struct options_s Options;

/** Global struct.
 * this is used for sendHandler() and readHandler()
 * to keep truck of packets */
struct udpqueue_s {
  Options *o;
  /** reading socket file descriptor */
  int readfd;                   
  /** writing socket file descriptor */
  int writfd;
  /** pointer to queue */                   
  Queue *queue;
  /** size of queue */                   
  int qBufSize;
  /** counter of utilization of the queue */
  int qUsed;
};
typedef struct udpqueue_s UdpQueue;

/** Global struct.
 * contains statisitcs information */
struct statistics_s {
  /** a time of first transmitted packtes */
  struct timeval startime;
  /** a time of the end of transitting packet */
  struct timeval lasttime;
  /** a time of previous packet arrived */
  struct timeval prvRcv_tv;
  /** a interval time of previous packet */
  struct timeval prvIntervled_tv;
  /** minimum speed of receiving packets */
  uint32_t minSp;
  /** maximum speed of receiving packets */
  uint32_t maxSp;
  /** total number of received packets */
  int rPacket;
  /** total number of send packets */
  int sPacket;
  /** total number of rejected packets from invalid host */
  int rejPack;
  /** total bytes of rejected packets from invalid host */
  int rejBytes;
  /** total number of dropped packets from congestion */
  int congPack;
  /** total bytes of dropped packets from congestion */
  int congBytes;
  /** total number of random loss packets */
  int randomLoss;
  /** total number of packets with bit error */
  int biterror;
  /** total number of packets with out of order */
  int outOfOrder;
};
typedef struct statistics_s Statistics;

#endif                          /* _NEWUDPL_H_ */
