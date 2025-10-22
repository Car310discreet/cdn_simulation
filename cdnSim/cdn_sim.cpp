#include "cdn_sim.hpp"

// --- Public Methods ---

void CDNSimulator::add_node(int node_id, Node::NodeType type, int cache_capacity)
{
    if (nodes_.count(node_id))
    {
        std::cerr << "Warning: Node " << node_id << " already exists." << std::endl;
        return;
    }

    // Store the ID of the origin server when it's added
    if (type == Node::NodeType::ORIGIN)
    {
        if (origin_server_id_ != -1)
        {
            std::cerr << "Warning: Multiple origin servers added. Using the last one: " << node_id << std::endl;
        }
        origin_server_id_ = node_id;
    }

    // Emplace constructs the Node in-place, avoiding copy operations
    nodes_.emplace(node_id, Node(node_id, type, cache_capacity));
}

void CDNSimulator::add_edge(int u_id, int v_id, int latency)
{
    // Check if both nodes exist
    if (nodes_.find(u_id) == nodes_.end() || nodes_.find(v_id) == nodes_.end())
    {
        std::cerr << "Error: Adding edge between non-existent nodes (" << u_id << ", " << v_id << ")" << std::endl;
        return;
    }

    // Enforce our design rule: no direct user-to-user connections
    if (nodes_.at(u_id).type == Node::NodeType::USER && nodes_.at(v_id).type == Node::NodeType::USER)
    {
        std::cerr << "Error: Cannot create a direct edge between two users (" << u_id << ", " << v_id << ")" << std::endl;
        return;
    }

    // Add the undirected edge
    adj_list_[u_id].push_back({v_id, latency});
    adj_list_[v_id].push_back({u_id, latency});
}

void CDNSimulator::simulate_request(int user_id, const std::string &content_name)
{
    if (nodes_.find(user_id) == nodes_.end() || nodes_.at(user_id).type != Node::NodeType::USER)
    {
        std::cout << "[Request Failed: User " << user_id << " does not exist.]" << std::endl;
        return;
    }

    std::cout << "--- Simulating Request: User " << user_id << " -> '" << content_name << "' ---" << std::endl;

    // 1. Get the content ID
    int content_id = get_or_create_content_id(content_name);

    // 2. Run Dijkstra's from the user to find paths to all nodes
    std::unordered_map<int, int> user_distances = run_dijkstra(user_id);

    // 3. Find the best server from the results
    std::pair<int, int> best_server = find_best_server(user_distances);
    int server_id = best_server.first;
    int latency_to_server = best_server.second;

    if (server_id == -1)
    {
        std::cout << "[Request Failed: No path from User " << user_id << " to any edge server.]" << std::endl;
        return;
    }

    std::cout << "Optimal server found: Server " << server_id << " (Latency: " << latency_to_server << "ms)" << std::endl;

    // 4. Check the server's cache
    Node &server_node = nodes_.at(server_id);
    std::string content = server_node.cache->get(content_id);

    if (content != "")
    {
        // 5. Cache Hit
        std::cout << "[Cache HIT] Content " << content_id << " served from Server " << server_id << "." << std::endl;
        std::cout << "Total Latency: " << latency_to_server << "ms" << std::endl;
    }
    else
    {
        // 6. Cache Miss
        std::cout << "[Cache MISS] Content " << content_id << " not in Server " << server_id << ". Fetching from origin..." << std::endl;

        // 6a. Find path from server to origin
        std::unordered_map<int, int> server_distances = run_dijkstra(server_id);
        int latency_to_origin = server_distances.count(origin_server_id_) ? server_distances.at(origin_server_id_) : -1;

        if (latency_to_origin == -1)
        {
            std::cout << "[Request Failed: Server " << server_id << " has no path to Origin " << origin_server_id_ << ".]" << std::endl;
            return;
        }

        std::cout << "Origin server (Node " << origin_server_id_ << ") reached in " << latency_to_origin << "ms." << std::endl;

        // 6b. Fetch content and put it in the cache
        std::string new_content = fetch_from_origin(content_id);
        server_node.cache->put(content_id, new_content);
        std::cout << "Content " << content_id << " cached on Server " << server_id << " and served." << std::endl;

        // 6c. Report total latency
        std::cout << "Total Latency: " << (latency_to_server + latency_to_origin) << "ms ("
                  << latency_to_server << "ms to server + " << latency_to_origin << "ms to origin)" << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}

// --- Private Helper Methods ---

std::unordered_map<int, int> CDNSimulator::run_dijkstra(int start_node_id)
{
    std::unordered_map<int, int> distances;
    for (auto const &[id, node] : nodes_)
    {
        distances[id] = std::numeric_limits<int>::max();
    }

    // Priority queue stores {latency, node_id}
    // We use std::greater to make it a min-heap
    std::priority_queue<std::pair<int, int>,
                        std::vector<std::pair<int, int>>,
                        std::greater<std::pair<int, int>>>
        pq;

    distances[start_node_id] = 0;
    pq.push({0, start_node_id});

    while (!pq.empty())
    {
        int current_dist = pq.top().first;
        int current_id = pq.top().second;
        pq.pop();

        // If we've already found a shorter path, skip
        if (current_dist > distances[current_id])
        {
            continue;
        }

        // Stop if no path exists from this node
        if (adj_list_.find(current_id) == adj_list_.end())
        {
            continue;
        }

        // Check all neighbors
        for (auto const &neighbor : adj_list_.at(current_id))
        {
            int neighbor_id = neighbor.first;
            int edge_weight = neighbor.second;
            int new_dist = current_dist + edge_weight;

            if (new_dist < distances[neighbor_id])
            {
                distances[neighbor_id] = new_dist;
                pq.push({new_dist, neighbor_id});
            }
        }
    }
    return distances;
}

std::pair<int, int> CDNSimulator::find_best_server(const std::unordered_map<int, int> &distances)
{
    int min_latency = std::numeric_limits<int>::max();
    int best_server_id = -1;

    for (auto const &[id, node] : nodes_)
    {
        if (node.type == Node::NodeType::EDGE_SERVER)
        {
            if (distances.count(id) && distances.at(id) < min_latency)
            {
                min_latency = distances.at(id);
                best_server_id = id;
            }
        }
    }

    if (best_server_id == -1)
    {
        return {-1, -1}; // No server found
    }
    return {best_server_id, min_latency};
}

int CDNSimulator::get_or_create_content_id(const std::string &content_name)
{
    if (content_manifest_.find(content_name) == content_manifest_.end())
    {
        // This is a new piece of content, register it
        content_manifest_[content_name] = next_content_id_;
        next_content_id_++;
    }
    return content_manifest_[content_name];
}

std::string CDNSimulator::fetch_from_origin(int content_id)
{
    // This is a simulation, so we just return a dummy string
    return "DataPayload(ContentID:" + std::to_string(content_id) + ")";
}