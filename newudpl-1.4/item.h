#ifndef _ITEM_H_
#define _ITEM_H_
/***********************************************************
 * item.h
 * Header to define item type and useful macros
 */

/**** PUBLIC STRUCTURES, TYPES ****/

typedef struct item_s Item;
struct item_s {
  char *data;
  int bytes;
};

/**** MACROS ****/

//#define MAXDATASIZE      6000

#endif                          /* _ITEM_H_ */
