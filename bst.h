#ifndef BST_H
#define BST_H

#include "station.h"

typedef struct BSTNode {
    char stationName[MAX_NAME_LEN];
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct BST {
    BSTNode *root;
    int count;
} BST;

BSTNode *newNode(const char *name);
BSTNode *insertNode(BSTNode *node, const char *name, int *inserted);
int      searchNode(BSTNode *node, const char *name);
void     partialInorder(BSTNode *node, const char *prefix, int prefixLen, int *found);
void     inorderPrint(BSTNode *node, int *counter);
BSTNode *findMin(BSTNode *node);
BSTNode *removeNode(BSTNode *node, const char *name);
void     freeNodes(BSTNode *node);

BST  *createBST(void);
void  bstInsert(BST *tree, const char *name);
int   bstSearch(const BST *tree, const char *name);
int   bstSearchPartial(const BST *tree, const char *prefix);
void  bstInorder(const BST *tree);
void  bstRemove(BST *tree, const char *name);
void  bstClear(BST *tree);
void  freeBST(BST *tree);

#endif
