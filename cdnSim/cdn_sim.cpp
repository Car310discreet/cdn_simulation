#include "cdn_sim.hpp"
using namespace std;

void CDNSimulator::add_node(int node_id, Node::NodeType type, int cache_capacity)
{
    if (nodes_.count(node_id))
    {
        cerr << "Warning: Node " << node_id << " already exists." << endl;
        return;
    }

    if (type == Node::NodeType::ORIGIN)
    {
        if (origin_server_id_ != -1)
        {
            cerr << "Warning: Multiple origin servers added. Using the last one: " << node_id << endl;
        }
        origin_server_id_ = node_id;
    }

    nodes_.emplace(node_id, Node(node_id, type, cache_capacity));
}

void CDNSimulator::add_edge(int u_id, int v_id, int latency)
{
    // Check if both nodes exist
    if (nodes_.find(u_id) == nodes_.end() || nodes_.find(v_id) == nodes_.end())
    {
        cerr << "Error: Adding edge between non-existent nodes (" << u_id << ", " << v_id << ")" << endl;
        return;
    }

    if (nodes_.at(u_id).type == Node::NodeType::USER && nodes_.at(v_id).type == Node::NodeType::USER)
    {
        cerr << "Error: Cannot create a direct edge between two users (" << u_id << ", " << v_id << ")" << endl;
        return;
    }

    // Add the undirected edge
    adj_list_[u_id].push_back({v_id, latency});
    adj_list_[v_id].push_back({u_id, latency});
}

void CDNSimulator::simulate_request(int user_id, const string &content_name)
{
    if (nodes_.find(user_id) == nodes_.end() || nodes_.at(user_id).type != Node::NodeType::USER)
    {
        cout << "[Request Failed: User " << user_id << " does not exist.]" << endl;
        return;
    }

    cout << "--- Simulating Request: User " << user_id << " -> '" << content_name << "' ---" << endl;

    // 1. Get the content ID
    int content_id = get_or_create_content_id(content_name);

    // 2. Run Dijkstra's from the user to find paths to all nodes
    unordered_map<int, int> user_distances = run_dijkstra(user_id);

    // 3. Find the best server from the results
    pair<int, int> best_server = find_best_server(user_distances);
    int server_id = best_server.first;
    int latency_to_server = best_server.second;

    if (server_id == -1)
    {
        cout << "[Request Failed: No path from User " << user_id << " to any edge server.]" << endl;
        return;
    }

    cout << "Optimal server found: Server " << server_id << " (Latency: " << latency_to_server << "ms)" << endl;

    // 4. Check the server's cache
    Node &server_node = nodes_.at(server_id);
    string content = server_node.cache->get(content_id);

    if (content != "")
    {
        // 5. Cache Hit
        cout << "[Cache HIT] Content " << content_id << " served from Server " << server_id << "." << endl;
        cout << "Total Latency: " << latency_to_server << "ms" << endl;
    }
    else
    {
        // 6. Cache Miss
        cout << "[Cache MISS] Content " << content_id << " not in Server " << server_id << ". Fetching from origin..." << endl;

        // 6a. Find path from server to origin
        unordered_map<int, int> server_distances = run_dijkstra(server_id);
        int latency_to_origin = server_distances.count(origin_server_id_) ? server_distances.at(origin_server_id_) : -1;

        if (latency_to_origin == -1)
        {
            cout << "[Request Failed: Server " << server_id << " has no path to Origin " << origin_server_id_ << ".]" << endl;
            return;
        }

        cout << "Origin server (Node " << origin_server_id_ << ") reached in " << latency_to_origin << "ms." << endl;

        // 6b. Fetch content and put it in the cache
        string new_content = fetch_from_origin(content_id);
        server_node.cache->put(content_id, new_content);
        cout << "Content " << content_id << " cached on Server " << server_id << " and served." << endl;

        // 6c. Report total latency
        cout << "Total Latency: " << (latency_to_server + latency_to_origin) << "ms ("
             << latency_to_server << "ms to server + " << latency_to_origin << "ms to origin)" << endl;
    }
    cout << "----------------------------------------" << endl;
}

