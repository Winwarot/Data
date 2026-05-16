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

static MinHeap *createMinHeap(int cap);
static void pushHeap(MinHeap *h, int cost, int idx);
static HeapNode popHeap(MinHeap *h);
static int isHeapEmpty(MinHeap *h);
static void freeMinHeap(MinHeap *h);

enum {
    FARE_TYPE_NONE = 0,
    FARE_TYPE_BTS,
    FARE_TYPE_MRT,
    FARE_TYPE_ARL,
    FARE_TYPE_COUNT
};

static const char *fareTypeIdToString(int fareTypeId) {
    switch (fareTypeId) {
        case FARE_TYPE_BTS: return "BTS";
        case FARE_TYPE_MRT: return "MRT";
        case FARE_TYPE_ARL: return "Airport Rail Link";
        default:            return "BTS";
    }
}

static int fareTypeToId(const char *fareType) {
    if (!fareType || fareType[0] == '\0') return FARE_TYPE_BTS;
    if (strcmp(fareType, "BTS") == 0) return FARE_TYPE_BTS;
    if (strcmp(fareType, "MRT") == 0) return FARE_TYPE_MRT;
    if (strcmp(fareType, "Airport Rail Link") == 0) return FARE_TYPE_ARL;
    return FARE_TYPE_BTS;
}

static const char *mapLineToFareType(const char *lineName) {
    if (!lineName || lineName[0] == '\0') return "BTS";
    if (strstr(lineName, "Airport Rail Link")) return "Airport Rail Link";
    if (strstr(lineName, "BTS")) return "BTS";
    if (strstr(lineName, "MRT")) return "MRT";
    return "BTS";
}

static int maxStationsPassedForFareType(const Graph *g, const char *fareType) {
    int maxStations = -1;

    for (int i = 0; i < g->fareTable.count; i++) {
        const FareEntry *entry = &g->fareTable.entries[i];
        if (strcmp(entry->lineType, fareType) == 0 && entry->stationsPassed > maxStations)
            maxStations = entry->stationsPassed;
    }

    return maxStations;
}

static int maxStationsPassedAcrossFareTable(const Graph *g) {
    int maxStations = 0;

    for (int i = 0; i < g->fareTable.count; i++) {
        if (g->fareTable.entries[i].stationsPassed > maxStations)
            maxStations = g->fareTable.entries[i].stationsPassed;
    }

    return maxStations;
}

static int clampStationsPassed(const Graph *g, int fareTypeId, int stationsPassed) {
    int maxStations = maxStationsPassedForFareType(g, fareTypeIdToString(fareTypeId));

    if (maxStations < 0) return 0;
    if (stationsPassed < 0) return 0;
    if (stationsPassed > maxStations) return maxStations;
    return stationsPassed;
}

int lookupFare(const Graph *g, const char *fareType, int stationsPassed) {
    int cappedStations = stationsPassed;
    int maxStations = maxStationsPassedForFareType(g, fareType);

    if (maxStations >= 0 && cappedStations > maxStations)
        cappedStations = maxStations;
    if (cappedStations < 0)
        cappedStations = 0;

    for (int i = 0; i < g->fareTable.count; i++) {
        const FareEntry *entry = &g->fareTable.entries[i];
        if (strcmp(entry->lineType, fareType) == 0 && entry->stationsPassed == cappedStations)
            return entry->fare;
    }

    return 17;
}

static const Edge *findEdge(const Graph *g, int from, int to) {
    const Edge *e = g->adjList[from];
    while (e) {
        if (e->destination == to) return e;
        e = e->next;
    }
    return NULL;
}

static const char *resolveEdgeFareType(const Graph *g, int from, int to) {
    const char *fromType = mapLineToFareType(g->stations[from].line);
    const char *toType = mapLineToFareType(g->stations[to].line);

    if (strcmp(fromType, toType) == 0) return fromType;
    if (g->stations[from].line[0] != '\0' && strstr(g->stations[from].line, "Interchange") == NULL) return fromType;
    return toType;
}

