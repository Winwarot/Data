#ifndef STATION_H
#define STATION_H

#define MAX_NAME_LEN  100
#define MAX_LINE_LEN   80
#define MAX_CODE_LEN   10
#define MAX_STATIONS  400
#define INF           999999999

#define LINE_BTS_SUKHUMVIT  "BTS Sukhumvit (Light Green)"
#define LINE_BTS_SILOM      "BTS Silom (Dark Green)"
#define LINE_MRT_BLUE       "MRT Blue Line"
#define LINE_MRT_PURPLE     "MRT Purple Line"
#define LINE_ARL            "Airport Rail Link"
#define LINE_GOLD           "BTS Gold Line"
#define LINE_PINK           "Pink Line"
#define LINE_YELLOW         "Yellow Line"
#define LINE_ORANGE_EAST    "Orange Line (East)"
#define LINE_ORANGE_WEST    "Orange Line (West)"
#define LINE_RED_LIGHT      "Light Red Line (SRT)"
#define LINE_RED_DARK       "Dark Red Line (SRT)"
#define LINE_INTERCHANGE    "Interchange"

typedef struct Station {
    int id;
    char code[MAX_CODE_LEN];
    char name[MAX_NAME_LEN];
    char line[MAX_LINE_LEN];
    int isInterchange;
    int isClosed;
} Station;

typedef struct Edge {
    int destination;
    double distance;
    int time;
    int fare;
    struct Edge *next;
} Edge;

#endif
