#include "data.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STATIONS_CSV "stations.csv"
#define EDGES_CSV    "edges.csv"
#define FARE_CSV     "fare.csv"

char g_dataDir[PATH_MAX] = "";

char *trimNewline(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
    return s;
}

char *trimSpace(char *s) {
    while (*s == ' ') s++;
    int n = (int)strlen(s);
    while (n > 0 && s[n-1] == ' ') s[--n] = '\0';
    return s;
}

int nextField(char **p, char *out, int maxLen) {
    if (!*p || **p == '\0') { out[0] = '\0'; return 0; }

    char *start = *p;
    char *comma = strchr(start, ',');
    int len;

    if (comma) {
        len = (int)(comma - start);
        *p = comma + 1;
    } else {
        len = (int)strlen(start);
        *p = start + len;
    }

    if (len >= maxLen) len = maxLen - 1;
    strncpy(out, start, len);
    out[len] = '\0';
    return 1;
}

void buildDataPath(char *out, size_t outSize, const char *filename) {
    if (g_dataDir[0] == '\0')
        snprintf(out, outSize, "%s", filename);
    else
        snprintf(out, outSize, "%s/%s", g_dataDir, filename);
}

void initDataPaths(const char *argv0) {
    char resolved[PATH_MAX];

    g_dataDir[0] = '\0';
    if (!argv0 || argv0[0] == '\0') return;

    if (!realpath(argv0, resolved)) {
        if (argv0[0] != '/') return;
        strncpy(resolved, argv0, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    }

    char *slash = strrchr(resolved, '/');
    if (!slash) return;
    *slash = '\0';

    strncpy(g_dataDir, resolved, sizeof(g_dataDir) - 1);
    g_dataDir[sizeof(g_dataDir) - 1] = '\0';
}

int hasEdge(const Graph *g, int from, int to) {
    for (Edge *e = g->adjList[from]; e; e = e->next)
        if (e->destination == to) return 1;
    return 0;
}

void linkSameNameInterchanges(Graph *g) {
    for (int i = 0; i < g->stationCount; i++) {
        for (int j = i + 1; j < g->stationCount; j++) {
            if (strcmp(g->stations[i].name, g->stations[j].name) != 0) continue;
            if (strcmp(g->stations[i].line, g->stations[j].line) == 0) continue;

            g->stations[i].isInterchange = 1;
            g->stations[j].isInterchange = 1;

            if (!hasEdge(g, i, j)) graphAddEdge(g, i, j, 0.0, 0, 0);
            if (!hasEdge(g, j, i)) graphAddEdge(g, j, i, 0.0, 0, 0);
        }
    }
}

void rebuildLookupStructures(const Graph *g, HashTable *ht, BST *bst) {
    hashClear(ht);
    bstClear(bst);

    for (int i = 0; i < g->stationCount; i++) {
        const Station *s = &g->stations[i];
        if (s->name[0] != '\0' && hashSearch(ht, s->name) < 0)
            hashInsert(ht, s->name, i);
        if (s->code[0] != '\0')
            hashInsert(ht, s->code, i);
        if (s->name[0] != '\0')
            bstInsert(bst, s->name);
    }
}

void loadData(Graph *g, HashTable *ht, BST *bst) {
    char pathBuf[PATH_MAX];
    char line[512];

    buildDataPath(pathBuf, sizeof(pathBuf), STATIONS_CSV);
    FILE *fs = fopen(pathBuf, "r");
    if (!fs) { fprintf(stderr, "  [!] Cannot open %s\n", STATIONS_CSV); return; }

    fgets(line, sizeof(line), fs);
    while (fgets(line, sizeof(line), fs)) {
        trimNewline(line);
        if (line[0] == '\0') continue;

        char *p = line;
        char code[MAX_CODE_LEN], name[MAX_NAME_LEN], lineName[MAX_LINE_LEN], isIxStr[4];

        nextField(&p, code,     MAX_CODE_LEN);
        nextField(&p, name,     MAX_NAME_LEN);
        nextField(&p, lineName, MAX_LINE_LEN);
        nextField(&p, isIxStr,  sizeof(isIxStr));

        graphAddStation(g, code, name, lineName, atoi(isIxStr));
    }
    fclose(fs);

    linkSameNameInterchanges(g);
    rebuildLookupStructures(g, ht, bst);

    FareTable ft;
    ft.count = 0;

    buildDataPath(pathBuf, sizeof(pathBuf), FARE_CSV);
    FILE *ff = fopen(pathBuf, "r");
    if (!ff) {
        fprintf(stderr, "  [!] Cannot open %s\n", FARE_CSV);
    } else {
        fgets(line, sizeof(line), ff);
        while (fgets(line, sizeof(line), ff) && ft.count < MAX_FARE_ENTRIES) {
            trimNewline(line);
            if (line[0] == '\0') continue;

            char *p = line;
            char typeStr[32], passStr[8], fareStr[8];

            nextField(&p, typeStr, sizeof(typeStr));
            nextField(&p, passStr, sizeof(passStr));
            nextField(&p, fareStr, sizeof(fareStr));

            strncpy(ft.entries[ft.count].lineType, trimSpace(typeStr), MAX_LINE_LEN - 1);
            ft.entries[ft.count].lineType[MAX_LINE_LEN - 1] = '\0';
            ft.entries[ft.count].stationsPassed = atoi(trimSpace(passStr));
            ft.entries[ft.count].fare           = atoi(trimSpace(fareStr));
            ft.count++;
        }
        fclose(ff);
    }
    g->fareTable = ft;

    buildDataPath(pathBuf, sizeof(pathBuf), EDGES_CSV);
    FILE *fe = fopen(pathBuf, "r");
    if (!fe) { fprintf(stderr, "  [!] Cannot open %s\n", EDGES_CSV); return; }

    fgets(line, sizeof(line), fe);
    while (fgets(line, sizeof(line), fe)) {
        trimNewline(line);
        if (line[0] == '\0') continue;

        char *p = line;
        char fromCode[MAX_CODE_LEN], toCode[MAX_CODE_LEN], distStr[16];

        nextField(&p, fromCode, MAX_CODE_LEN);
        nextField(&p, toCode,   MAX_CODE_LEN);
        nextField(&p, distStr,  sizeof(distStr));

        int fi = hashSearch(ht, fromCode);
        int ti = hashSearch(ht, toCode);
        if (fi < 0 || ti < 0) continue;

        double dist = atof(distStr);

        if (!hasEdge(g, fi, ti)) graphAddEdge(g, fi, ti, dist, 2, 17);
        if (!hasEdge(g, ti, fi)) graphAddEdge(g, ti, fi, dist, 2, 17);
    }
    fclose(fe);

    printf("  Loaded %d stations.\n", g->stationCount);
}

void saveData(const Graph *g) {
    char pathBuf[PATH_MAX];

    buildDataPath(pathBuf, sizeof(pathBuf), STATIONS_CSV);
    FILE *fs = fopen(pathBuf, "w");
    if (!fs) { fprintf(stderr, "  [!] Cannot write %s\n", STATIONS_CSV); return; }

    fprintf(fs, "code,name,line,isInterchange\n");
    for (int i = 0; i < g->stationCount; i++) {
        const Station *s = &g->stations[i];
        fprintf(fs, "%s,%s,%s,%d\n", s->code, s->name, s->line, s->isInterchange);
    }
    fclose(fs);

    buildDataPath(pathBuf, sizeof(pathBuf), EDGES_CSV);
    FILE *fe = fopen(pathBuf, "w");
    if (!fe) { fprintf(stderr, "  [!] Cannot write %s\n", EDGES_CSV); return; }

    fprintf(fe, "from_code,to_code,distance\n");
    for (int i = 0; i < g->stationCount; i++) {
        for (Edge *e = g->adjList[i]; e; e = e->next) {
            int j = e->destination;
            int isInterchange = (strcmp(g->stations[i].line, g->stations[j].line) != 0);
            if (i < j || isInterchange)
                fprintf(fe, "%s,%s,%.1f\n", g->stations[i].code, g->stations[j].code, e->distance);
        }
    }
    fclose(fe);
}
