#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include "network.h"

// Initialize client state
void net_init_client_state(ClientState* state) {
    pthread_mutex_init(&state->lock, NULL);
    state->is_active = false;
    state->socket_fd = 0;
    memset(&state->addr, 0, sizeof(state->addr));
}

// Clean up client state
void net_cleanup_client_state(ClientState* state) {
    pthread_mutex_lock(&state->lock);
    if (state->socket_fd > 0) {
        close(state->socket_fd);
        state->socket_fd = 0;
    }
    state->is_active = false;
    pthread_mutex_unlock(&state->lock);
    pthread_mutex_destroy(&state->lock);
}

// Initialize network endpoint
bool net_init(NetworkEndpoint* endpoint) {
    int result = true;
    pthread_mutex_init(&endpoint->lock, NULL);
    
    pthread_mutex_lock(&endpoint->lock);
    
    // Create socket
    endpoint->socket_fd = socket(AF_INET, 
        endpoint->protocol == NET_TCP ? SOCK_STREAM : SOCK_DGRAM, 
        0);
    
    if (endpoint->socket_fd < 0) {
        perror("Socket creation failed");
        result = false;
        goto cleanup;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(endpoint->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        result = false;
        goto cleanup;
    }
    
    // Configure address
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(endpoint->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    endpoint->addr = addr;
    
    // For server endpoints
    if (endpoint->role == NET_SERVER) {
        if (bind(endpoint->socket_fd, (struct sockaddr*)&endpoint->addr, sizeof(endpoint->addr)) < 0) {
            perror("Bind failed");
            result = false;
            goto cleanup;
        }
        
        if (endpoint->protocol == NET_TCP) {
            if (listen(endpoint->socket_fd, 5) < 0) {
                perror("Listen failed");
                result = false;
                goto cleanup;
            }
        }
    }

cleanup:
    pthread_mutex_unlock(&endpoint->lock);
    if (!result && endpoint->socket_fd > 0) {
        close(endpoint->socket_fd);
        endpoint->socket_fd = 0;
    }
    return result;
}

// Close network endpoint
void net_close(NetworkEndpoint* endpoint) {
    pthread_mutex_lock(&endpoint->lock);
    if (endpoint->socket_fd > 0) {
        close(endpoint->socket_fd);
        endpoint->socket_fd = 0;
    }
    pthread_mutex_unlock(&endpoint->lock);
}

// Send data through network endpoint
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    ssize_t result;
    pthread_mutex_lock(&endpoint->lock);
    result = send(endpoint->socket_fd, packet->data, packet->size, packet->flags);
    pthread_mutex_unlock(&endpoint->lock);
    return result;
}

// Receive data through network endpoint
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    ssize_t result;
    pthread_mutex_lock(&endpoint->lock);
    result = recv(endpoint->socket_fd, packet->data, packet->size, packet->flags);
    pthread_mutex_unlock(&endpoint->lock);
    return result;
}

// Add client to program
bool net_add_client(NetworkProgram* program, int socket_fd, struct sockaddr_in addr) {
    bool added = false;
    pthread_mutex_lock(&program->clients_lock);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_mutex_lock(&program->clients[i].lock);
        if (!program->clients[i].is_active) {
            program->clients[i].socket_fd = socket_fd;
            program->clients[i].addr = addr;
            program->clients[i].is_active = true;
            added = true;
            pthread_mutex_unlock(&program->clients[i].lock);
            break;
        }
        pthread_mutex_unlock(&program->clients[i].lock);
    }
    
    pthread_mutex_unlock(&program->clients_lock);
    return added;
}

// Remove client from program
void net_remove_client(NetworkProgram* program, int socket_fd) {
    pthread_mutex_lock(&program->clients_lock);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_mutex_lock(&program->clients[i].lock);
        if (program->clients[i].is_active && program->clients[i].socket_fd == socket_fd) {
            close(program->clients[i].socket_fd);
            program->clients[i].is_active = false;
            program->clients[i].socket_fd = 0;
        }
        pthread_mutex_unlock(&program->clients[i].lock);
    }
    
    pthread_mutex_unlock(&program->clients_lock);
}

// Initialize network program
void net_init_program(NetworkProgram* program) {
    pthread_mutex_init(&program->clients_lock, NULL);
    program->running = true;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        net_init_client_state(&program->clients[i]);
    }
}

// Clean up network program
void net_cleanup_program(NetworkProgram* program) {
    pthread_mutex_lock(&program->clients_lock);
    program->running = false;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        net_cleanup_client_state(&program->clients[i]);
    }
    
    pthread_mutex_unlock(&program->clients_lock);
    pthread_mutex_destroy(&program->clients_lock);
}

// Run network program
void net_run(NetworkProgram* program) {
    fd_set readfds;
    int max_sd;
    char buffer[BUFFER_SIZE];
    
    net_init_program(program);
    printf("Server started, waiting for connections...\n");

    while (program->running) {
        FD_ZERO(&readfds);
        max_sd = 0;

        // Add main server socket
        pthread_mutex_lock(&program->endpoints[0].lock);
        FD_SET(program->endpoints[0].socket_fd, &readfds);
        max_sd = program->endpoints[0].socket_fd;
        pthread_mutex_unlock(&program->endpoints[0].lock);

        // Add client sockets
        pthread_mutex_lock(&program->clients_lock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            pthread_mutex_lock(&program->clients[i].lock);
            if (program->clients[i].is_active) {
                FD_SET(program->clients[i].socket_fd, &readfds);
                if (program->clients[i].socket_fd > max_sd) {
                    max_sd = program->clients[i].socket_fd;
                }
            }
            pthread_mutex_unlock(&program->clients[i].lock);
        }
        pthread_mutex_unlock(&program->clients_lock);

        // Wait for activity
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) continue;

        // Check server socket
        if (FD_ISSET(program->endpoints[0].socket_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_socket = accept(program->endpoints[0].socket_fd,
                                  (struct sockaddr*)&client_addr, 
                                  &addr_len);

            if (new_socket >= 0) {
                if (net_add_client(program, new_socket, client_addr)) {
                    NetworkEndpoint client_endpoint = {0};
                    client_endpoint.socket_fd = new_socket;
                    client_endpoint.addr = client_addr;
                    if (program->on_connect) {
                        program->on_connect(&client_endpoint);
                    }
                }
            }
        }

        // Check client sockets
        pthread_mutex_lock(&program->clients_lock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            pthread_mutex_lock(&program->clients[i].lock);
            if (program->clients[i].is_active && 
                FD_ISSET(program->clients[i].socket_fd, &readfds)) {
                
                NetworkEndpoint client_endpoint = {
                    .socket_fd = program->clients[i].socket_fd,
                    .addr = program->clients[i].addr
                };
                
                NetworkPacket packet = {
                    .data = buffer,
                    .size = BUFFER_SIZE,
                    .flags = 0
                };

                ssize_t valread = net_receive(&client_endpoint, &packet);
                
                if (valread <= 0) {
                    if (program->on_disconnect) {
                        program->on_disconnect(&client_endpoint);
                    }
                    net_remove_client(program, program->clients[i].socket_fd);
                }
                else if (program->on_receive) {
                    packet.size = valread;
                    program->on_receive(&client_endpoint, &packet);
                }
            }
            pthread_mutex_unlock(&program->clients[i].lock);
        }
        pthread_mutex_unlock(&program->clients_lock);
    }

    net_cleanup_program(program);
}
