
#include "sysdep.h"

static char rcsid[] = "$Id: host2ip.c,v 1.1.1.1 1999/02/14 15:36:02 hgs Exp $";

/*
* Return IP address given host name 'host'.
* If 'host' is "", set to INADDR_ANY.
*/
struct in_addr host2ip(char *host)
{
  struct in_addr in;
  register struct hostent *hep;

  /* Check whether this is a dotted decimal. */
  if (!host || *host == '\0') {
    in.s_addr = INADDR_ANY;
  }
  else if ((in.s_addr = inet_addr(host)) != -1) {
  }
  /* Attempt to resolve host name via DNS. */
  else if ((hep = gethostbyname(host))) {
    in = *(struct in_addr *)(hep->h_addr_list[0]);
  }
  /* As a last resort, try YP. */
  else {
    static char *domain = 0;  /* YP domain */
    char *value;              /* key value */
    int value_len;            /* length of returned value */

    /* added by Akira T. 01/07/2002 */
    /* inet_addr() have destryed it, reset INADDR_ANY*/
    in.s_addr = INADDR_ANY;  

    /* #ifdef HAVE_YP_MATCH added by Akira T. 01/07/2002 */
    /* because win32 does not have yp functions */
#ifdef HAVE_YP_MATCH         
    if (!domain) yp_get_default_domain(&domain);
    if (yp_match(domain, "hosts.byname", host, strlen(host), &value, &value_len) == 0) {
      in.s_addr = inet_addr(value);
    }
#endif
  }
  return in;
} /* host2ip */
