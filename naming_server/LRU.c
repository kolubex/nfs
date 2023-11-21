#include "LRU.h"
// Function to create a new queue node
QueueNode* createQueueNode(const char* file_name, int storage_server_id) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode != NULL) {
        strncpy(newNode->file_name, file_name, MAX_FILENAME_LENGTH);
        newNode->storage_server_id = storage_server_id;
        newNode->next = NULL;
    }
    return newNode;
}

// Function to initialize the LRU Cache queue
LRUCacheQueue* createLRUCacheQueue(int capacity) {
    LRUCacheQueue* cacheQueue = (LRUCacheQueue*)malloc(sizeof(LRUCacheQueue));
    if (cacheQueue != NULL) {
        cacheQueue->front = NULL;
        cacheQueue->rear = NULL;
        cacheQueue->size = 0;
        cacheQueue->capacity = capacity;
    }
    return cacheQueue;
}

// Function to enqueue a file name and storage server ID into the LRU Cache queue
void enqueue(LRUCacheQueue* cacheQueue, const char* file_name, int storage_server_id) {
    if (cacheQueue == NULL) {
        return;
    }

    QueueNode* newNode = createQueueNode(file_name, storage_server_id);
    if (newNode == NULL) {
        return;
    }

    // If the queue is already at its capacity, remove the least recently used node (front of the queue)
    if (cacheQueue->size >= cacheQueue->capacity) {
        QueueNode* temp = cacheQueue->front;
        cacheQueue->front = temp->next;
        free(temp);
        cacheQueue->size--;
        if (cacheQueue->front == NULL) {
            cacheQueue->rear = NULL;
        }
    }

    // Enqueue the new node at the rear of the queue
    if (cacheQueue->rear == NULL) {
        cacheQueue->front = newNode;
    } else {
        cacheQueue->rear->next = newNode;
    }
    cacheQueue->rear = newNode;
    cacheQueue->size++;
}

// Function to print the contents of the LRU Cache queue
void printCacheQueue(LRUCacheQueue* cacheQueue) {
    QueueNode* temp = cacheQueue->front;
    while (temp != NULL) {
        printf("%s (%d) ", temp->file_name, temp->storage_server_id);
        temp = temp->next;
    }
    printf("\n");
}

int LRU_search(LRUCacheQueue* cacheQueue, const char* file_name) {
    QueueNode* temp = cacheQueue->front;
    while (temp != NULL) {
        if (strcmp(temp->file_name, file_name) == 0) {
            printf("Found file %s in LRU Cache\n", file_name);
            return temp->storage_server_id;
        }
        temp = temp->next;
    }
    return -1;
}

int LRU_set_to_neg(LRUCacheQueue* cacheQueue, const char* file_name) {
    QueueNode* temp = cacheQueue->front;
    while (temp != NULL) {
        if (strcmp(temp->file_name, file_name) == 0) {
            temp->storage_server_id = -1;
            return 0;
        }
        temp = temp->next;
    }
    return -1;
}

// Example usage
// int main() {
//     LRUCacheQueue* cacheQueue = createLRUCacheQueue(MAX_CACHE_SIZE);

//     // Adding file names and storage server IDs to the LRU Cache queue
//     enqueue(cacheQueue, "file4", 3);
//     enqueue(cacheQueue, "file1", 1);
//     enqueue(cacheQueue, "file2", 2);
//     enqueue(cacheQueue, "file3", 1);

//     // Printing the contents of the LRU Cache queue
//     printf("LRU Cache contents: ");
//     printCacheQueue(cacheQueue); // Output: LRU Cache contents: file1 (1) file2 (2) file3 (1) file4 (3)

//     // Cleanup: You should free the allocated memory when you're done with the queue
//     // (not implemented in this example for simplicity)

//     return 0;
// }
