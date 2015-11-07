#ifndef WINSOCKLIB_H_
#define WINSOCKLIB_H_
/***************************************************************************
 *
 * Header file for winsock related functions.
 *  winsocklib.h
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef WIN32
extern void startupSocket(void);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* WINSOCKLIB_H_ */
