# Bangkok Mass Transit Route Finder

A terminal-based route planner for Bangkok's rail transit network, written entirely in C with no external libraries. Built as a final project for CPE112 Data Structures.


## Features

- Find the route with the **fewest stops** between any two stations
- Find the route with the **lowest fare**
- Search stations by full name, station code, or name prefix
- View station details including line, interchange status, and neighboring stations
- List all stations alphabetically (A-Z)
- Browse stations grouped by transit line
- Add a new station and optionally connect it to an existing one
- Delete a station — changes are saved to CSV immediately

Covers **210 stations** across 11 transit lines with **36 interchange stations** auto-detected from the data.


## Transit Lines Covered

| Line | Stations |
|---|---|
| BTS Sukhumvit (Light Green) | 48 |
| MRT Blue Line | 38 |
| Pink Line | 32 |
| Yellow Line | 23 |
| Orange Line (East) | 17 |
| MRT Purple Line | 16 |
| BTS Silom (Dark Green) | 13 |
| Airport Rail Link | 8 |
| Dark Red Line (SRT) | 8 |
| Light Red Line (SRT) | 4 |
| BTS Gold Line | 3 |


## Getting Started

### Requirements

- `gcc` or `clang` with standard C library
- macOS, Linux, or Windows (with MinGW / MSYS2 for Windows)

### macOS — double-click to run

Double-click `start.command` in Finder. The first time macOS may ask for permission — go to **System Settings > Privacy & Security** and click Allow.

### Windows — double-click to run

Double-click `start.bat`. Requires `gcc` to be installed and available on PATH (e.g. via [MinGW-w64](https://www.mingw-w64.org/) or [MSYS2](https://www.msys2.org/)).

### Compile and run manually

```bash
gcc -Wall -o transit main.c data.c graph.c hashtable.c bst.c
./transit
```

Both start scripts compile the project first, then launch the program automatically.


## How to Use

The program shows a numbered menu. Type a number and press Enter.

```
1. Find Route (Minimum Stops)    Fewest stations between source and destination
2. Find Route (Minimum Fare)     Cheapest path between source and destination
3. Search Station                Search by name, code, or prefix
4. Show Station Details          Full info + list of neighboring stations
5. Show All Stations (A-Z)       Alphabetical list of all 210 stations
6. Show Stations by Line         Stations grouped by transit line
7. Add Station                   Add a new station and optionally connect it
8. Delete Station                Remove a station (with confirmation prompt)
0. Exit
```

**Entering a station** — you can use the station's full name or its code:

| Input | Matches |
|---|---|
| `Asok` | Asok station (BTS) |
| `E4` | Same station by code |
| `Siam` | Siam station |
| `CEN` | Same station by code |
| `Bang` | Prefix search — lists all stations starting with "Bang" |

If no exact match is found, the program automatically falls back to a prefix search using the BST.

---

## Example Session

Finding the route from **Asok to Siam**:

```
Choice: 1
Source station (name or code): Asok
Destination station (name or code): Siam

Route: [E4] Asok -> [E3] Nana -> [E2] Phloen Chit -> [E1] Chit Lom -> [CEN] Siam
Stations    : 4 stop(s)
Distance    : 4.0 km
Time        : ~8 minutes
Fare        : 68 THB
Line changes: 0
```


## Project Structure

```
main.c           Menu loop, input parsing, all menu handlers
graph.c / .h     Adjacency-list graph, Dijkstra, path display
hashtable.c / .h Hash table for O(1) station lookup by name or code
bst.c / .h       Binary Search Tree for prefix search and A-Z ordering
data.c / .h      CSV load/save, interchange auto-linking, index rebuild
station.h        Station and Edge structs, line name constants

stations.csv     Station data  (code, name, line, isInterchange)
edges.csv        Connections   (from_code, to_code, distance_km)
fare.csv         Fare table    (line_type, stations_passed, fare_THB)

start.command    macOS launcher — double-click in Finder
start.bat        Windows launcher — double-click in Explorer
```


## Data Files

All data lives in plain CSV files in the same folder as the executable, so the program can read and write them regardless of where it is launched from.

**stations.csv** — one station per row:
```
code,name,line,isInterchange
E4,Asok,BTS Sukhumvit (Light Green),1
BL22,Sukhumvit,MRT Blue Line,1
```

**edges.csv** — one directed pair per row (program loads both directions):
```
from_code,to_code,distance
E4,E3,1.0
E3,E2,0.9
```

**fare.csv** — step fare lookup table per line type:
```
type, station_pass, fare
BTS, 0, 17
BTS, 1, 17
BTS, 2, 25
MRT, 0, 17
MRT, 1, 17
MRT, 2, 20
```

Minimum fare is **17 THB** across all lines.
