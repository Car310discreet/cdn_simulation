#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "./cdnSim/cdn_sim.hpp" // Include the simulator we just built

/**
 * @brief Loads the initial network configuration from a text file.
 */
void load_config_from_file(const std::string &filename, CDNSimulator &simulator)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open config file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue; // Skip empty lines and comments
        }

        std::stringstream ss(line);
        std::string keyword;
        ss >> keyword;

        if (keyword == "NODE")
        {
            std::string type_str;
            int id;
            int capacity = 0;
            ss >> type_str >> id;

            Node::NodeType type;
            if (type_str == "SERVER")
            {
                type = Node::NodeType::EDGE_SERVER;
                ss >> capacity; // Only servers have capacity
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
                std::cerr << "Warning: Unknown node type '" << type_str << "'. Skipping." << std::endl;
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

/**
 * @brief Runs the interactive Command-Line Interface (CLI).
 */
void run_cli(CDNSimulator &simulator)
{
    std::string line;
    std::cout << "\nCDN Simulator CLI. Type 'help' for commands." << std::endl;

    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
        {
            break; // End of input (e.g., Ctrl+D)
        }
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "addUser")
        {
            int user_id, connect_to_id, latency;
            if (!(ss >> user_id >> connect_to_id >> latency))
            {
                std::cout << "Usage: addUser <newUserID> <connectToNodeID> <latency>" << std::endl;
            }
            else
            {
                simulator.add_node(user_id, Node::NodeType::USER);
                simulator.add_edge(user_id, connect_to_id, latency);
                std::cout << "User " << user_id << " added and connected to " << connect_to_id << "." << std::endl;
            }
        }
        else if (command == "request")
        {
            int user_id;
            std::string content_name;
            if (!(ss >> user_id >> content_name))
            {
                std::cout << "Usage: request <userID> <contentName>" << std::endl;
            }
            else
            {
                simulator.simulate_request(user_id, content_name);
            }
        }
        else if (command == "help")
        {
            std::cout << "Commands:\n"
                      << "  addUser <newUserID> <connectToNodeID> <latency>\n"
                      << "  request <userID> <contentName>\n"
                      << "  exit\n";
        }
        else if (command == "exit")
        {
            std::cout << "Exiting simulator." << std::endl;
            break;
        }
        else
        {
            std::cout << "Unknown command. Type 'help' for commands." << std::endl;
        }
    }
}

/**
 * @brief Main entry point of the program.
 */
int main()
{
    // 1. Create the simulator object
    CDNSimulator simulator;

    // 2. Load the base network from the file
    load_config_from_file("config.txt", simulator);
    std::cout << "Initial network configuration loaded from config.txt." << std::endl;

    // 3. Run the interactive CLI
    run_cli(simulator);

    return 0;
}