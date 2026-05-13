#ifndef GRAPH_H
#define GRAPH_H

#include "station.h"

#define MAX_FARE_ENTRIES 120

typedef struct {
    char lineType[MAX_LINE_LEN];
    int  stationsPassed;
    int  fare;
} FareEntry;

typedef struct {
    FareEntry entries[MAX_FARE_ENTRIES];
    int count;
} FareTable;

typedef struct PathResult {
    int *path;
    int pathLen;
    int totalDist10;
    int totalTime;
    int totalFare;
} PathResult;

typedef struct Graph {
    Station   stations[MAX_STATIONS];
    Edge     *adjList[MAX_STATIONS];
    int       stationCount;
    FareTable fareTable;
} Graph;

Graph *createGraph(void);
int graphAddStation(Graph *g, const char *code, const char *name, const char *line, int isInterchange);
void graphAddEdge(Graph *g, int from, int to, double dist, int time, int fare);
void graphAddUndirectedEdge(Graph *g, int a, int b, double dist, int time, int fare);
void graphCloseStation(Graph *g, int idx);
void graphReopenStation(Graph *g, int idx);
void graphRemoveStation(Graph *g, int idx);
PathResult dijkstra(Graph *g, int src, int dst, const char *mode);
void displayPath(const Graph *g, const PathResult *pr);
void graphDisplay(const Graph *g);
void freeGraph(Graph *g);

#endif
