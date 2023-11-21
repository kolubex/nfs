#include "tries.h"
// Function to initialize a new TrieNode
struct TrieNode *createNode()
{
    struct TrieNode *newNode = (struct TrieNode *)malloc(sizeof(struct TrieNode));
    if (newNode)
    {
        newNode->isEndOfWord = 0;
        newNode->storageServerId = -1;
        for (int i = 0; i < MAX_CHAR; i++)
        {
            newNode->children[i] = NULL;
        }
    }
    return newNode;
}

// Function to insert a file name and its storage server ID into the trie
void insert(struct TrieNode *root, const char *fileName, int storageServerId)
{
    struct TrieNode *node = root;
    int length = strlen(fileName);

    for (int level = 0; level < length; level++)
    {
        int index = fileName[level] - 'a';

        if (!node->children[index])
        {
            node->children[index] = createNode();
        }

        node = node->children[index];
    }

    node->isEndOfWord = 1;
    node->storageServerId = storageServerId;
}

// Function to search for a file name in the trie and return the storage server ID
int Efficient_search(struct TrieNode *root, const char *fileName)
{
    printf("In Efficient_search\n");
    struct TrieNode *node = root;
    int length = strlen(fileName);

    for (int level = 0; level < length; level++)
    {
        int index = fileName[level] - 'a';

        if (!node->children[index])
        {
            // File name not found
            return -1;
        }

        node = node->children[index];
    }

    if (node != NULL && node->isEndOfWord)
    {
        // File name found, return the associated storage server ID
        return node->storageServerId;
    }
    else
    {
        // File name not found
        return -1;
    }
}

// Function to remove a file name from the trie
void removeFileName(struct TrieNode *root, const char *fileName)
{
    int length = strlen(fileName);
    struct TrieNode *node = root;

    if (length == 0)
    {
        return;
    }

    // Stack to keep track of nodes in the path
    struct TrieNode *stack[length + 1];
    int top = -1;

    // Traverse the trie to find the node corresponding to the file name
    for (int level = 0; level < length; level++)
    {
        int index = fileName[level] - 'a';

        if (!node->children[index])
        {
            // File name doesn't exist in the trie
            return;
        }

        stack[++top] = node;
        node = node->children[index];
    }

    // Mark the last node as not the end of the word
    node->isEndOfWord = 0;
    node->storageServerId = -1;

    // Check if the last node has no children and remove nodes accordingly
    if (node->isEndOfWord == 0)
    {
        for (int level = length - 1; level >= 0; level--)
        {
            int index = fileName[level] - 'a';

            if (node->children[index] != NULL)
            {
                return;
            }

            free(node->children[index]);
            node->children[index] = NULL;

            top--;
            node = stack[top];
        }
    }
}

// Example usage
// int main()
// {
//     struct TrieNode *root = createNode();

//     // Insert file names and storage server IDs
//     insert(root, "file1.txt", 1);
//     insert(root, "file2.txt", 2);
//     insert(root, "file3.txt", 3);
//     insert(root, "file4", 1);

//     // Search for a file name
//     const char *fileNameToSearch = "file4";
//     int storageServerId = search(root, fileNameToSearch);

//     if (storageServerId != -1)
//     {
//         printf("File '%s' found on Storage Server %d\n", fileNameToSearch, storageServerId);
//     }
//     else
//     {
//         printf("File '%s' not found\n", fileNameToSearch);
//     }

//     removeFileName(root, "file4");
//     storageServerId = search(root, fileNameToSearch);

//     if (storageServerId != -1)
//     {
//         printf("File '%s' found on Storage Server %d\n", fileNameToSearch, storageServerId);
//     }
//     else
//     {
//         printf("File '%s' not found\n", fileNameToSearch);
//     }

//     // Cleanup: You should free the allocated memory when you're done with the trie
//     // (not implemented in this example for simplicity)

//     return 0;
// }