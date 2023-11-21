#ifndef TRIES_H_
#define TRIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Define the maximum number of characters in a file name
#define MAX_CHAR 26

// Trie Node structure
struct TrieNode
{
    struct TrieNode *children[MAX_CHAR];
    int storageServerId; // Storage server ID associated with the file name
    int isEndOfWord;     // Flag to indicate the end of a word
};
struct TrieNode *createNode();
void insert(struct TrieNode *root, const char *fileName, int storageServerId);
int Efficient_search(struct TrieNode *root, const char *fileName);
void removeFileName(struct TrieNode *root, const char *fileName);

#endif 