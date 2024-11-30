#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// Constants
#define NET_MAX_CLIENTS        10      // Maximum number of concurrent clients
#define NET_BUFFER_SIZE        1024    // Default buffer size for network operations

// Forward declaration of PhantomDaemon
typedef struct PhantomDaemon PhantomDaemon;

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
    char address[INET_ADDRSTRLEN];  // IP address
    uint16_t port;                  // Port number
    NetworkProtocol protocol;       // TCP/UDP
    NetworkRole role;               // Server/Client/Peer
    NetworkMode mode;               // Blocking/Non-blocking
    int socket_fd;                  // Socket file descriptor
    struct sockaddr_in addr;        // Socket address
    PhantomDaemon* phantom;         // Reference to PhantomDaemon
} NetworkEndpoint;

// Network packet with thread safety
typedef struct {
    void* data;                     // Packet data
    size_t size;                    // Data size
    uint32_t flags;                 // Packet flags
    pthread_mutex_t lock;           // Mutex for thread-safe access
} NetworkPacket;

// Thread-safe program state
typedef struct {
    NetworkEndpoint* endpoints;     // Array of endpoints
    size_t count;                   // Number of endpoints
    ClientState clients[NET_MAX_CLIENTS]; // Array of client states
    pthread_mutex_t clients_lock;   // Mutex for client list access
    volatile bool running;          // Server running state
    struct {
        void (*on_receive)(NetworkEndpoint*, NetworkPacket*);  // Receive callback
        void (*on_connect)(NetworkEndpoint*);                  // Connect callback
        void (*on_disconnect)(NetworkEndpoint*);               // Disconnect callback
    } handlers;
    PhantomDaemon* phantom;         // Reference to PhantomDaemon
} NetworkProgram;

// Function declarations
bool net_init(NetworkEndpoint* endpoint);
void net_close(NetworkEndpoint* endpoint);
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet);

// Client management
void net_init_client_state(ClientState* state);
void net_cleanup_client_state(ClientState* state);
bool net_add_client(NetworkProgram* program, int socket_fd, struct sockaddr_in addr);
void net_remove_client(NetworkProgram* program, int socket_fd);

// Program management
void net_init_program(NetworkProgram* program);
void net_cleanup_program(NetworkProgram* program);
void net_run(NetworkProgram* program);

#endif // NETWORK_H