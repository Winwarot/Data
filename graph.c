#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fare tables - hardcoded so no file I/O */
int fareBTS[] = {17, 17, 25, 28, 32, 35, 40, 43, 47};
int fareMRT[] = {17, 17, 20, 22, 25, 27, 30, 32, 35, 37, 40, 42, 45};
int fareARL[] = {15, 15, 20, 25, 30, 35, 40, 45};
#define FARE_BTS_MAX 8
#define FARE_MRT_MAX 12
#define FARE_ARL_MAX 7

/* allocate a new empty graph */
struct Graph *createGraph(){
  struct Graph *g = (struct Graph *)calloc(1, sizeof(struct Graph));
  if(g == NULL){
    printf("graph alloc failed\n");
    exit(1);
  }
  g->stationCount = 0;
  return g;
}

/* free every edge in every adjacency list, then the graph itself */
void freeGraph(struct Graph *g){
  if(g == NULL) return;
  for(int i = 0; i < g->stationCount; i++){
    Edge *cur = g->adjList[i];
    while(cur != NULL){
      Edge *next = cur->next;
      free(cur);
      cur = next;
    }
  }
  free(g);
}

/* add a node to the graph, return its index (-1 if full) */
int graphAddStation(struct Graph *g, const char *name, const char *line, int isInterchange){
  if(g->stationCount >= MAX_STATIONS) return -1;
  int idx = g->stationCount;
  Station *s = &g->stations[idx];
  s->id = idx;
  s->isInterchange = isInterchange;
  s->isClosed = 0;
  strncpy(s->name, name, MAX_NAME_LEN - 1);
  s->name[MAX_NAME_LEN - 1] = '\0';
  strncpy(s->line, line, MAX_LINE_LEN - 1);
  s->line[MAX_LINE_LEN - 1] = '\0';
  g->adjList[idx] = NULL;
  g->stationCount++;
  return idx;
}

/* add a directed edge by inserting a new node at the head of the linked list */
void graphAddEdge(struct Graph *g, int from, int to, double dist, int time, int fare){
  if(from < 0 || to < 0) return;
  if(from >= g->stationCount || to >= g->stationCount) return;
  Edge *e = (Edge *)malloc(sizeof(Edge));
  if(e == NULL) return;
  e->destination = to;
  e->distance = dist;
  e->time = time;
  e->fare = fare;
  e->next = g->adjList[from];
  g->adjList[from] = e;
}

/* undirected = two directed edges */
void graphAddUndirectedEdge(struct Graph *g, int a, int b, double dist, int time, int fare){
  graphAddEdge(g, a, b, dist, time, fare);
  graphAddEdge(g, b, a, dist, time, fare);
}

void graphCloseStation(struct Graph *g, int idx){
  if(idx >= 0 && idx < g->stationCount) g->stations[idx].isClosed = 1;
}

void graphReopenStation(struct Graph *g, int idx){
  if(idx >= 0 && idx < g->stationCount) g->stations[idx].isClosed = 0;
}

/* delete a node and every edge that touches it, then shift the array left */
void graphRemoveStation(struct Graph *g, int idx){
  if(idx < 0 || idx >= g->stationCount) return;

  /* free outgoing edges */
  Edge *cur = g->adjList[idx];
  while(cur != NULL){
    Edge *next = cur->next;
    free(cur);
    cur = next;
  }
  g->adjList[idx] = NULL;

  /* free incoming edges (linked-list deletion using prev pointer) */
  for(int i = 0; i < g->stationCount; i++){
    Edge *prev = NULL;
    Edge *e = g->adjList[i];
    while(e != NULL){
      if(e->destination == idx){
        Edge *toDelete = e;
        if(prev == NULL) g->adjList[i] = e->next;
        else prev->next = e->next;
        e = e->next;
        free(toDelete);
      }
      else{
        prev = e;
        e = e->next;
      }
    }
  }

  /* shift everything after idx down by 1 to keep indices contiguous */
  for(int i = idx; i < g->stationCount - 1; i++){
    g->stations[i] = g->stations[i + 1];
    g->stations[i].id = i;
    g->adjList[i] = g->adjList[i + 1];
  }
  g->stationCount--;

  /* fix up edge destination indices that were shifted */
  for(int i = 0; i < g->stationCount; i++){
    Edge *e = g->adjList[i];
    while(e != NULL){
      if(e->destination > idx) e->destination--;
      e = e->next;
    }
  }
}

/* dijkstra's shortest-path algorithm using a min-heap as priority queue.
   mode picks which weight to minimize:
     "stops" - number of edges (each edge = 1)
     "time"  - travel minutes
     "fare"  - per-edge fare (will be replaced by table lookup in Push 2) */
