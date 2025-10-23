# CDN Simulator in C++

This project is a C++ simulation of a basic Content Delivery Network (CDN). It demonstrates how pathfinding algorithms (like Dijkstra's) and caching mechanisms (like LRU) work together to efficiently deliver content to users across a network.

The network topology (servers, gateways, and latencies) is loaded from an external file, and new users can be added dynamically through a command-line interface.

## Features

* **Graph-Based Network**: The network is modeled as an undirected graph where nodes can be users, edge servers, gateways, or an origin server.
* **Dijkstra's Algorithm**: Used to find the lowest-latency path from a user to the nearest edge server.
* **LRU Caching**: Each edge server is equipped with a an LRU (Least Recently Used) cache to store content.
* **Dynamic Simulation**: The simulation runs in real-time via a simple Command-Line Interface (CLI).
* **Configurable**: The entire base network is loaded from a `config.txt` file, making it easy to test different topologies.

---

## File Structure

The project is split into three main modules:

* **`lru_cache.h` / `lru_cache.cpp`**: A self-contained, high-performance LRU Cache implementation using `std::list` and `std::unordered_map` for O(1) get and put operations.
* **`cdn_simulator.h` / `cdn_simulator.cpp`**: The core logic. This module defines the `Node` and `CDNSimulator` classes, manages the graph, runs Dijkstra's algorithm, and handles the simulation workflow.
* **`main.cpp`**: The application entry point. This file is responsible for parsing the `config.txt` file and running the interactive CLI.
* **`config.txt`**: The input file that defines the initial network of servers and gateways.

---

## How to Compile and Run

You will need a C++ compiler like `g++`.

1.  **Compile the Project:**
    Open your terminal and run the following command to compile all the source files into a single executable named `cdn_sim`:
    ```bash
    g++ main.cpp cdn_simulator.cpp lru_cache.cpp -o cdn_sim
    ```
    *Note: On some systems, you may need to add the `-std=c++17` flag if your compiler defaults to an older standard.*

2.  **Run the Simulator:**
    Once compiled, run the program:
    ```bash
    ./cdn_sim
    ```
    The program will load the `config.txt` file and present you with a `>` prompt.

---

## Configuration File Format (`config.txt`)

The `config.txt` file defines the nodes and edges of your network.

### `NODE`
Defines a new node in the graph.
```
NODE <TYPE> <ID> [CACHE_CAPACITY]
```
* **`TYPE`**: Can be `ORIGIN`, `SERVER` (Edge Server), or `GATEWAY`.
* **`ID`**: A unique integer ID for the node.
* **`[CACHE_CAPACITY]`**: **Required only for `SERVER` types.** Defines the number of items the server's cache can hold.

**Example:**
```
# The main origin server
NODE ORIGIN 1

# An edge server with a cache size of 10
NODE SERVER 10 10

# A simple network gateway
NODE GATEWAY 100
```

### `EDGE`
Defines an undirected edge (a two-way connection) between two nodes with a given latency.
```
EDGE <NODE1_ID> <NODE2_ID> <LATENCY_MS>
```
**Example:**
```
# Connects Gateway 100 to Server 10 with 15ms latency
EDGE 100 10 15
```

---

## CLI Commands

Once the simulator is running, you can use the following commands:

* **`addUser <newUserID> <connectToNodeID> <latency>`**
    Adds a new user to the simulation and connects them to an existing node in the network.
    ```
    > addUser 1001 100 5
    User 1001 added and connected to 100.
    ```

* **`request <userID> <contentName>`**
    Simulates a content request from a user. This will trigger Dijkstra's algorithm to find the best server and then perform a cache check.
    ```
    > request 1001 highlights.mp4
    ```

* **`help`**
    Displays a list of all available commands.

* **`exit`**
    Quits the simulator.

## Example Session

```bash
$ ./cdn_sim
Initial network configuration loaded from config.txt.

CDN Simulator CLI. Type 'help' for commands.
> addUser 1001 100 5
User 1001 added and connected to 100.
> request 1001 cool_video.mp4
--- Simulating Request: User 1001 -> 'cool_video.mp4' ---
Optimal server found: Server 10 (Latency: 15ms)
[Cache MISS] Content 1 not in Server 10. Fetching from origin...
Origin server (Node 1) reached in 100ms.
Content 1 cached on Server 10 and served.
Total Latency: 115ms (15ms to server + 100ms to origin)
----------------------------------------
> request 1001 cool_video.mp4
--- Simulating Request: User 1001 -> 'cool_video.mp4' ---
Optimal server found: Server 10 (Latency: 15ms)
[Cache HIT] Content 1 served from Server 10.
Total Latency: 15ms
----------------------------------------
> exit
Exiting simulator.
```