int calculatePathFare(const Graph *g, const int *path, int pathLen) {
    int totalFare = 0;
    char currentFareType[MAX_LINE_LEN] = "";
    int currentStationsPassed = 0;
    int hasActiveSegment = 0;

    if (!g || !path || pathLen <= 1) return 0;

    for (int i = 1; i < pathLen; i++) {
        int from = path[i - 1];
        int to = path[i];
        const Edge *e = findEdge(g, from, to);
        const char *nextFareType;

        if (!e) continue;

        /*
         * Transfer / walk links are represented with distance == 0.0.
         * They do not add fare and they end the current paid segment,
         * so the next paid edge starts a new fare calculation.
         */
        if (e->distance == 0.0) {
            hasActiveSegment = 0;
            currentFareType[0] = '\0';
            currentStationsPassed = 0;
            continue;
        }

        nextFareType = resolveEdgeFareType(g, from, to);

        if (!hasActiveSegment) {
            strncpy(currentFareType, nextFareType, MAX_LINE_LEN - 1);
            currentFareType[MAX_LINE_LEN - 1] = '\0';
            currentStationsPassed = 0;
            totalFare += lookupFare(g, currentFareType, currentStationsPassed);
            hasActiveSegment = 1;
            continue;
        }

        if (strcmp(currentFareType, nextFareType) == 0) {
            int previousFare = lookupFare(g, currentFareType, currentStationsPassed);
            currentStationsPassed++;
            int updatedFare = lookupFare(g, currentFareType, currentStationsPassed);
            totalFare += updatedFare - previousFare;
            continue;
        }

        strncpy(currentFareType, nextFareType, MAX_LINE_LEN - 1);
        currentFareType[MAX_LINE_LEN - 1] = '\0';
        currentStationsPassed = 0;
        totalFare += lookupFare(g, currentFareType, currentStationsPassed);
        hasActiveSegment = 1;
    }

    if (totalFare < 17 && pathLen > 1)
        totalFare = 17;

    return totalFare;
}

static int encodeFareState(int stationIndex, int fareTypeId, int stationsPassed, int passSpan) {
    return ((stationIndex * FARE_TYPE_COUNT) + fareTypeId) * passSpan + stationsPassed;
}

static void decodeFareState(int stateIndex, int passSpan, int *stationIndex, int *fareTypeId, int *stationsPassed) {
    *stationsPassed = stateIndex % passSpan;
    stateIndex /= passSpan;
    *fareTypeId = stateIndex % FARE_TYPE_COUNT;
    *stationIndex = stateIndex / FARE_TYPE_COUNT;
}

