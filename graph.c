#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MinHeap for dijkstra
typedef struct {
    int cost;
    int stationIndex;
} HeapNode;

typedef struct {
    HeapNode *data;
    int size;
    int capacity;
} MinHeap;

static MinHeap *createMinHeap(int cap) {
    MinHeap *h = (MinHeap *)malloc(sizeof(MinHeap));
    h->data = (HeapNode *)malloc(sizeof(HeapNode) * cap);
    h->size = 0;
    h->capacity = cap;
    return h;
}

static void heapSwap(HeapNode *a, HeapNode *b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

static void pushHeap(MinHeap *h, int cost, int idx) {
    if (h->size == h->capacity) {
        h->capacity *= 2;
        h->data = (HeapNode *)realloc(h->data, sizeof(HeapNode) * h->capacity);
    }
    int i = h->size++;
    h->data[i].cost = cost;
    h->data[i].stationIndex = idx;

    // bubble up
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].cost <= h->data[i].cost) break;
        heapSwap(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

static HeapNode popHeap(MinHeap *h) {
    HeapNode top = h->data[0];
    h->data[0] = h->data[--h->size];

    // bubble down
    int i = 0;
    while (1) {
        int left = 2*i+1, right = 2*i+2, smallest = i;
        if (left  < h->size && h->data[left].cost  < h->data[smallest].cost) smallest = left;
        if (right < h->size && h->data[right].cost < h->data[smallest].cost) smallest = right;
        if (smallest == i) break;
        heapSwap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
    return top;
}

static int isHeapEmpty(MinHeap *h) { return h->size == 0; }

static void freeMinHeap(MinHeap *h) {
    free(h->data);
    free(h);
}

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

    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) return;

    e->destination = to;
    e->distance = dist;
    e->time = time;
    e->fare = fare;
    e->next = g->adjList[from];
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

// find shortest path, mode = "time" / "fare" / else = min stops
PathResult dijkstra(Graph *g, int src, int dst, const char *mode) {
    PathResult result;
    result.path = NULL;
    result.pathLen = 0;
    result.totalDist10 = 0;
    result.totalTime = 0;
    result.totalFare = 0;

    int n = g->stationCount;
    if (src < 0 || dst < 0 || src >= n || dst >= n) return result;

    int *dist = (int *)malloc(sizeof(int) * n);
    int *prev = (int *)malloc(sizeof(int) * n);
    double *distKm = (double *)malloc(sizeof(double) * n);
    int *distT = (int *)malloc(sizeof(int) * n);
    int *distF = (int *)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        prev[i] = -1;
        distKm[i] = 0.0;
        distT[i] = 0;
        distF[i] = 0;
    }
    dist[src] = 0;

    MinHeap *pq = createMinHeap(n * 2 + 16);
    pushHeap(pq, 0, src);

    while (!isHeapEmpty(pq)) {
        HeapNode cur = popHeap(pq);
        int u = cur.stationIndex;

        if (cur.cost > dist[u]) continue;
        if (u == dst) break;

        Edge *e = g->adjList[u];
        while (e) {
            int v = e->destination;
            if (g->stations[v].isClosed && v != dst) {
                e = e->next;
                continue;
            }

            int w;
            if (strcmp(mode, "time") == 0)
                w = e->time;
            else if (strcmp(mode, "fare") == 0)
                w = e->fare;
            else
                w = 1;

            int newCost = dist[u] + w;
            if (newCost < dist[v]) {
                dist[v] = newCost;
                prev[v] = u;
                distKm[v] = distKm[u] + e->distance;
                distT[v] = distT[u] + e->time;
                distF[v] = distF[u] + e->fare;
                pushHeap(pq, newCost, v);
            }
            e = e->next;
        }
    }
    freeMinHeap(pq);

    if (dist[dst] == INF) {
        free(dist); free(prev); free(distKm); free(distT); free(distF);
        return result;
    }

    // trace back path
    int len = 0;
    for (int v = dst; v != -1; v = prev[v]) len++;

    int *path = (int *)malloc(sizeof(int) * len);
    int pos = len - 1;
    for (int v = dst; v != -1; v = prev[v])
        path[pos--] = v;

    result.path = path;
    result.pathLen = len;
    result.totalDist10 = (int)(distKm[dst] * 10.0 + 0.5);
    result.totalTime = distT[dst];
    result.totalFare = distF[dst];

    // minimum fare 17 THB
    if (result.totalFare < 17 && result.pathLen > 1)
        result.totalFare = 17;

    free(dist); free(prev); free(distKm); free(distT); free(distF);
    return result;
}

void displayPath(const Graph *g, const PathResult *pr) {
    if (!pr->path || pr->pathLen == 0) {
        printf("  [!] No route found.\n");
        return;
    }

    printf("\n  Route: ");
    for (int i = 0; i < pr->pathLen; i++) {
        const Station *st = &g->stations[pr->path[i]];
        if (st->code[0])
            printf("[%s] %s", st->code, st->name);
        else
            printf("%s", st->name);
        if (i < pr->pathLen - 1) printf(" -> ");
    }

    // count line changes
    int interchanges = 0;
    const char *currentLine = g->stations[pr->path[0]].line;
    for (int i = 1; i < pr->pathLen; i++) {
        int from = pr->path[i - 1];
        int to = pr->path[i];
        const char *nextLine = g->stations[to].line;

        int isWalk = 0;
        Edge *e = g->adjList[from];
        while (e) {
            if (e->destination == to) {
                if (e->distance == 0.0) isWalk = 1;
                break;
            }
            e = e->next;
        }

        if (isWalk || strcmp(nextLine, currentLine) != 0) {
            interchanges++;
            currentLine = nextLine;
        }
    }

    printf("\n");
    printf("  Stations    : %d stop(s)\n", pr->pathLen - 1);
    printf("  Distance    : %.1f km\n", pr->totalDist10 / 10.0);
    printf("  Time        : ~%d minutes\n", pr->totalTime);
    printf("  Fare        : %d THB\n", pr->totalFare);
    printf("  Line changes: %d\n", interchanges);

    printf("\n  Detailed route:\n");
    for (int i = 0; i < pr->pathLen; i++) {
        int idx = pr->path[i];
        printf("    [%2d] [%-5s] %-35s (%s)%s\n",
               i + 1,
               g->stations[idx].code[0] ? g->stations[idx].code : "-",
               g->stations[idx].name,
               g->stations[idx].line,
               g->stations[idx].isInterchange ? " [INTERCHANGE]" : "");
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
