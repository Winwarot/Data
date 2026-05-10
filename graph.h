#ifndef GRAPH_H
#define GRAPH_H

#include "station.h"

typedef struct Graph {
    Station stations[MAX_STATIONS];
    Edge *adjList[MAX_STATIONS];
    int stationCount;
} Graph;

Graph  *createGraph(void);
int     graphAddStation(Graph *g, const char *code, const char *name, const char *line, int isInterchange);
void    graphAddEdge(Graph *g, int from, int to, double dist, int time, int fare);
void    graphAddUndirectedEdge(Graph *g, int a, int b, double dist, int time, int fare);
void    graphCloseStation(Graph *g, int idx);
void    graphReopenStation(Graph *g, int idx);
void    graphRemoveStation(Graph *g, int idx);
void    graphDisplay(const Graph *g);
void    freeGraph(Graph *g);

#endif
