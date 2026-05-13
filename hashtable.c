#include"hashtable.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

static unsigned int djb2Hash(const char *str){
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5)+ hash) +c;
    return hash % HASH_SIZE;
    
}

HashTable *createHashTable(void){
    HashTable *ht = calloc(1, sizeof(HashTable));
    if (!ht)
    {
        fprintf(stderr, "Hashtable alloc failed\n");
        exit(1);
    }
    return ht;
}
void hashInsert(HashTable *ht, const char *name, int stationIndex){
    unsigned int bucket = djb2Hash(name);
    HashNode *cur = ht->buckets[bucket];
    while (cur)
    {
        if (strcmp(cur->key, name) ==0)
        {
            cur->stationIndex = stationIndex;
            return;
        }
        cur = cur->next;
    }
    HashNode *node = malloc(sizeof(HashNode));
    if (!node) return;
    strncpy(node->key, name, MAX_NAME_LEN -1);
    node->key[MAX_NAME_LEN -1] = '\0';
    node->stationIndex = stationIndex;
    node->next = ht->buckets[bucket];
    ht->buckets[bucket] = node;
    
}

int hashSearch(const HashTable *ht, const char *name){
    unsigned int bucket = djb2Hash(name);
    const HashNode *cur = ht->buckets[bucket];
    while (cur)
    {
        if(strcmp(cur->key, name) == 0) return cur->stationIndex;
        cur = cur->next;
    }
    return -1;
}

void hashRemove(HashTable *ht, const char *name){
    unsigned int bucket = djb2Hash(name);
    HashNode **pp = &ht->buckets[bucket];
    while (*pp)
    {
        if(strcmp((*pp)->key, name) ==0)
        {
            HashNode *del = *pp;
            *pp = del->next;
            free(del);
            return;
        }
        pp = &(*pp)->next;
    }
    
}
void hashUpdateIndex(HashTable *ht, const char *name, int newIndex){
    unsigned int bucket = djb2Hash(name);
    HashNode *cur = ht->buckets[bucket];
    while (cur)
    {
        if(strcmp(cur->key, name) ==0)
        {
            cur->stationIndex = newIndex;
            return;
        }
        cur = cur->next;
    }
    
}
void hashClear(HashTable *ht){
    if (!ht) return;
    for (int i = 0; i < HASH_SIZE; i++){
        HashNode *cur = ht->buckets[i];
        while (cur){
            HashNode *nxt = cur->next;
            free(cur);
            cur = nxt;
        }
        ht->buckets[i] = NULL;
    }
}

void freeHashTable(HashTable *ht){
    if (!ht) return;
    for (int i = 0; i < HASH_SIZE; i++)
    {
        HashNode *cur = ht->buckets[i];
        while (cur)
        {
            HashNode *nxt = cur->next;
            free(cur);
            cur = nxt;
        }
        
    }
    
    free(ht);
}