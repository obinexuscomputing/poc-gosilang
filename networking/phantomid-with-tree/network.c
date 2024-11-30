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
    if (!endpoint) return false;
    
    pthread_mutex_init(&endpoint->lock, NULL);
    pthread_mutex_lock(&endpoint->lock);
    
    // Create socket
    endpoint->socket_fd = socket(AF_INET, 
        endpoint->protocol == NET_TCP ? SOCK_STREAM : SOCK_DGRAM, 
        0);
    
    if (endpoint->socket_fd < 0) {
        perror("Socket creation failed");
        pthread_mutex_unlock(&endpoint->lock);
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(endpoint->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(endpoint->socket_fd);
        pthread_mutex_unlock(&endpoint->lock);
        return false;
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
            close(endpoint->socket_fd);
            pthread_mutex_unlock(&endpoint->lock);
            return false;
        }
        
        if (endpoint->protocol == NET_TCP) {
            if (listen(endpoint->socket_fd, 5) < 0) {
                perror("Listen failed");
                close(endpoint->socket_fd);
                pthread_mutex_unlock(&endpoint->lock);
                return false;
            }
        }
    }

    pthread_mutex_unlock(&endpoint->lock);
    return true;
}

// Close network endpoint
void net_close(NetworkEndpoint* endpoint) {
    if (!endpoint) return;
    pthread_mutex_lock(&endpoint->lock);
    if (endpoint->socket_fd > 0) {
        close(endpoint->socket_fd);
        endpoint->socket_fd = 0;
    }
    pthread_mutex_unlock(&endpoint->lock);
}

// Send data through network endpoint
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    if (!endpoint || !packet) return -1;
    
    ssize_t result;
    pthread_mutex_lock(&endpoint->lock);
    result = send(endpoint->socket_fd, packet->data, packet->size, packet->flags);
    pthread_mutex_unlock(&endpoint->lock);
    return result;
}

// Receive data through network endpoint
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    if (!endpoint || !packet) return -1;
    
    ssize_t result;
    pthread_mutex_lock(&endpoint->lock);
    result = recv(endpoint->socket_fd, packet->data, packet->size, packet->flags);
    pthread_mutex_unlock(&endpoint->lock);
    return result;
}

// Add client to program
bool net_add_client(NetworkProgram* program, int socket_fd, struct sockaddr_in addr) {
    if (!program) return false;
    
    bool added = false;
    pthread_mutex_lock(&program->clients_lock);
    
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
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
    if (!program) return;
    
    pthread_mutex_lock(&program->clients_lock);
    
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
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
    if (!program) return;
    
    pthread_mutex_init(&program->clients_lock, NULL);
    program->running = true;
    
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        net_init_client_state(&program->clients[i]);
    }
}

// Clean up network program
void net_cleanup_program(NetworkProgram* program) {
    if (!program) return;
    
    pthread_mutex_lock(&program->clients_lock);
    program->running = false;
    
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        net_cleanup_client_state(&program->clients[i]);
    }
    
    pthread_mutex_unlock(&program->clients_lock);
    pthread_mutex_destroy(&program->clients_lock);
}

// Run network program
void net_run(NetworkProgram* program) {
    if (!program || !program->running) return;

    fd_set readfds;
    struct timeval tv = {
        .tv_sec = 1,  // 1 second timeout
        .tv_usec = 0
    };

    // Setup file descriptors
    FD_ZERO(&readfds);
    int max_fd = program->endpoints[0].socket_fd;
    FD_SET(max_fd, &readfds);

    // Add active clients
    pthread_mutex_lock(&program->clients_lock);
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        pthread_mutex_lock(&program->clients[i].lock);
        if (program->clients[i].is_active) {
            int fd = program->clients[i].socket_fd;
            FD_SET(fd, &readfds);
            if (fd > max_fd) max_fd = fd;
        }
        pthread_mutex_unlock(&program->clients[i].lock);
    }
    pthread_mutex_unlock(&program->clients_lock);

    // Wait for activity with timeout
    int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
    
    if (activity < 0) {
        if (errno != EINTR) {
            perror("select error");
        }
        return;
    }

    // Handle new connections
    if (FD_ISSET(program->endpoints[0].socket_fd, &readfds)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int new_socket = accept(program->endpoints[0].socket_fd,
                              (struct sockaddr*)&client_addr,
                              &addr_len);

        if (new_socket >= 0) {
            // Set non-blocking mode
            int flags = fcntl(new_socket, F_GETFL, 0);
            fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

            // Add client
            if (net_add_client(program, new_socket, client_addr)) {
                NetworkEndpoint client_endpoint = {
                    .socket_fd = new_socket,
                    .addr = client_addr,
                    .phantom = program->phantom
                };
                
                if (program->handlers.on_connect) {
                    program->handlers.on_connect(&client_endpoint);
                }
            } else {
                close(new_socket);
            }
        }
    }

    // Handle client data
    pthread_mutex_lock(&program->clients_lock);
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        pthread_mutex_lock(&program->clients[i].lock);
        if (program->clients[i].is_active &&
            FD_ISSET(program->clients[i].socket_fd, &readfds)) {
            
            char buffer[NET_BUFFER_SIZE];
            ssize_t bytes_read = recv(program->clients[i].socket_fd,
                                    buffer,
                                    sizeof(buffer) - 1,
                                    0);

            if (bytes_read <= 0) {
                // Handle disconnection
                NetworkEndpoint client_endpoint = {
                    .socket_fd = program->clients[i].socket_fd,
                    .addr = program->clients[i].addr,
                    .phantom = program->phantom
                };

                if (program->handlers.on_disconnect) {
                    program->handlers.on_disconnect(&client_endpoint);
                }
                
                net_remove_client(program, program->clients[i].socket_fd);
            } else {
                // Handle received data
                NetworkEndpoint client_endpoint = {
                    .socket_fd = program->clients[i].socket_fd,
                    .addr = program->clients[i].addr,
                    .phantom = program->phantom
                };

                NetworkPacket packet = {
                    .data = buffer,
                    .size = bytes_read,
                    .flags = 0
                };

                if (program->handlers.on_receive) {
                    program->handlers.on_receive(&client_endpoint, &packet);
                }
            }
        }
        pthread_mutex_unlock(&program->clients[i].lock);
    }
    pthread_mutex_unlock(&program->clients_lock);
}