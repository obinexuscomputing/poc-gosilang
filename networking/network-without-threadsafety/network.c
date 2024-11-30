#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include "network.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

bool net_init(NetworkEndpoint* endpoint) {
    // Create socket
    endpoint->socket_fd = socket(AF_INET, 
        endpoint->protocol == NET_TCP ? SOCK_STREAM : SOCK_DGRAM,
        0);
    
    if (endpoint->socket_fd < 0) {
        perror("Socket creation failed");
        return false;
    }

    // Set socket options for reuse
    int opt = 1;
    if (setsockopt(endpoint->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return false;
    }
    
    endpoint->addr.sin_family = AF_INET;
    endpoint->addr.sin_port = htons(endpoint->port);
    endpoint->addr.sin_addr.s_addr = INADDR_ANY;
    
    if (endpoint->role == NET_SERVER) {
        if (bind(endpoint->socket_fd, (struct sockaddr*)&endpoint->addr, 
                sizeof(endpoint->addr)) < 0) {
            perror("Bind failed");
            return false;
        }
        if (endpoint->protocol == NET_TCP) {
            if (listen(endpoint->socket_fd, 5) < 0) {
                perror("Listen failed");
                return false;
            }
        }
    }
    
    return true;
}

bool net_connect(NetworkEndpoint* endpoint) {
    if (endpoint->role != NET_CLIENT) return false;
    return connect(endpoint->socket_fd, (struct sockaddr*)&endpoint->addr,
                  sizeof(endpoint->addr)) == 0;
}

bool net_accept(NetworkEndpoint* server, NetworkEndpoint* client) {
    if (server->role != NET_SERVER) return false;
    
    socklen_t addr_len = sizeof(client->addr);
    client->socket_fd = accept(server->socket_fd, 
                             (struct sockaddr*)&client->addr,
                             &addr_len);
    
    return client->socket_fd >= 0;
}

ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    return send(endpoint->socket_fd, packet->data, packet->size, packet->flags);
}

ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    return recv(endpoint->socket_fd, packet->data, packet->size, packet->flags);
}

void net_close(NetworkEndpoint* endpoint) {
    close(endpoint->socket_fd);
}

void net_run(NetworkProgram* program) {
    fd_set readfds;
    NetworkEndpoint clients[MAX_CLIENTS] = {0};
    int max_clients = MAX_CLIENTS;
    int activity, i;
    int max_sd;
    char buffer[BUFFER_SIZE];

    // Clear the client socket set
    for (i = 0; i < max_clients; i++) {
        clients[i].socket_fd = 0;
    }

    printf("Server started, waiting for connections...\n");

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(program->endpoints[0].socket_fd, &readfds);
        max_sd = program->endpoints[0].socket_fd;

        // Add child sockets to set
        for (i = 0; i < max_clients; i++) {
            int sd = clients[i].socket_fd;

            // If valid socket descriptor then add to read list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // Highest file descriptor number, need it for the select function
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for an activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            printf("Select error: %s\n", strerror(errno));
            continue;
        }

        // If something happened on the server socket,
        // then its an incoming connection
        if (FD_ISSET(program->endpoints[0].socket_fd, &readfds)) {
            NetworkEndpoint new_client = {0};
            socklen_t addrlen = sizeof(new_client.addr);

            int new_socket = accept(program->endpoints[0].socket_fd,
                (struct sockaddr *)&new_client.addr, &addrlen);

            if (new_socket < 0) {
                perror("Accept failed");
                continue;
            }

            printf("New connection, socket fd is %d\n", new_socket);

            // Add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                if (clients[i].socket_fd == 0) {
                    clients[i] = new_client;
                    clients[i].socket_fd = new_socket;
                    printf("Adding to list of sockets at index %d\n", i);
                    if (program->on_connect) {
                        program->on_connect(&clients[i]);
                    }
                    break;
                }
            }
        }

        // Check for I/O operations on client sockets
        for (i = 0; i < max_clients; i++) {
            int sd = clients[i].socket_fd;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                // Read incoming message
                NetworkPacket packet = {
                    .data = buffer,
                    .size = BUFFER_SIZE,
                    .flags = 0
                };

                ssize_t valread = recv(sd, buffer, BUFFER_SIZE, 0);

                if (valread == 0) {
                    // Client disconnected
                    if (program->on_disconnect) {
                        program->on_disconnect(&clients[i]);
                    }
                    close(sd);
                    clients[i].socket_fd = 0;
                }
                else if (valread > 0) {
                    packet.size = valread;
                    if (program->on_receive) {
                        program->on_receive(&clients[i], &packet);
                    }
                }
            }
        }
    }
}