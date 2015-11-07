/***************************************************************************
 *
 * Code for uname() function.
 *  uname.c
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#include "sysdep.h"
#include "uname.h"

#if defined(WIN32) && defined(HAVE_WINSOCK2)
int uname(struct utsname *name)
{
    char buff[MAXBUFF];

    OSVERSIONINFO os;       /* GetVersionEx() */
    SYSTEM_INFO sys;        /* GetSystemInfo() */

    if (name == NULL) {
        errno = EFAULT;
        return -1;
    }
    memset(name, 0, sizeof(struct utsname));

    /* sysname */
    strcpy(name->sysname, "Windows");

    /* nodename */
    memset(buff, 0, sizeof(buff));
    if (gethostname(buff, MAXBUFF)) {
        errno = EFAULT;
        return -1;
    }
    strncpy(name->nodename, buff, SYS_NMLN-1);
    name->nodename[SYS_NMLN-1] = '\0';

    /* os release. version */
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os)) {
        errno = EFAULT;
        return -1;
    }
    sprintf(buff, "%d.%d.%d",
            os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber);
    strncpy(name->release, buff, SYS_NMLN-1);
    name->release[SYS_NMLN-1] = '\0';
    strncpy(name->version, os.szCSDVersion, 128-1);
    name->version[128-1] = '\0';

    /* machine */
    GetSystemInfo(&sys);
    if (sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        strcpy(name->machine, "i386");
    else
        strcpy(name->machine, "unknown");
    return 0;
}
#endif
