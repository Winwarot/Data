#ifndef DATA_H
#define DATA_H

#include <stddef.h>
#include "graph.h"
#include "hashtable.h"
#include "bst.h"

void initDataPaths(const char *argv0);
void buildDataPath(char *out, size_t outSize, const char *filename);

char *trimNewline(char *s);
char *trimSpace(char *s);
int   nextField(char **p, char *out, int maxLen);

int  hasEdge(const Graph *g, int from, int to);
void linkSameNameInterchanges(Graph *g);
void rebuildLookupStructures(const Graph *g, HashTable *ht, BST *bst);

void loadData(Graph *g, HashTable *ht, BST *bst);
void saveData(const Graph *g);

#endif
