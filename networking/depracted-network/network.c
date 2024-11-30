#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Program-driven network types based on common operations
typedef enum {
    NET_TCP,
    NET_UDP,
    NET_RAW
} NetworkProtocol;

typedef enum {
    NET_CLIENT,
    NET_SERVER,
    NET_PEER
} NetworkRole;

typedef enum {
    NET_BLOCKING,
    NET_NONBLOCKING
} NetworkMode;

// Network endpoint definition driven by program requirements
typedef struct {
    char address[INET_ADDRSTRLEN];
    uint16_t port;
    NetworkProtocol protocol;
    NetworkRole role;
    NetworkMode mode;
    int socket_fd;
    struct sockaddr_in addr;
} NetworkEndpoint;

// Network operations as language primitives
typedef struct {
    void* data;
    size_t size;
    uint32_t flags;
} NetworkPacket;

// Core network operations
bool net_init(NetworkEndpoint* endpoint) {
    endpoint->socket_fd = socket(AF_INET, 
        endpoint->protocol == NET_TCP ? SOCK_STREAM : SOCK_DGRAM,
        0);
    
    if (endpoint->socket_fd < 0) {
        return false;
    }
    
    endpoint->addr.sin_family = AF_INET;
    endpoint->addr.sin_port = htons(endpoint->port);
    inet_pton(AF_INET, endpoint->address, &endpoint->addr.sin_addr);
    
    if (endpoint->role == NET_SERVER) {
        bind(endpoint->socket_fd, (struct sockaddr*)&endpoint->addr, 
             sizeof(endpoint->addr));
        if (endpoint->protocol == NET_TCP) {
            listen(endpoint->socket_fd, 5);
        }
    }
    
    return true;
}

// Network primitives
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

// Higher-level network operations
typedef struct {
    NetworkEndpoint* endpoints;
    size_t count;
    void (*on_receive)(NetworkEndpoint*, NetworkPacket*);
    void (*on_connect)(NetworkEndpoint*);
    void (*on_disconnect)(NetworkEndpoint*);
} NetworkProgram;

// Network event loop
void net_run(NetworkProgram* program) {
    fd_set readfds;
    int max_fd = 0;
    
    while (1) {
        FD_ZERO(&readfds);
        
        for (size_t i = 0; i < program->count; i++) {
            FD_SET(program->endpoints[i].socket_fd, &readfds);
            if (program->endpoints[i].socket_fd > max_fd) {
                max_fd = program->endpoints[i].socket_fd;
            }
        }
        
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) > 0) {
            for (size_t i = 0; i < program->count; i++) {
                if (FD_ISSET(program->endpoints[i].socket_fd, &readfds)) {
                    NetworkPacket packet = {0};
                    char buffer[4096];
                    packet.data = buffer;
                    packet.size = sizeof(buffer);
                    
                    ssize_t received = net_receive(&program->endpoints[i], &packet);
                    if (received > 0 && program->on_receive) {
                        program->on_receive(&program->endpoints[i], &packet);
                    }
                }
            }
        }
    }
}
