#ifndef CDN_SIMULATOR_H
#define CDN_SIMULATOR_H

#include <vector>
#include <string>
#include <unordered_map>
#include <utility> // For std::pair
#include <limits>  // For INT_MAX
#include <queue>   // For std::priority_queue
#include <iostream>

#include "../lruCache/lru_cache.hpp" // Include the cache we built

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

    // Constructor to simplify node creation
    Node(int node_id = -1, NodeType node_type = NodeType::USER, int cache_capacity = 0)
        : id(node_id), type(node_type), cache(nullptr)
    {
        if (type == NodeType::EDGE_SERVER)
        {
            cache = new LRUCache(cache_capacity);
        }
    }

    // Destructor to clean up the cache memory
    ~Node()
    {
        delete cache; // delete on nullptr is safe
    }

    // --- Rule of Five ---
    // We must delete copy operations because we are managing a raw pointer.
    // This prevents two Node objects from pointing to (and trying to delete) the same cache.
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    // We can, however, allow move operations
    Node(Node &&other) noexcept
        : id(other.id), type(other.type), cache(other.cache)
    {
        other.cache = nullptr; // Important: prevent the moved-from object from deleting the cache
    }

    Node &operator=(Node &&other) noexcept
    {
        if (this != &other)
        {
            delete cache; // Delete our own cache
            // Steal data from the other object
            id = other.id;
            type = other.type;
            cache = other.cache;
            other.cache = nullptr; // Prevent double-delete
        }
        return *this;
    }
};

// The main class to manage and run the simulation
class CDNSimulator
{
public:
    CDNSimulator() = default; // Default constructor is fine

    // Methods for building the network
    void add_node(int node_id, Node::NodeType type, int cache_capacity = 0);
    void add_edge(int u_id, int v_id, int latency);

    // Method to run the simulation logic
    void simulate_request(int user_id, const std::string &content_name);

private:
    // --- Data Structures ---
    std::unordered_map<int, std::vector<std::pair<int, int>>> adj_list_;
    std::unordered_map<int, Node> nodes_;
    std::unordered_map<std::string, int> content_manifest_;

    int next_content_id_ = 1;
    int origin_server_id_ = -1;

    // --- Private Helper Methods ---

    // Runs Dijkstra's from a start node and returns a map of {node, min_latency}
    std::unordered_map<int, int> run_dijkstra(int start_node_id);

    // Parses a distance map to find the closest edge server
    std::pair<int, int> find_best_server(const std::unordered_map<int, int> &distances);

    // Gets a content ID, creating one if it doesn't exist
    int get_or_create_content_id(const std::string &content_name);

    // Simulates fetching content from the origin
    std::string fetch_from_origin(int content_id);
};

#endif // CDN_SIMULATOR_H