struct PathResult dijkstra(struct Graph *g, int src, int dst, const char *mode){
  struct PathResult result;
  result.path = NULL;
  result.pathLen = 0;
  result.totalDist10 = 0;
  result.totalTime = 0;
  result.totalFare = 0;

  int n = g->stationCount;
  if(src < 0 || dst < 0 || src >= n || dst >= n) return result;

  /* dist[v] = best cost so far, prev[v] = which node we came from */
  int *dist = (int *)malloc(sizeof(int) * n);
  int *prev = (int *)malloc(sizeof(int) * n);
  double *distKm = (double *)malloc(sizeof(double) * n);
  int *distT = (int *)malloc(sizeof(int) * n);
  int *distF = (int *)malloc(sizeof(int) * n);

  for(int i = 0; i < n; i++){
    dist[i] = INF;
    prev[i] = -1;
    distKm[i] = 0.0;
    distT[i] = 0;
    distF[i] = 0;
  }
  dist[src] = 0;

  MinHeap *pq = createMinHeap(n * 2 + 16);
  pushHeap(pq, 0, src);

  /* main loop: always expand the cheapest unfinished node */
  while(!isHeapEmpty(pq)){
    HeapNode cur = popHeap(pq);
    int u = cur.stationIndex;
    if(cur.cost > dist[u]) continue;   /* stale entry, skip */
    if(u == dst) break;

    /* relax every outgoing edge of u */
    Edge *e = g->adjList[u];
    while(e != NULL){
      int v = e->destination;
      if(g->stations[v].isClosed && v != dst){
        e = e->next;
        continue;
      }

      int w;
      if(strcmp(mode, "time") == 0) w = e->time;
      else if(strcmp(mode, "fare") == 0) w = e->fare;
      else w = 1;

      int newCost = dist[u] + w;
      if(newCost < dist[v]){
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

  if(dist[dst] == INF){
    free(dist); free(prev); free(distKm); free(distT); free(distF);
    return result;
  }

  /* rebuild the path by walking prev[] from dst back to src */
  int len = 0;
  for(int v = dst; v != -1; v = prev[v]) len++;

  int *path = (int *)malloc(sizeof(int) * len);
  int pos = len - 1;
  for(int v = dst; v != -1; v = prev[v]) path[pos--] = v;

  result.path = path;
  result.pathLen = len;
  result.totalDist10 = (int)(distKm[dst] * 10.0 + 0.5);
  result.totalTime = distT[dst];
  result.totalFare = distF[dst];

  free(dist); free(prev); free(distKm); free(distT); free(distF);
  return result;
}

/* print a route summary */
void displayPath(const struct Graph *g, const struct PathResult *pr){
  if(pr->path == NULL || pr->pathLen == 0){
    printf("  no route found\n");
    return;
  }
  printf("\n  Route: ");
  for(int i = 0; i < pr->pathLen; i++){
    printf("%s", g->stations[pr->path[i]].name);
    if(i < pr->pathLen - 1) printf(" -> ");
  }
  printf("\n");
  printf("  Stations : %d stop(s)\n", pr->pathLen - 1);
  printf("  Distance : %.1f km\n", pr->totalDist10 / 10.0);
  printf("  Time     : ~%d min\n", pr->totalTime);
  printf("  Fare     : %d THB\n", pr->totalFare);
}

/* dump the whole adjacency list for debugging */
void graphDisplay(const struct Graph *g){
  printf("\n  Graph (%d stations)\n", g->stationCount);
  for(int i = 0; i < g->stationCount; i++){
    printf("  [%3d] %-40s -> ", i, g->stations[i].name);
    Edge *e = g->adjList[i];
    if(e == NULL){ printf("(none)\n"); continue; }
    while(e != NULL){
      printf("%s(%.1fkm,%dmin,%dTHB) ",
             g->stations[e->destination].name,
             e->distance, e->time, e->fare);
      e = e->next;
    }
    printf("\n");
  }
}

/* map line name to fare-table type using simple substring match */
int getFareType(const char *lineName){
  if(lineName == NULL) return FARE_TYPE_MRT;
  if(strstr(lineName, "BTS") != NULL) return FARE_TYPE_BTS;
  if(strstr(lineName, "Airport") != NULL) return FARE_TYPE_ARL;
  return FARE_TYPE_MRT;
}

/* table lookup: clamp the index, then read the array */
int calcLineFare(int fareType, int numStops){
  if(numStops < 0) numStops = 0;
  if(fareType == FARE_TYPE_BTS){
    if(numStops > FARE_BTS_MAX) numStops = FARE_BTS_MAX;
    return fareBTS[numStops];
  }
  if(fareType == FARE_TYPE_ARL){
    if(numStops > FARE_ARL_MAX) numStops = FARE_ARL_MAX;
    return fareARL[numStops];
  }
  if(numStops > FARE_MRT_MAX) numStops = FARE_MRT_MAX;
  return fareMRT[numStops];
}