unordered_map<int, int> CDNSimulator::run_dijkstra(int start_node_id)
{
    unordered_map<int, int> distances;
    for (auto const &[id, node] : nodes_)
    {
        distances[id] = numeric_limits<int>::max();
    }

    priority_queue<pair<int, int>,
                   vector<pair<int, int>>,
                   greater<pair<int, int>>>
        pq;

    distances[start_node_id] = 0;
    pq.push({0, start_node_id});

    while (!pq.empty())
    {
        int current_dist = pq.top().first;
        int current_id = pq.top().second;
        pq.pop();

        if (current_dist > distances[current_id])
        {
            continue;
        }

        if (adj_list_.find(current_id) == adj_list_.end())
        {
            continue;
        }

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

pair<int, int> CDNSimulator::find_best_server(const unordered_map<int, int> &distances)
{
    int min_latency = numeric_limits<int>::max();
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
        return {-1, -1};
    }
    return {best_server_id, min_latency};
}

int CDNSimulator::get_or_create_content_id(const string &content_name)
{
    if (content_manifest_.find(content_name) == content_manifest_.end())
    {
        content_manifest_[content_name] = next_content_id_;
        next_content_id_++;
    }
    return content_manifest_[content_name];
}

string CDNSimulator::fetch_from_origin(int content_id)
{
    return "DataPayload(ContentID:" + to_string(content_id) + ")";
}

void CDNSimulator::calculate_backbone_mst()
{
    cout << "--- Calculating Network Backbone MST (Prim's Algorithm) ---" << endl;

    if (origin_server_id_ == -1)
    {
        cout << "[MST Failed: No origin server (start node) found.]" << endl;
        return;
    }

    priority_queue<tuple<int, int, int>,
                   vector<tuple<int, int, int>>,
                   greater<tuple<int, int, int>>>
        pq;

    unordered_map<int, bool> in_mst;
    vector<string> mst_edges;
    int total_cost = 0;
    int backbone_nodes_count = 0;

    for (auto const &[id, node] : nodes_)
    {
        if (node.type != Node::NodeType::USER)
        {
            backbone_nodes_count++;
        }
    }

    // Start Prim's algorithm from the origin server
    int start_node = origin_server_id_;
    in_mst[start_node] = true;

    // Add all edges from the start node to the queue
    for (auto const &neighbor : adj_list_[start_node])
    {
        int neighbor_id = neighbor.first;
        int cost = neighbor.second;
        if (nodes_.at(neighbor_id).type != Node::NodeType::USER)
        {
            pq.push({cost, neighbor_id, start_node});
        }
    }

    while (!pq.empty() && mst_edges.size() < backbone_nodes_count - 1)
    {
        auto [cost, to_node, from_node] = pq.top();
        pq.pop();

        // If this node is already in the MST, skip it
        if (in_mst.count(to_node))
        {
            continue;
        }

        in_mst[to_node] = true; // Add the new node to the tree
        total_cost += cost;
        mst_edges.push_back("  (" + to_string(from_node) + " <--> " + to_string(to_node) + ") Cost: " + to_string(cost));

        // Add all of this new node's edges to the queue
        for (auto const &neighbor : adj_list_[to_node])
        {
            int neighbor_id = neighbor.first;
            int neighbor_cost = neighbor.second;

            // Only add if neighbor is a backbone node and not already in the MST
            if (nodes_.at(neighbor_id).type != Node::NodeType::USER && !in_mst.count(neighbor_id))
            {
                pq.push({neighbor_cost, neighbor_id, to_node});
            }
        }
    }

    cout << "MST Calculation Complete." << endl;
    cout << "Total Backbone Nodes: " << backbone_nodes_count << endl;
    cout << "Edges in MST: " << mst_edges.size() << endl;
    cout << "Total Cost: " << total_cost << endl;
    cout << "Edges:" << endl;
    for (const auto &edge_str : mst_edges)
    {
        cout << edge_str << endl;
    }
    cout << "----------------------------------------" << endl;
}