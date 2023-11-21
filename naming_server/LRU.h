
#ifndef LRU_H
#define LRU_H
#define MAX_FILENAME_LENGTH 256
#define MAX_CACHE_SIZE 3 // Define the maximum size of the cache
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
// Structure for a queue node
typedef struct QueueNode {
    char file_name[MAX_FILENAME_LENGTH];
    int storage_server_id;
    struct QueueNode* next;
} QueueNode;

// Structure for LRU Cache using a queue
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
    int capacity;
} LRUCacheQueue;

QueueNode* createQueueNode(const char* file_name, int storage_server_id);

LRUCacheQueue* createLRUCacheQueue(int capacity);
void enqueue(LRUCacheQueue* cacheQueue, const char* file_name, int storage_server_id);
void printCacheQueue(LRUCacheQueue* cacheQueue);
int LRU_search(LRUCacheQueue* cacheQueue, const char* file_name);
int LRU_set_to_neg(LRUCacheQueue* cacheQueue, const char* file_name);

#endif