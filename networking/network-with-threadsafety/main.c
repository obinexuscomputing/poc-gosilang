#include <stdio.h>
#include <arpa/inet.h>
#include "network.h"

void on_data_received(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("DEBUG: Received %zd bytes\n", packet->size);
    printf("Received data from %s:%d\n", 
           addr, 
           ntohs(endpoint->addr.sin_port));
    printf("Data: %.*s\n", (int)packet->size, (char*)packet->data);
    fflush(stdout);  // Force output to be displayed immediately
}

void on_peer_connected(NetworkEndpoint* endpoint) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("DEBUG: New connection detected\n");
    printf("New connection from %s:%d\n",
           addr,
           ntohs(endpoint->addr.sin_port));
    fflush(stdout);
}

void on_peer_disconnected(NetworkEndpoint* endpoint) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("Peer disconnected: %s:%d\n",
           addr,
           ntohs(endpoint->addr.sin_port));
    fflush(stdout);
}

int main() {
    // Define server endpoint
    NetworkEndpoint server = {
        .address = "0.0.0.0",
        .port = 8080,
        .protocol = NET_TCP,
        .role = NET_SERVER,
        .mode = NET_BLOCKING
    };
    
    // Initialize the network program
    NetworkProgram program = {
        .endpoints = &server,
        .count = 1,
        .on_receive = on_data_received,
        .on_connect = on_peer_connected,
        .on_disconnect = on_peer_disconnected
    };
    
    // Initialize and start the server
    if (!net_init(&server)) {
        printf("Failed to initialize server\n");
        return 1;
    }
    
    printf("Server running on port 8080...\n");
    fflush(stdout);
    net_run(&program);
    
    return 0;
}
