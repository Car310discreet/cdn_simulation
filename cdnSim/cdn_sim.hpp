#ifndef CDN_SIMULATOR_H
#define CDN_SIMULATOR_H

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <limits>
#include <queue>
#include <iostream>

#include "../lruCache/lru_cache.hpp"
using namespace std;

// Represents a single node in our network graph
struct Node
{
    enum class NodeType
    {
        USER,
        EDGE_SERVER,
        GATEWAY,
        ORIGIN
    };

    int id;
    NodeType type;
    LRUCache *cache; // Only edge servers will have a cache

    Node(int node_id = -1, NodeType node_type = NodeType::USER, int cache_capacity = 0)
        : id(node_id), type(node_type), cache(nullptr)
    {
        if (type == NodeType::EDGE_SERVER)
        {
            cache = new LRUCache(cache_capacity);
        }
    }

    ~Node()
    {
        delete cache;
    }

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    Node(Node &&other) noexcept
        : id(other.id), type(other.type), cache(other.cache)
    {
        other.cache = nullptr;
    }

    Node &operator=(Node &&other) noexcept
    {
        if (this != &other)
        {
            delete cache;
            id = other.id;
            type = other.type;
            cache = other.cache;
            other.cache = nullptr;
        }
        return *this;
    }
};

class CDNSimulator
{
public:
    CDNSimulator() = default;

    // Methods for building the network
    void add_node(int node_id, Node::NodeType type, int cache_capacity = 0);
    void add_edge(int u_id, int v_id, int latency);

    // Method to run the simulation logic
    void simulate_request(int user_id, const string &content_name);

    void calculate_backbone_mst();

private:
    // Data structures
    unordered_map<int, vector<pair<int, int>>> adj_list_;
    unordered_map<int, Node> nodes_;
    unordered_map<string, int> content_manifest_;

    int next_content_id_ = 1;
    int origin_server_id_ = -1;

    // Helper methods

    // Runs Dijkstra's from a start node and returns a map of {node, min_latency}
    unordered_map<int, int> run_dijkstra(int start_node_id);

    // Parses a distance map to find the closest edge server
    pair<int, int> find_best_server(const unordered_map<int, int> &distances);

    // Gets a content ID, creating one if it doesn't exist
    int get_or_create_content_id(const string &content_name);

    // Simulates fetching content from the origin
    string fetch_from_origin(int content_id);
};

#endif // CDN_SIMULATOR_H