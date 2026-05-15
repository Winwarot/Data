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

#define FARE_TYPE_NONE 0
#define FARE_TYPE_BTS  1
#define FARE_TYPE_MRT  2
#define FARE_TYPE_ARL  3
#define FARE_TYPE_COUNT 4
#define MAX_FARE_PASS 20

static const char *fareTypeName[FARE_TYPE_COUNT] = {
    "",
    "BTS",
    "MRT",
    "Airport Rail Link"
};

static const char *mapLineToFareType(const char *lineName) {
    if (lineName && strstr(lineName, "Airport Rail Link")) return "Airport Rail Link";
    if (lineName && strstr(lineName, "MRT")) return "MRT";
    if (lineName && strstr(lineName, "BTS")) return "BTS";
    return "BTS";
}

static int mapLineToFareTypeId(const char *lineName) {
    if (lineName && strstr(lineName, "Airport Rail Link")) return FARE_TYPE_ARL;
    if (lineName && strstr(lineName, "MRT")) return FARE_TYPE_MRT;
    if (lineName && strstr(lineName, "BTS")) return FARE_TYPE_BTS;
    return FARE_TYPE_BTS;
}

static int lookupFare(const Graph *g, const char *fareType, int stationsPassed) {
    int maxPass = -1;
    int maxFare = 17;

    if (stationsPassed < 0) stationsPassed = 0;

    for (int i = 0; i < g->fareTable.count; i++) {
        FareEntry fe = g->fareTable.entries[i];

        if (strcmp(fe.lineType, fareType) != 0) continue;

        if (fe.stationsPassed == stationsPassed)
            return fe.fare;

        if (fe.stationsPassed > maxPass) {
            maxPass = fe.stationsPassed;
            maxFare = fe.fare;
        }
    }

    if (stationsPassed > maxPass) return maxFare;
    return 17;
}

static int calculatePathFare(const Graph *g, const int *path, int pathLen) {
    int totalFare = 0;
    const char *currentType = NULL;
    int stationsPassed = 0;

    if (!path || pathLen <= 1) return 0;

    for (int i = 1; i < pathLen; i++) {
        int from = path[i - 1];
        int to = path[i];
        Edge *e = g->adjList[from];
        const char *fromType;
        const char *nextType;

        while (e && e->destination != to) e = e->next;
        if (!e) continue;

        if (e->distance == 0.0) {
            currentType = NULL;
            stationsPassed = 0;
            continue;
        }

        fromType = mapLineToFareType(g->stations[from].line);
        nextType = mapLineToFareType(g->stations[to].line);

        if (strcmp(fromType, nextType) != 0) {
            if (!currentType) {
                currentType = nextType;
                stationsPassed = 1;
                totalFare += lookupFare(g, currentType, stationsPassed);
            } else if (i == pathLen - 1) {
                int oldFare = lookupFare(g, currentType, stationsPassed);
                stationsPassed++;
                totalFare += lookupFare(g, currentType, stationsPassed) - oldFare;
            } else {
                currentType = NULL;
                stationsPassed = 0;
            }
            continue;
        }

        if (!currentType || strcmp(currentType, nextType) != 0) {
            currentType = nextType;
            stationsPassed = 1;
            totalFare += lookupFare(g, currentType, stationsPassed);
        } else {
            int oldFare = lookupFare(g, currentType, stationsPassed);
            stationsPassed++;
            totalFare += lookupFare(g, currentType, stationsPassed) - oldFare;
        }
    }

    if (totalFare < 17 && pathLen > 1) totalFare = 17;
    return totalFare;
}

