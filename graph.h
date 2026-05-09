#ifndef GRAPH_H
#define GRAPH_H

#include "station.h"
#include "minheap.h"

#define FARE_TYPE_BTS 0
#define FARE_TYPE_MRT 1
#define FARE_TYPE_ARL 2

/* result of one dijkstra query */
struct PathResult{
  int *path;
  int pathLen;
  int totalDist10;
  int totalTime;
  int totalFare;
};

/* graph stored as adjacency list:
   stations[i] is the node, adjList[i] is the linked list of edges */
struct Graph{
  Station stations[MAX_STATIONS];
  Edge *adjList[MAX_STATIONS];
  int stationCount;
};

struct Graph *createGraph();
void freeGraph(struct Graph *g);

int graphAddStation(struct Graph *g, const char *name, const char *line, int isInterchange);
void graphAddEdge(struct Graph *g, int from, int to, double dist, int time, int fare);
void graphAddUndirectedEdge(struct Graph *g, int a, int b, double dist, int time, int fare);

void graphCloseStation(struct Graph *g, int idx);
void graphReopenStation(struct Graph *g, int idx);
void graphRemoveStation(struct Graph *g, int idx);

struct PathResult dijkstra(struct Graph *g, int src, int dst, const char *mode);
void displayPath(const struct Graph *g, const struct PathResult *pr);
void graphDisplay(const struct Graph *g);

int getFareType(const char *lineName);
int calcLineFare(int fareType, int numStops);

#endif
