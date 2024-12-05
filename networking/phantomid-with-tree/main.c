#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #define sleep(x) Sleep(x * 1000)
    #define usleep(x) Sleep(x / 1000)
#else
    #include <unistd.h>
    #include <signal.h>
#endif
#include "phantomid.h"

static PhantomDaemon phantom_daemon;
static volatile bool running = true;

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("\nReceived signal %d, initiating shutdown...\n", sig);
    running = false;
}

// Print program usage
void print_usage(const char* program_name) {
    printf("PhantomID Daemon - Anonymous Account Management System\n\n");
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT    Port to listen on (default: 8888)\n");
    printf("  -v, --verbose      Enable verbose logging\n");
    printf("  -d, --debug        Enable debug mode\n");
    printf("  -h, --help         Show this help message\n");
}

// Initialize signal handlers
void setup_signals(void) {
#ifdef _WIN32
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#else
    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
#endif
}

// Print system status
void print_status(const PhantomDaemon* phantom) {
    printf("\nPhantomID System Status:\n");
    printf("------------------------\n");
    printf("Total Nodes: %zu\n", phantom_tree_size(phantom));
    printf("Root Exists: %s\n", phantom_tree_has_root(phantom) ? "Yes" : "No");
    printf("Tree Depth: %zu\n", phantom_tree_depth(phantom));
    printf("System Time: %lld\n", (long long)phantom_get_time());
    printf("------------------------\n");
}

// Debug visitor function for tree traversal
void debug_visitor(PhantomNode* node, void* user_data) {
    bool is_verbose = *(bool*)user_data;
    printf("Node ID: %s (Root: %s, Admin: %s)\n",
           node->account.id,
           node->is_root ? "Yes" : "No",
           node->is_admin ? "Yes" : "No");
    
    if (is_verbose) {
        printf("  Children: %zu/%zu\n", node->child_count, node->max_children);
        printf("  Created: %llu\n", (unsigned long long)node->account.creation_time);
        printf("  Expires: %llu\n", (unsigned long long)node->account.expiry_time);
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock\n");
        return 1;
    }
#endif

    uint16_t port = 8888;
    bool verbose = false;
    bool debug = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                int temp_port = atoi(argv[i + 1]);
                if (temp_port > 0 && temp_port < 65536) {
                    port = (uint16_t)temp_port;
                    i++;
                } else {
                    fprintf(stderr, "Invalid port number. Must be between 1 and 65535\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Port number not provided\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug = true;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    setup_signals();
    
    printf("Initializing PhantomID daemon on port %d...\n", port);
    if (!phantom_init(&phantom_daemon, port)) {
        fprintf(stderr, "Failed to initialize PhantomID daemon: %s\n", 
                phantom_get_error());
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    if (verbose || debug) {
        print_status(&phantom_daemon);
        
        if (debug) {
            printf("\nTree structure (BFS):\n");
            phantom_tree_bfs(&phantom_daemon, debug_visitor, &verbose);
            
            printf("\nTree structure (DFS):\n");
            phantom_tree_dfs(&phantom_daemon, debug_visitor, &verbose);
        }
    }
    
    printf("\nPhantomID daemon is running. Press Ctrl+C to stop.\n");
    
    while (running) {
        phantom_run(&phantom_daemon);
        
        if (verbose) {
            print_status(&phantom_daemon);
        }
        
        usleep(100000); // 100ms
    }
    
    printf("\nCleaning up PhantomID daemon...\n");
    phantom_cleanup(&phantom_daemon);
    printf("PhantomID daemon stopped successfully\n");
    
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}