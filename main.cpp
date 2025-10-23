#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "./cdnSim/cdn_sim.hpp"
using namespace std;

void load_config_from_file(const string &filename, CDNSimulator &simulator)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error: Could not open config file " << filename << endl;
        return;
    }

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        stringstream ss(line);
        string keyword;
        ss >> keyword;

        if (keyword == "NODE")
        {
            string type_str;
            int id;
            int capacity = 0;
            ss >> type_str >> id;

            Node::NodeType type;
            if (type_str == "SERVER")
            {
                type = Node::NodeType::EDGE_SERVER;
                ss >> capacity;
            }
            else if (type_str == "GATEWAY")
            {
                type = Node::NodeType::GATEWAY;
            }
            else if (type_str == "ORIGIN")
            {
                type = Node::NodeType::ORIGIN;
            }
            else
            {
                cerr << "Warning: Unknown node type '" << type_str << "'. Skipping." << endl;
                continue;
            }
            simulator.add_node(id, type, capacity);
        }
        else if (keyword == "EDGE")
        {
            int u, v, weight;
            ss >> u >> v >> weight;
            simulator.add_edge(u, v, weight);
        }
    }
}

void run_cli(CDNSimulator &simulator)
{
    string line;
    cout << "\nCDN Simulator CLI. Type 'help' for commands." << endl;

    while (true)
    {
        cout << "> ";
        if (!getline(cin, line))
        {
            break;
        }
        if (line.empty())
        {
            continue;
        }

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "addUser")
        {
            int user_id, connect_to_id, latency;
            if (!(ss >> user_id >> connect_to_id >> latency))
            {
                cout << "Usage: addUser <newUserID> <connectToNodeID> <latency>" << endl;
            }
            else
            {
                simulator.add_node(user_id, Node::NodeType::USER);
                simulator.add_edge(user_id, connect_to_id, latency);
                cout << "User " << user_id << " added and connected to " << connect_to_id << "." << endl;
            }
        }
        else if (command == "request")
        {
            int user_id;
            string content_name;
            if (!(ss >> user_id >> content_name))
            {
                cout << "Usage: request <userID> <contentName>" << endl;
            }
            else
            {
                simulator.simulate_request(user_id, content_name);
            }
        }
        else if (command == "mst")
        {
            simulator.calculate_backbone_mst();
        }
        else if (command == "help")
        {
            cout << "Commands:\n"
                 << "  addUser <newUserID> <connectToNodeID> <latency>\n"
                 << "  request <userID> <contentName>\n"
                 << "  mst\n"
                 << "  exit\n";
        }
        else if (command == "exit")
        {
            cout << "Exiting simulator." << endl;
            break;
        }
        else
        {
            cout << "Unknown command. Type 'help' for commands." << endl;
        }
    }
}

int main()
{
    // 1. Create the simulator object
    CDNSimulator simulator;

    // 2. Load the base network from the file
    load_config_from_file("config.txt", simulator);
    cout << "Initial network configuration loaded from config.txt." << endl;

    // 3. Run the interactive CLI
    run_cli(simulator);

    return 0;
}