#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Network types
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

// Thread-safe client state
typedef struct {
    pthread_mutex_t lock;           // Mutex for thread-safe access
    bool is_active;                 // Is this client slot active?
    int socket_fd;                  // Client socket
    struct sockaddr_in addr;        // Client address
} ClientState;

// Thread-safe endpoint structure
typedef struct {
    pthread_mutex_t lock;           // Mutex for thread-safe access
    char address[INET_ADDRSTRLEN];
    uint16_t port;
    NetworkProtocol protocol;
    NetworkRole role;
    NetworkMode mode;
    int socket_fd;
    struct sockaddr_in addr;
} NetworkEndpoint;

// Network packet with thread safety
typedef struct {
    void* data;
    size_t size;
    uint32_t flags;
    pthread_mutex_t lock;           // Mutex for thread-safe access
} NetworkPacket;

// Thread-safe program state
typedef struct {
    NetworkEndpoint* endpoints;
    size_t count;
    ClientState clients[MAX_CLIENTS];
    pthread_mutex_t clients_lock;   // Mutex for client list access
    volatile bool running;          // Atomic flag for server state
    void (*on_receive)(NetworkEndpoint*, NetworkPacket*);
    void (*on_connect)(NetworkEndpoint*);
    void (*on_disconnect)(NetworkEndpoint*);
} NetworkProgram;

// Function declarations
bool net_init(NetworkEndpoint* endpoint);
bool net_connect(NetworkEndpoint* endpoint);
bool net_accept(NetworkEndpoint* server, NetworkEndpoint* client);
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet);
void net_close(NetworkEndpoint* endpoint);
void net_run(NetworkProgram* program);

// Thread-safe utility functions
void net_init_client_state(ClientState* state);
void net_cleanup_client_state(ClientState* state);
bool net_add_client(NetworkProgram* program, int socket_fd, struct sockaddr_in addr);
void net_remove_client(NetworkProgram* program, int socket_fd);
void net_init_program(NetworkProgram* program);
void net_cleanup_program(NetworkProgram* program);

#endif // NETWORK_H