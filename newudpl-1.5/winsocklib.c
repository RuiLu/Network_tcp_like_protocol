/***************************************************************************
 *
 * Code for winsock related functions.
 *  winsocklib.c
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#include "sysdep.h"

#if defined(WIN32)
void startupSocket(void)
{
  WSADATA wsaData;      /* WSAStartup() */

  /* startup winsock */
  if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
    fprintf(stderr, "WSAStartup(): Could not start up WinSock.\n");
    exit(1);
  }
}
#endif /* WIN32 */