static PathResult dijkstraByFare(Graph *g, int src, int dst) {
    PathResult result;
    int n = g->stationCount;
    int passSpan = maxStationsPassedAcrossFareTable(g) + 1;
    int stateCount;
    int *dist;
    int *prev;
    int bestState = -1;

    result.path = NULL;
    result.pathLen = 0;
    result.totalFare = 0;

    if (src < 0 || dst < 0 || src >= n || dst >= n) return result;
    if (passSpan <= 0) passSpan = 1;

    stateCount = n * FARE_TYPE_COUNT * passSpan;
    dist = (int *)malloc(sizeof(int) * stateCount);
    prev = (int *)malloc(sizeof(int) * stateCount);
    if (!dist || !prev) {
        free(dist);
        free(prev);
        return result;
    }

    for (int i = 0; i < stateCount; i++) {
        dist[i] = INF;
        prev[i] = -1;
    }

    MinHeap *pq = createMinHeap(stateCount / 4 + 16);
    int srcState = encodeFareState(src, FARE_TYPE_NONE, 0, passSpan);
    dist[srcState] = 0;
    pushHeap(pq, 0, srcState);

    while (!isHeapEmpty(pq)) {
        HeapNode cur = popHeap(pq);
        int state = cur.stationIndex;
        int u, fareTypeId, stationsPassed;

        if (cur.cost > dist[state]) continue;

        decodeFareState(state, passSpan, &u, &fareTypeId, &stationsPassed);
        if (u == dst) break;

        for (Edge *e = g->adjList[u]; e; e = e->next) {
            int v = e->destination;
            int nextFareTypeId = fareTypeId;
            int nextStationsPassed = stationsPassed;
            int newCost = dist[state];

            if (g->stations[v].isClosed && v != dst) continue;

            if (e->distance == 0.0) {
                nextFareTypeId = FARE_TYPE_NONE;
                nextStationsPassed = 0;
            } else {
                const char *nextFareType = resolveEdgeFareType(g, u, v);
                nextFareTypeId = fareTypeToId(nextFareType);

                if (fareTypeId == FARE_TYPE_NONE) {
                    nextStationsPassed = 0;
                    newCost += lookupFare(g, nextFareType, nextStationsPassed);
                } else if (nextFareTypeId == fareTypeId) {
                    int previousStationsPassed = clampStationsPassed(g, fareTypeId, stationsPassed);
                    int updatedStationsPassed = clampStationsPassed(g, fareTypeId, stationsPassed + 1);
                    int previousFare = lookupFare(g, fareTypeIdToString(fareTypeId), previousStationsPassed);
                    int updatedFare = lookupFare(g, fareTypeIdToString(fareTypeId), updatedStationsPassed);

                    nextStationsPassed = updatedStationsPassed;
                    newCost += updatedFare - previousFare;
                } else {
                    nextStationsPassed = 0;
                    newCost += lookupFare(g, nextFareType, nextStationsPassed);
                }
            }

            int nextState = encodeFareState(v, nextFareTypeId, nextStationsPassed, passSpan);
            if (newCost < dist[nextState]) {
                dist[nextState] = newCost;
                prev[nextState] = state;
                pushHeap(pq, newCost, nextState);
            }
        }
    }

    freeMinHeap(pq);

    for (int fareTypeId = 0; fareTypeId < FARE_TYPE_COUNT; fareTypeId++) {
        for (int stationsPassed = 0; stationsPassed < passSpan; stationsPassed++) {
            int state = encodeFareState(dst, fareTypeId, stationsPassed, passSpan);
            if (dist[state] == INF) continue;
            if (bestState == -1 || dist[state] < dist[bestState]) bestState = state;
        }
    }

    if (bestState == -1) {
        free(dist);
        free(prev);
        return result;
    }

    int len = 0;
    for (int state = bestState; state != -1; state = prev[state]) len++;

    result.path = (int *)malloc(sizeof(int) * len);
    if (!result.path) {
        free(dist);
        free(prev);
        return result;
    }

    int pos = len - 1;
    for (int state = bestState; state != -1; state = prev[state]) {
        int stationIndex, fareTypeId, stationsPassed;
        decodeFareState(state, passSpan, &stationIndex, &fareTypeId, &stationsPassed);
        result.path[pos--] = stationIndex;
    }

    result.pathLen = len;
    result.totalFare = calculatePathFare(g, result.path, result.pathLen);

    free(dist);
    free(prev);
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

/*
  Find shortest path.
  - by stops: weight = 1 per edge
  - by fare : handled by fare-table-aware state expansion below, because
              fare depends on stations passed per BTS/MRT/ARL segment and
              transfer edges (distance == 0.0) do not add fare.
 */
PathResult dijkstra(Graph *g, int src, int dst, const char *mode) {
    PathResult result;
    result.path = NULL;
    result.pathLen = 0;
    result.totalFare = 0;

    int n = g->stationCount;
    if (src < 0 || dst < 0 || src >= n || dst >= n) return result;
    if (strcmp(mode, "fare") == 0) return dijkstraByFare(g, src, dst);

    int *dist = (int *)malloc(sizeof(int) * n);
    int *prev = (int *)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        prev[i] = -1;
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

            int w = 1;

            int newCost = dist[u] + w;
            if (newCost < dist[v]) {
                dist[v] = newCost;
                prev[v] = u;
                pushHeap(pq, newCost, v);
            }
            e = e->next;
        }
    }
    freeMinHeap(pq);

    if (dist[dst] == INF) {
        free(dist); free(prev);
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
    result.totalFare = calculatePathFare(g, result.path, result.pathLen);

    free(dist); free(prev);
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
            printf("%s(%dTHB) ",
                   g->stations[e->destination].name,
                   e->fare);
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
