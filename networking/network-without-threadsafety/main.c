#include <stdio.h>
#include <arpa/inet.h>
#include "network.h"

void on_data_received(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("\nReceived data from %s:%d\n", 
           addr, 
           ntohs(endpoint->addr.sin_port));
    printf("Data: %.*s", (int)packet->size, (char*)packet->data);
    fflush(stdout);
}

void on_peer_connected(NetworkEndpoint* endpoint) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("\nNew connection from %s:%d\n",
           addr,
           ntohs(endpoint->addr.sin_port));
    fflush(stdout);
}

void on_peer_disconnected(NetworkEndpoint* endpoint) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &endpoint->addr.sin_addr, addr, INET_ADDRSTRLEN);
    
    printf("\nPeer disconnected: %s:%d\n",
           addr,
           ntohs(endpoint->addr.sin_port));
    fflush(stdout);
}

int main() {
    NetworkEndpoint server = {
        .address = "0.0.0.0",
        .port = 8080,
        .protocol = NET_TCP,
        .role = NET_SERVER,
        .mode = NET_BLOCKING
    };
    
    NetworkProgram program = {
        .endpoints = &server,
        .count = 1,
        .on_receive = on_data_received,
        .on_connect = on_peer_connected,
        .on_disconnect = on_peer_disconnected
    };
    
    if (!net_init(&server)) {
        printf("Failed to initialize server\n");
        return 1;
    }
    
    printf("Server running on port 8080...\n");
    net_run(&program);
    
    return 0;
}