static PathResult dijkstraFare(Graph *g, int src, int dst) {
    PathResult result;
    int n = g->stationCount;
    int dist[MAX_STATIONS][FARE_TYPE_COUNT][MAX_FARE_PASS];
    int prevStation[MAX_STATIONS][FARE_TYPE_COUNT][MAX_FARE_PASS];
    int prevType[MAX_STATIONS][FARE_TYPE_COUNT][MAX_FARE_PASS];
    int prevPass[MAX_STATIONS][FARE_TYPE_COUNT][MAX_FARE_PASS];
    int used[MAX_STATIONS][FARE_TYPE_COUNT][MAX_FARE_PASS];
    int bestStation = -1, bestType = -1, bestPass = -1;

    result.path = NULL;
    result.pathLen = 0;
    result.totalDist10 = 0;
    result.totalTime = 0;
    result.totalFare = 0;

    for (int s = 0; s < n; s++) {
        for (int t = 0; t < FARE_TYPE_COUNT; t++) {
            for (int p = 0; p < MAX_FARE_PASS; p++) {
                dist[s][t][p] = INF;
                prevStation[s][t][p] = -1;
                prevType[s][t][p] = -1;
                prevPass[s][t][p] = -1;
                used[s][t][p] = 0;
            }
        }
    }

    dist[src][FARE_TYPE_NONE][0] = 0;

    while (1) {
        int minCost = INF;

        bestStation = -1;
        bestType = -1;
        bestPass = -1;

        for (int s = 0; s < n; s++) {
            for (int t = 0; t < FARE_TYPE_COUNT; t++) {
                for (int p = 0; p < MAX_FARE_PASS; p++) {
                    if (used[s][t][p]) continue;
                    if (dist[s][t][p] < minCost) {
                        minCost = dist[s][t][p];
                        bestStation = s;
                        bestType = t;
                        bestPass = p;
                    }
                }
            }
        }

        if (bestStation == -1) break;
        used[bestStation][bestType][bestPass] = 1;
        if (bestStation == dst) break;

        for (Edge *e = g->adjList[bestStation]; e; e = e->next) {
            int v = e->destination;
            int nextType = bestType;
            int nextPass = bestPass;
            int newCost = dist[bestStation][bestType][bestPass];
            int fromType = mapLineToFareTypeId(g->stations[bestStation].line);
            int toType = mapLineToFareTypeId(g->stations[v].line);
            int continueRide = 0;
            int startRide = 0;

            if (g->stations[v].isClosed && v != dst) continue;

            if (e->distance == 0.0) {
                nextType = FARE_TYPE_NONE;
                nextPass = 0;
            } else {
                if (fromType != toType) {
                    if (bestType == FARE_TYPE_NONE) startRide = 1;
                    else if (v == dst) continueRide = 1;
                    else {
                        nextType = FARE_TYPE_NONE;
                        nextPass = 0;
                    }
                } else {
                    if (bestType == FARE_TYPE_NONE || bestType != toType) startRide = 1;
                    else continueRide = 1;
                }

                if (startRide) {
                    nextType = toType;
                    nextPass = 1;
                    newCost += lookupFare(g, fareTypeName[nextType], nextPass);
                } else if (continueRide) {
                    int oldFare = lookupFare(g, fareTypeName[bestType], bestPass);
                    nextType = bestType;
                    nextPass = bestPass + 1;
                    if (nextPass >= MAX_FARE_PASS) nextPass = MAX_FARE_PASS - 1;
                    newCost += lookupFare(g, fareTypeName[nextType], nextPass) - oldFare;
                }
            }

            if (newCost < dist[v][nextType][nextPass]) {
                dist[v][nextType][nextPass] = newCost;
                prevStation[v][nextType][nextPass] = bestStation;
                prevType[v][nextType][nextPass] = bestType;
                prevPass[v][nextType][nextPass] = bestPass;
            }
        }
    }

    bestType = -1;
    bestPass = -1;
    for (int t = 0; t < FARE_TYPE_COUNT; t++) {
        for (int p = 0; p < MAX_FARE_PASS; p++) {
            if (dist[dst][t][p] == INF) continue;
            if (bestType == -1 || dist[dst][t][p] < dist[dst][bestType][bestPass]) {
                bestType = t;
                bestPass = p;
            }
        }
    }

    if (bestType == -1) return result;

    int revPath[MAX_STATIONS];
    int len = 0;
    int curStation = dst;
    int curType = bestType;
    int curPass = bestPass;

    while (curStation != -1 && len < MAX_STATIONS) {
        revPath[len++] = curStation;

        {
            int ps = prevStation[curStation][curType][curPass];
            int pt = prevType[curStation][curType][curPass];
            int pp = prevPass[curStation][curType][curPass];
            curStation = ps;
            curType = pt;
            curPass = pp;
        }
    }

    result.path = (int *)malloc(sizeof(int) * len);
    if (!result.path) return result;

    result.pathLen = len;
    for (int i = 0; i < len; i++)
        result.path[i] = revPath[len - 1 - i];

    for (int i = 1; i < result.pathLen; i++) {
        Edge *e = g->adjList[result.path[i - 1]];
        while (e && e->destination != result.path[i]) e = e->next;
        if (!e) continue;
        result.totalDist10 += (int)(e->distance * 10.0 + 0.5);
        result.totalTime += e->time;
    }

    result.totalFare = calculatePathFare(g, result.path, result.pathLen);
    return result;
}

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

// find shortest path
// stops = 1 per edge
// time  = use e->time
// fare  = find path first, then calculate real fare from fare.csv
PathResult dijkstra(Graph *g, int src, int dst, const char *mode) {
    PathResult result;
    result.path = NULL;
    result.pathLen = 0;
    result.totalDist10 = 0;
    result.totalTime = 0;
    result.totalFare = 0;

    int n = g->stationCount;
    if (src < 0 || dst < 0 || src >= n || dst >= n) return result;
    if (strcmp(mode, "fare") == 0) return dijkstraFare(g, src, dst);

    int *dist = (int *)malloc(sizeof(int) * n);
    int *prev = (int *)malloc(sizeof(int) * n);
    double *distKm = (double *)malloc(sizeof(double) * n);
    int *distT = (int *)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        prev[i] = -1;
        distKm[i] = 0.0;
        distT[i] = 0;
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
            else
                w = 1;

            int newCost = dist[u] + w;
            if (newCost < dist[v]) {
                dist[v] = newCost;
                prev[v] = u;
                distKm[v] = distKm[u] + e->distance;
                distT[v] = distT[u] + e->time;
                pushHeap(pq, newCost, v);
            }
            e = e->next;
        }
    }
    freeMinHeap(pq);

    if (dist[dst] == INF) {
        free(dist); free(prev); free(distKm); free(distT);
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
    result.totalFare = calculatePathFare(g, result.path, result.pathLen);

    free(dist); free(prev); free(distKm); free(distT);
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

    // count line changes from the shown route
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
        int showInterchange = 0;

        if (g->stations[idx].isInterchange && i > 0 && i < pr->pathLen - 1) {
            const char *prevLine = g->stations[pr->path[i - 1]].line;
            const char *curLine = g->stations[idx].line;
            const char *nextLine = g->stations[pr->path[i + 1]].line;

            if (strcmp(prevLine, curLine) != 0 || strcmp(curLine, nextLine) != 0)
                showInterchange = 1;
        }

        printf("    [%2d] [%-5s] %-35s (%s)%s\n",
               i + 1,
               g->stations[idx].code[0] ? g->stations[idx].code : "-",
               g->stations[idx].name,
               g->stations[idx].line,
               showInterchange ? " [INTERCHANGE]" : "");
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
