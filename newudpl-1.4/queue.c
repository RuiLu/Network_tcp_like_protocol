/***********************************************************
 * queue.c
 * Code for functions to manipulate a queue.
 */

/**** INCLUDES ****/
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

/**** PRIVATE FUNCTION PROTOTYPES ****/

/**** PUBLIC FUNCTION DEFINITIONS ****/

/* Initialize the queue Q to be the empty queue */
Queue *openQueue(void)
{
  Queue *newQueue;

  newQueue = (Queue *) malloc(sizeof(Queue));

  if (!newQueue) {
    printf("ERROR: No memory for Queue!\n");
    return NULL;
  }

  newQueue->count = 0;          /* Count == number of items in the queue */
  newQueue->root = NULL;        /* Front == location of item to remove next */
  newQueue->last = NULL;        /* Rear == place to insert next item */

  return newQueue;
}

int countQueue(Queue * queue)
{
  return queue->count;
}

/* If Q is not full, insert a new item onto the rear of Q */
Queue *addQueue(Queue * queue, QItems * newItem)
{
  QueueNode *newNode;

  if (queue->count >= _MAX_Q_SIZE_) {
    fprintf(stderr, "Error! Too much data for Queue.\n");
    return NULL;
  }
  if (!queue) {
    fprintf(stderr, "getQueue(): queue is NULL.\n");
    return NULL;
  }
  if (newItem == NULL) {
    fprintf(stderr, "getQueue(): newItem is NULL.\n");
    return NULL;
  }

  /* Create a new list node and stick the new item into it */
  newNode = (QueueNode *) malloc(sizeof(QueueNode));

  newNode->items = newItem;
  newNode->prev = NULL;
  newNode->next = NULL;

  /* Add to the root when queue is empty */
  if (queue->root == NULL) {
    queue->last = newNode;
  } else {
    queue->root->prev = newNode;
  }

  newNode->next = queue->root;
  queue->root = newNode;
  queue->count++;

  return queue;
}

/* If Q is non-empty, remove the frontmost item of Q and return it */
Queue *getQueue(Queue * queue, QItems ** itemToReturn)
{
  QueueNode *lastNode;

  if (queue->count <= 0) {
    fprintf(stderr, "Error! No more data in Queue.\n");
    return NULL;
  }

  if (!queue) {
    fprintf(stderr, "getQueue(): ERROR: queue is NULL.\n");
    return NULL;
  }

  *itemToReturn = queue->last->items;

  /* Empty the queue when it's the last Node */
  if (queue->root == queue->last) {
    queue->root = NULL;
  } else {
    queue->last->prev->next = NULL;
  }

  lastNode = queue->last;
  queue->last = queue->last->prev;
  free(lastNode);
  queue->count--;

  return queue;
}

void closeQueue(Queue * queue)
{
  QItems *dummy;

  if (!queue) {
    fprintf(stderr, "closeQueue(): ERROR: queue is NULL.\n");
    return;
  }

  /* Dump remained elements in the queue */
  while (queue->count) {
    getQueue(queue, &dummy);
  }
  free(queue);
}
