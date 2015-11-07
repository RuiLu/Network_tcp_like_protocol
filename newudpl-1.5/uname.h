#ifndef UNAME_H_
#define UNAME_H_
/***************************************************************************
 *
 * Header file for uname() function.
 *  uname.h
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#include "sysdep.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef SYS_NMLN
#ifdef MAXHOSTNAMELEN
#define SYS_NMLN MAXHOSTNAMELEN+1
#else
#define SYS_NMLN 257
#endif
#endif /* SYS_NMLN */

#ifndef HAVE_UNAME
struct utsname {
    char    sysname[SYS_NMLN];
    char    nodename[SYS_NMLN];
    char    release[SYS_NMLN];
    char    version[SYS_NMLN];
    char    machine[SYS_NMLN];
};

extern int uname(struct utsname *name);

#endif /* HAVE_UNAME */

#ifdef  __cplusplus
}
#endif

#endif /* UNAME_H_ */
