#ifndef HASHTABLE_H
#define HASHTABLE_H
#include "station.h"
#define HASH_SIZE 512

typedef struct HashNode
{
    char key[MAX_NAME_LEN];
    int stationIndex;
    struct HashNode *next;
} HashNode;


typedef struct HashTable {
    HashNode *buckets[HASH_SIZE];
} HashTable;

HashTable *createHashTable(void);
void hashInsert(HashTable *ht, const char *name, int stationIndex);
int  hashSearch(const HashTable *ht, const char *name);
void hashRemove(HashTable *ht, const char *name);
void hashUpdateIndex(HashTable *ht, const char *name, int newIndex);
void hashClear(HashTable *ht);
void freeHashTable(HashTable *ht);
#endif