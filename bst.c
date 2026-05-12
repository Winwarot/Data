#include "bst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Create a new BST */
BST *createBST(void) {
    BST *tree = malloc(sizeof(BST));
    if (tree == NULL) {
        fprintf(stderr, "Error: could not allocate BST\n");
        exit(1);
    }
    tree->root  = NULL;
    tree->count = 0;
    return tree;
}

/* Create a new node */
BSTNode *newNode(const char *name) {
    BSTNode *node = malloc(sizeof(BSTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: could not allocate node\n");
        return NULL;
    }
    strncpy(node->stationName, name, MAX_NAME_LEN - 1);
    node->stationName[MAX_NAME_LEN - 1] = '\0';
    node->left  = NULL;
    node->right = NULL;
    return node;
}

/* Insert a node */
BSTNode *insertNode(BSTNode *node, const char *name, int *inserted) {
    if (node == NULL) {
        *inserted = 1;
        return newNode(name);
    }

    int cmp = strcmp(name, node->stationName);

    if (cmp < 0) {
        node->left = insertNode(node->left, name, inserted);
    } else if (cmp > 0) {
        node->right = insertNode(node->right, name, inserted);
    }

    return node;
}

void bstInsert(BST *tree, const char *name) {
    int inserted = 0;
    tree->root = insertNode(tree->root, name, &inserted);
    if (inserted) {
        tree->count++;
    }
}

/* Search */
int searchNode(BSTNode *node, const char *name) {
    if (node == NULL) return 0;

    int cmp = strcmp(name, node->stationName);

    if (cmp == 0) {
        return 1;
    } else if (cmp < 0) {
        return searchNode(node->left, name);
    } else {
        return searchNode(node->right, name);
    }
}

int bstSearch(const BST *tree, const char *name) {
    return searchNode(tree->root, name);
}

/* Partial search */
void partialInorder(BSTNode *node, const char *prefix, int prefixLen, int *found) {
    if (node == NULL) return;

    partialInorder(node->left, prefix, prefixLen, found);

    if (strncmp(node->stationName, prefix, prefixLen) == 0) {
        printf("    %s\n", node->stationName);
        (*found)++;
    }

    partialInorder(node->right, prefix, prefixLen, found);
}

int bstSearchPartial(const BST *tree, const char *prefix) {
    int found = 0;
    int prefixLen = (int)strlen(prefix);
    partialInorder(tree->root, prefix, prefixLen, &found);
    return found;
}

/* Print all stations A - Z */
void inorderPrint(BSTNode *node, int *counter) {
    if (node == NULL) return;

    inorderPrint(node->left, counter);
    (*counter)++;
    printf("  %3d. %s\n", *counter, node->stationName);
    inorderPrint(node->right, counter);
}

void bstInorder(const BST *tree) {
    int counter = 0;
    inorderPrint(tree->root, &counter);
}

/* Find the leftmost node */
BSTNode *findMin(BSTNode *node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

/* Remove a node by name */
BSTNode *removeNode(BSTNode *node, const char *name) {
    if (node == NULL) return NULL;

    int cmp = strcmp(name, node->stationName);

    if (cmp < 0) {
        node->left = removeNode(node->left, name);
    } else if (cmp > 0) {
        node->right = removeNode(node->right, name);
    } else {
        /* Case: leaf node */
        if (node->left == NULL && node->right == NULL) {
            free(node);
            return NULL;

        /* only right child */
        } else if (node->left == NULL) {
            BSTNode *tmp = node->right;
            free(node);
            return tmp;

        /* only left child */
        } else if (node->right == NULL) {
            BSTNode *tmp = node->left;
            free(node);
            return tmp;

        /* Case: two children — replace with inorder successor */
        } else {
            BSTNode *successor = findMin(node->right);
            strncpy(node->stationName, successor->stationName, MAX_NAME_LEN - 1);
            node->right = removeNode(node->right, successor->stationName);
        }
    }
    return node;
}

void bstRemove(BST *tree, const char *name) {
    if (!bstSearch(tree, name)) return;
    tree->root = removeNode(tree->root, name);
    tree->count--;
}

/* Free node */
void freeNodes(BSTNode *node) {
    if (node == NULL) return;
    freeNodes(node->left);
    freeNodes(node->right);
    free(node);
}

void bstClear(BST *tree) {
    if (tree == NULL) return;
    freeNodes(tree->root);
    tree->root  = NULL;
    tree->count = 0;
}

/* Free BST struct */
void freeBST(BST *tree) {
    if (tree == NULL) return;
    bstClear(tree);
    free(tree);
}
