#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "station.h"
#include "graph.h"
#include "hashtable.h"
#include "bst.h"
#include "data.h"

static void clearInput(){
  int c;
  while((c = getchar()) != '\n' && c != EOF);
}

static void readLine(char *buf, int len){
  if(!fgets(buf, len, stdin)){ buf[0] = '\0'; return; }
  int n = (int)strlen(buf);
  if(n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
}

/* strict int parser: returns 1 only if the whole string is a valid int */
static int parseIntStrict(const char *text, int *out){
  char *end;
  long value;
  if(!text || text[0] == '\0') return 0;
  value = strtol(text, &end, 10);
  if(*end != '\0') return 0;
  *out = (int)value;
  return 1;
}

static int parseNonNegativeDouble(const char *text, double *out){
  char *end;
  double value;
  if(!text || text[0] == '\0') return 0;
  value = strtod(text, &end);
  if(*end != '\0' || value < 0.0) return 0;
  *out = value;
  return 1;
}

/* hash lookup wrapper that prints a friendly error */
static int lookupStation(const HashTable *ht, const char *query){
  int idx = hashSearch(ht, query);
  if(idx < 0)
    printf("  [!] Station \"%s\" not found. Check name or station code.\n", query);
  return idx;
}

static int stationCodeExists(const Graph *g, const char *code){
  if(!code || code[0] == '\0') return 0;
  for(int i = 0; i < g->stationCount; i++){
    if(strcmp(g->stations[i].code, code) == 0) return 1;
  }
  return 0;
}

static int stationNameLineExists(const Graph *g, const char *name, const char *line){
  for(int i = 0; i < g->stationCount; i++){
    if(strcmp(g->stations[i].name, name) == 0 &&
       strcmp(g->stations[i].line, line) == 0)
      return 1;
  }
  return 0;
}

static int compareStationsByName(const Graph *g, int ia, int ib){
  int cmp = strcmp(g->stations[ia].name, g->stations[ib].name);
  if(cmp != 0) return cmp;
  return strcmp(g->stations[ia].code, g->stations[ib].code);
}

/* menu option 1/2: find route (mode = "stops" or "fare") */
static void menuFindRoute(Graph *g, HashTable *ht, const char *mode){
  char src[MAX_NAME_LEN], dst[MAX_NAME_LEN];

  printf("\n  Source station (name or code): ");
  readLine(src, MAX_NAME_LEN);
  printf("  Destination station (name or code): ");
  readLine(dst, MAX_NAME_LEN);

  if(src[0] == '\0' || dst[0] == '\0'){
    printf("  [!] Source and destination cannot be empty.\n");
    return;
  }

  int si = lookupStation(ht, src);
  int di = lookupStation(ht, dst);
  if(si < 0 || di < 0) return;
  if(si == di){ printf("  [!] Source and destination are the same.\n"); return; }

  printf("\n  Searching (mode: %s)...\n", mode);
  PathResult pr = dijkstra(g, si, di, mode);
  displayPath(g, &pr);
  free(pr.path);
}

/* menu option 3: search station - exact match via hash, prefix via BST */
static void menuSearchStation(const Graph *g, const HashTable *ht, const BST *bst){
  char query[MAX_NAME_LEN];
  printf("\n  Search by name, code, or prefix: ");
  readLine(query, MAX_NAME_LEN);

  if(query[0] == '\0'){
    printf("  [!] Search text cannot be empty.\n");
    return;
  }

  int idx = hashSearch(ht, query);
  if(idx >= 0){
    const Station *s = &g->stations[idx];
    printf("\n  === Station Found ===\n");
    printf("  Code        : %s\n", s->code[0] ? s->code : "-");
    printf("  Name        : %s\n", s->name);
    printf("  Line        : %s\n", s->line);
    printf("  Interchange : %s\n", s->isInterchange ? "Yes" : "No");
    printf("  Status      : %s\n", s->isClosed ? "Closed (maintenance)" : "Open");
    printf("  Connections:\n");
    Edge *e = g->adjList[idx];
    if(!e) printf("    (none)\n");
    while(e){
      const Station *nb = &g->stations[e->destination];
      printf("    -> [%-5s] %-30s  %.1f km\n",
             nb->code[0] ? nb->code : "--",
             nb->name, e->distance);
      e = e->next;
    }
    return;
  }

  /* no exact match - fall back to BST prefix search */
  printf("\n  No exact match. Prefix search for \"%s\":\n", query);
  int found = bstSearchPartial(bst, query);
  if(found == 0) printf("  [!] No stations found with that prefix.\n");
  else printf("  Found %d station(s).\n", found);
}

/* menu option 4: show station details + neighbors */
static void menuShowStationDetails(const Graph *g, const HashTable *ht){
  char query[MAX_NAME_LEN];
  printf("\n  Station name or code: ");
  readLine(query, MAX_NAME_LEN);

  int idx = lookupStation(ht, query);
  if(idx < 0) return;

  const Station *s = &g->stations[idx];
  printf("\n  +-----------------------------------------------+\n");
  printf("  |  [%-5s] %-37s|\n", s->code[0] ? s->code : "-", s->name);
  printf("  +-----------------------------------------------+\n");
  printf("  |  Line        : %-30s|\n", s->line);
  printf("  |  Interchange : %-30s|\n", s->isInterchange ? "Yes" : "No");
  printf("  |  Status      : %-30s|\n", s->isClosed ? "Closed (maintenance)" : "Open");
  printf("  +-----------------------------------------------+\n");

  printf("  Neighboring stations:\n");
  Edge *e = g->adjList[idx];
  if(!e){ printf("    (none)\n"); return; }
  while(e){
    const Station *nb = &g->stations[e->destination];
    printf("    -> [%-5s] %-30s  %.1f km%s\n",
           nb->code[0] ? nb->code : "--",
           nb->name, e->distance,
           (e->distance == 0.0) ? "  [INTERCHANGE]" : "");
    e = e->next;
  }
}

/* menu option 5: show all stations A-Z using simple insertion sort on indices */
static void menuShowAllAZ(const Graph *g){
  int *indices = malloc(sizeof(int) * g->stationCount);
  if(!indices){
    printf("  [!] Memory allocation failed.\n");
    return;
  }

  for(int i = 0; i < g->stationCount; i++) indices[i] = i;

  /* insertion sort - small data set, simple to explain */
  for(int i = 1; i < g->stationCount; i++){
    int key = indices[i];
    int j = i - 1;
    while(j >= 0 && compareStationsByName(g, indices[j], key) > 0){
      indices[j + 1] = indices[j];
      j--;
    }
    indices[j + 1] = key;
  }

  printf("\n  === All Stations (A-Z) - %d total ===\n", g->stationCount);
  for(int i = 0; i < g->stationCount; i++){
    const Station *s = &g->stations[indices[i]];
    printf("  %3d. [%-5s] %s%s\n",
           i + 1,
           s->code[0] ? s->code : "-",
           s->name,
           s->isInterchange ? " [IX]" : "");
  }
  free(indices);
}

/* menu option 6: group stations by transit line */
static void menuShowByLine(const Graph *g){
  const char *lines[] = {
    LINE_BTS_SUKHUMVIT, LINE_BTS_SILOM, LINE_MRT_BLUE, LINE_MRT_PURPLE,
    LINE_ARL, LINE_GOLD, LINE_PINK, LINE_YELLOW,
    LINE_ORANGE_EAST, LINE_ORANGE_WEST, LINE_RED_LIGHT, LINE_RED_DARK, NULL
  };
  for(int li = 0; lines[li]; li++){
    printf("\n  --- %s ---\n", lines[li]);
    for(int i = 0; i < g->stationCount; i++){
      if(strcmp(g->stations[i].line, lines[li]) == 0){
        printf("    [%-5s] %s%s\n",
               g->stations[i].code[0] ? g->stations[i].code : "-",
               g->stations[i].name,
               g->stations[i].isInterchange ? " [IX]" : "");
      }
    }
  }
}

/* menu option 7: add a new station and optional connection */
static void menuAddStation(Graph *g, HashTable *ht, BST *bst){
  char code[MAX_CODE_LEN], name[MAX_NAME_LEN], line[MAX_LINE_LEN];
  char isIxStr[4], connCode[MAX_CODE_LEN];
  char distStr[16];
  int isInterchange;
  double dist;

  printf("\n  === Add New Station ===\n");
  printf("  Station code (e.g. N25): ");
  readLine(code, MAX_CODE_LEN);
  printf("  Station name           : ");
  readLine(name, MAX_NAME_LEN);
  printf("  Line name              : ");
  readLine(line, MAX_LINE_LEN);
  printf("  Is interchange? (1=Yes, 0=No): ");
  readLine(isIxStr, sizeof(isIxStr));

  if(code[0] == '\0' || name[0] == '\0' || line[0] == '\0'){
    printf("  [!] Code, name, and line cannot be empty.\n");
    return;
  }
  if(!parseIntStrict(isIxStr, &isInterchange) || (isInterchange != 0 && isInterchange != 1)){
    printf("  [!] Interchange must be 0 or 1.\n");
    return;
  }

  if(stationCodeExists(g, code)){
    printf("  [!] Station code \"%s\" already exists.\n", code);
    return;
  }
  if(stationNameLineExists(g, name, line)){
    printf("  [!] Station \"%s\" on line \"%s\" already exists.\n", name, line);
    return;
  }
  if(g->stationCount >= MAX_STATIONS){
    printf("  [!] Station limit reached (%d).\n", MAX_STATIONS);
    return;
  }

  int idx = graphAddStation(g, code, name, line, isInterchange);
  if(idx < 0){
    printf("  [!] Unable to add station.\n");
    return;
  }
  linkSameNameInterchanges(g);
  printf("  [+] Station \"%s\" added (id=%d).\n", name, idx);

  printf("\n  Connect to an existing station? (Enter code/name, or leave blank to skip): ");
  readLine(connCode, MAX_CODE_LEN);
  if(connCode[0] != '\0'){
    int ci = hashSearch(ht, connCode);
    if(ci < 0 || ci == idx){
      printf("  [!] Connection station not found or same station - skipping.\n");
    }
    else{
      printf("  Distance (km, e.g. 1.2): ");
      readLine(distStr, sizeof(distStr));
      if(!parseNonNegativeDouble(distStr, &dist)){
        printf("  [!] Distance must be a non-negative number - skipping connection.\n");
      }
      else{
        graphAddUndirectedEdge(g, idx, ci, dist, 2, 3);
        printf("  [+] Connected \"%s\" <-> \"%s\".\n", name, g->stations[ci].name);
      }
    }
  }

  rebuildLookupStructures(g, ht, bst);
  saveData(g);
  printf("  [+] Saved to CSV.\n");
}

/* menu option 8: delete a station (with confirmation) */
static void menuDeleteStation(Graph *g, HashTable *ht, BST *bst){
  char query[MAX_NAME_LEN];
  printf("\n  === Delete Station ===\n");
  printf("  Station name or code to delete: ");
  readLine(query, MAX_NAME_LEN);

  int idx = hashSearch(ht, query);
  if(idx < 0){
    printf("  [!] Station \"%s\" not found.\n", query);
    return;
  }

  const Station *s = &g->stations[idx];
  char savedName[MAX_NAME_LEN];
  strncpy(savedName, s->name, MAX_NAME_LEN - 1);
  savedName[MAX_NAME_LEN - 1] = '\0';
  printf("  About to delete: [%s] %s (%s)\n", s->code, s->name, s->line);
  printf("  Confirm? (y/n): ");
  char confirm[4];
  readLine(confirm, sizeof(confirm));
  if(confirm[0] != 'y' && confirm[0] != 'Y'){
    printf("  Cancelled.\n");
    return;
  }

  graphRemoveStation(g, idx);
  rebuildLookupStructures(g, ht, bst);
  saveData(g);
  printf("  [-] Station \"%s\" deleted and CSV updated.\n", savedName);
}

int main(int argc, char **argv){
  (void)argc;
  Graph *g = createGraph();
  HashTable *ht = createHashTable();
  BST *bst = createBST();

  initDataPaths(argv[0]);
  printf("\n  Loading Bangkok Mass Transit data...\n");
  loadData(g, ht, bst);
  printf("  Ready!\n\n");

  int choice;
  do{
    printf("\n+==================================================+\n");
    printf("|     Bangkok Mass Transit Route Finder            |\n");
    printf("|     CPE112 Data Structures Final Project         |\n");
    printf("+==================================================+\n");
    printf("|  1. Find Route (Minimum Stops)                   |\n");
    printf("|  2. Find Route (Minimum Fare)                    |\n");
    printf("|  3. Search Station                               |\n");
    printf("|  4. Show Station Details                         |\n");
    printf("|  5. Show All Stations (A-Z)                      |\n");
    printf("|  6. Show Stations by Line                        |\n");
    printf("|  7. Add Station                                  |\n");
    printf("|  8. Delete Station                               |\n");
    printf("|  0. Exit                                         |\n");
    printf("+==================================================+\n");
    printf("  Choice: ");

    if(scanf("%d", &choice) != 1){ clearInput(); continue; }
    clearInput();

    switch(choice){
      case 1: menuFindRoute(g, ht, "stops");      break;
      case 2: menuFindRoute(g, ht, "fare");       break;
      case 3: menuSearchStation(g, ht, bst);      break;
      case 4: menuShowStationDetails(g, ht);      break;
      case 5: menuShowAllAZ(g);                   break;
      case 6: menuShowByLine(g);                  break;
      case 7: menuAddStation(g, ht, bst);         break;
      case 8: menuDeleteStation(g, ht, bst);      break;
      case 0: printf("\n  Goodbye!\n");           break;
      default: printf("  [!] Invalid choice.\n");
    }
  } while(choice != 0);

  freeGraph(g);
  freeHashTable(ht);
  freeBST(bst);
  return 0;
}
