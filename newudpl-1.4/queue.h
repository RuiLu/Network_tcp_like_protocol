#ifndef _QUEUE_H_
#define _QUEUE_H_
/***********************************************************
 * queue.h
 * Header file for functions to manipulate a queue.
 */

/**** INCLUDES ****/
#include "item.h"
#define _MAX_Q_SIZE_    2048

typedef Item QItems;

/**** queueTypes ****/

typedef struct queuenode_s QueueNode;
struct queuenode_s {
  QItems *items;
  QueueNode *prev;
  QueueNode *next;
};

typedef struct queue_s Queue;
struct queue_s {
  int count;
  QueueNode *root;
  QueueNode *last;
};

/*** PUBLIC FUNCTION PROTOTYPES ***/

/** Initialize the queue Q to be the empty queue */
Queue *openQueue(void);

/** Close and free the queue Q */
void closeQueue(Queue * queue);

/** Returns number of elements in the queue */
int countQueue(Queue * queue);

/** If Q is not full, insert a new item onto the rear of Q */
Queue *addQueue(Queue * queue, QItems * newItem);

/** If Q is non-empty, remove the frontmost item of Q and return it */
Queue *getQueue(Queue * queue, QItems ** itemToReturn);

#endif                          /* _QUEUE_H_ */
