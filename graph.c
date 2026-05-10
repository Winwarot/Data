#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Graph *createGraph(void) {
    Graph *g = calloc(1, sizeof(Graph));
    if (!g) { fprintf(stderr, "Graph alloc failed\n"); exit(1); }
    g->stationCount = 0;
    return g;
}

// returns index, -1 if full
int graphAddStation(Graph *g, const char *code, const char *name, const char *line, int isInterchange) {
    if (g->stationCount >= MAX_STATIONS) return -1;

    int idx = g->stationCount++;

    g->stations[idx].id = idx;
    g->stations[idx].isInterchange = isInterchange;
    g->stations[idx].isClosed = 0;

    strcpy(g->stations[idx].code, code);
    strcpy(g->stations[idx].name, name);
    strcpy(g->stations[idx].line, line);

    g->adjList[idx] = NULL;
    return idx;
}

// prepend to adj list
void graphAddEdge(Graph *g, int from, int to, double dist, int time, int fare) {
    if (from < 0 || to < 0 || from >= g->stationCount || to >= g->stationCount) return;

    Edge *e = malloc(sizeof(Edge));
    if (!e) return;

    e->destination = to;
    e->distance    = dist;
    e->time        = time;
    e->fare        = fare;
    e->next        = g->adjList[from];
    g->adjList[from] = e;
}

void graphAddUndirectedEdge(Graph *g, int a, int b, double dist, int time, int fare) {
    graphAddEdge(g, a, b, dist, time, fare);
    graphAddEdge(g, b, a, dist, time, fare);
}

void graphCloseStation(Graph *g, int idx) {
    if (idx >= 0 && idx < g->stationCount)
        g->stations[idx].isClosed = 1;
}

void graphReopenStation(Graph *g, int idx) {
    if (idx >= 0 && idx < g->stationCount)
        g->stations[idx].isClosed = 0;
}

// remove station and reindex
void graphRemoveStation(Graph *g, int idx) {
    if (idx < 0 || idx >= g->stationCount) return;

    // free outgoing edges
    Edge *cur = g->adjList[idx];
    while (cur) {
        Edge *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    g->adjList[idx] = NULL;

    // remove incoming edges
    for (int i = 0; i < g->stationCount; i++) {
        Edge **pp = &g->adjList[i];
        while (*pp) {
            if ((*pp)->destination == idx) {
                Edge *del = *pp;
                *pp = del->next;
                free(del);
            } else {
                pp = &(*pp)->next;
            }
        }
    }

    // shift left
    for (int i = idx; i < g->stationCount - 1; i++) {
        g->stations[i] = g->stations[i + 1];
        g->stations[i].id = i;
        g->adjList[i] = g->adjList[i + 1];
    }
    g->stationCount--;

    // update shifted destinations
    for (int i = 0; i < g->stationCount; i++) {
        Edge *e = g->adjList[i];
        while (e) {
            if (e->destination > idx) e->destination--;
            e = e->next;
        }
    }
}

void graphDisplay(const Graph *g) {
    printf("\n  === Graph Adjacency List (%d stations) ===\n", g->stationCount);
    for (int i = 0; i < g->stationCount; i++) {
        printf("  [%3d] %-40s -> ", i, g->stations[i].name);
        Edge *e = g->adjList[i];
        if (!e) { printf("(none)\n"); continue; }
        while (e) {
            printf("%s(%.1fkm,%dmin,%dTHB) ",
                   g->stations[e->destination].name,
                   e->distance, e->time, e->fare);
            e = e->next;
        }
        printf("\n");
    }
}

void freeGraph(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->stationCount; i++) {
        Edge *cur = g->adjList[i];
        while (cur) {
            Edge *nxt = cur->next;
            free(cur);
            cur = nxt;
        }
    }
    free(g);
}
