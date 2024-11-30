#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "phantomid.h"

static PhantomDaemon daemon;
static volatile bool running = true;

void handle_signal(int sig) {
    running = false;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT    Port to listen on (default: 8888)\n");
    printf("  -h, --help         Show this help message\n");
}

int main(int argc, char* argv[]) {
    // Default port
    uint16_t port = 8888;
    
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
                    i++; // Skip the port number in next iteration
                } else {
                    fprintf(stderr, "Invalid port number. Must be between 1 and 65535\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Port number not provided\n");
                return 1;
            }
        }
    }
    
    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Initialize PhantomID daemon with specified port
    if (!phantom_init(&daemon, port)) {
        printf("Failed to initialize PhantomID daemon\n");
        return 1;
    }
    
    printf("PhantomID daemon initialized on port %d\n", port);
    
    // Create test account
    PhantomAccount account = {0};
    if (phantom_create_account(&daemon, &account)) {
        printf("Created anonymous account:\n");
        printf("ID: %s\n", account.id);
        printf("Creation Time: %lu\n", account.creation_time);
        printf("Expiry Time: %lu\n", account.expiry_time);
    }
    
    // Run the daemon
    phantom_run(&daemon);
    
    // Cleanup
    phantom_cleanup(&daemon);
    printf("PhantomID daemon stopped\n");
    
    return 0;
}