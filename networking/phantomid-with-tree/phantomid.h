#ifndef PHANTOMID_H
#define PHANTOMID_H

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // For usleep
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <assert.h>
#include "network.h"

// Function declarations at the top of phantomid.c
// Network function declarations if not in network.h
extern int send_network_data(NetworkEndpoint* endpoint, NetworkPacket* packet);
extern int net_run(NetworkProgram* program);


#define MAX_ACCOUNTS 1000
#define MAX_MESSAGE_SIZE 4096
#define MAX_CHILDREN 10

// Forward declarations
struct PhantomNode;
struct PhantomTree;
struct PhantomMessage;

typedef struct PhantomNode PhantomNode;
typedef struct PhantomTree PhantomTree;
typedef struct PhantomMessage PhantomMessage;

// PhantomID account structure
typedef struct {
    uint8_t seed[32];          // Cryptographic seed
    char id[65];               // Anonymous ID (64 chars + null terminator)
    uint64_t creation_time;    // Account creation timestamp
    uint64_t expiry_time;      // Account expiry timestamp
    pthread_mutex_t lock;      // Thread safety for account operations
} PhantomAccount;

// Message structure
struct PhantomMessage {
    char from_id[65];          // Sender ID
    char to_id[65];           // Recipient ID
    char content[MAX_MESSAGE_SIZE]; // Message content
    time_t timestamp;         // Message timestamp
};

// Tree node structure
struct PhantomNode {
    PhantomAccount account;    // Account information
    struct PhantomNode* parent;// Parent node
    struct PhantomNode** children; // Array of child nodes
    size_t child_count;       // Number of children
    size_t max_children;      // Maximum number of children
    bool is_root;            // Root node flag
    bool is_admin;           // Admin node flag
    pthread_mutex_t node_lock; // Thread safety for node operations
};

// Tree structure
struct PhantomTree {
    PhantomNode* root;        // Root node of the tree
    size_t total_nodes;       // Total number of nodes in tree
    pthread_mutex_t tree_lock; // Thread safety for tree operations
};

// Network handlers declaration
void phantom_on_client_data(NetworkEndpoint* endpoint, NetworkPacket* packet);
void phantom_on_client_connect(NetworkEndpoint* endpoint);
void phantom_on_client_disconnect(NetworkEndpoint* endpoint);

// PhantomID daemon state
typedef struct PhantomDaemon {
    NetworkProgram network;    // Network program for handling connections
    PhantomTree* tree;        // Tree structure for account hierarchy
    pthread_mutex_t state_lock;// Thread safety for daemon state
    bool running;             // Daemon running state
} PhantomDaemon;

// Tree traversal callback type
typedef void (*TreeVisitor)(PhantomNode* node, void* user_data);

// Core initialization and cleanup
bool phantom_tree_init(PhantomDaemon* phantom);
void phantom_tree_cleanup(PhantomDaemon* phantom);

// Tree operations
PhantomNode* phantom_tree_insert(PhantomDaemon* phantom, const PhantomAccount* account, const char* parent_id);
bool phantom_tree_delete(PhantomDaemon* phantom, const char* id);
PhantomNode* phantom_tree_find(PhantomDaemon* phantom, const char* id);

// Tree traversal
void phantom_tree_bfs(PhantomDaemon* phantom, TreeVisitor visitor, void* user_data);
void phantom_tree_dfs(PhantomDaemon* phantom, TreeVisitor visitor, void* user_data);

// Status and information
bool phantom_tree_has_root(const PhantomDaemon* phantom);
size_t phantom_tree_size(const PhantomDaemon* phantom);
size_t phantom_tree_depth(const PhantomDaemon* phantom);
void phantom_tree_print(const PhantomDaemon* phantom);

// Message operations
bool phantom_message_send(PhantomDaemon* phantom, const char* from_id, 
                         const char* to_id, const char* content);
PhantomMessage* phantom_message_get(PhantomDaemon* phantom, const char* id, 
                                  size_t* count);

// Daemon control
bool phantom_init(PhantomDaemon* phantom, uint16_t port);
void phantom_cleanup(PhantomDaemon* phantom);
void phantom_run(PhantomDaemon* phantom);

// Utility functions
const char* phantom_get_error(void);
time_t phantom_get_time(void);

#endif // PHANTOMID_H