#include <string.h>
#include <time.h>
#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "phantomid.h"

#define MAX_ACCOUNTS 1000

// Global daemon state
static PhantomDaemon* g_daemon = NULL;

// Generate cryptographic seed
static void generate_seed(uint8_t* seed) {
    RAND_bytes(seed, 32);
}

// Generate anonymous ID from seed using modern EVP interface

static void generate_id(const uint8_t* seed, char* id) {
    unsigned int len;
    uint8_t hash[EVP_MAX_MD_SIZE];
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(ctx, seed, 32);
        EVP_DigestFinal_ex(ctx, hash, &len);
        EVP_MD_CTX_free(ctx);
        
        // Ensure we only use the exact length needed for SHA256 (64 chars)
        for (unsigned int i = 0; i < 32; i++) {
            sprintf(&id[i * 2], "%02x", hash[i]);
        }
        id[64] = '\0';  // Ensure proper null termination
    }
}

static void on_client_data(NetworkEndpoint* endpoint, NetworkPacket* packet) {
    char* data = (char*)packet->data;
    data[packet->size] = '\0';
    
    printf("Received command: %s", data);
    
    char response[1024] = {0};
    NetworkPacket resp = {
        .data = response,
        .size = sizeof(response),
        .flags = 0
    };

    // Parse command
    if (strncmp(data, "create", 6) == 0) {
        PhantomAccount account = {0};
        if (phantom_create_account(g_daemon, &account)) {
            snprintf(response, sizeof(response), 
                    "\nAccount created:\nID: %s\nCreation Time: %lu\nExpiry Time: %lu\n",
                    account.id, account.creation_time, account.expiry_time);
        } else {
            snprintf(response, sizeof(response), "\nFailed to create account\n");
        }
    }
    else if (strncmp(data, "delete", 6) == 0) {
        // Skip "delete " and any whitespace
        char* id = data + 6;
        while (*id == ' ') id++;
        
        if (phantom_delete_account(g_daemon, id)) {
            snprintf(response, sizeof(response), "\nAccount deleted: %s\n", id);
        } else {
            snprintf(response, sizeof(response), "\nFailed to delete account or account not found\n");
        }
    }
    else if (strncmp(data, "list", 4) == 0) {
        pthread_mutex_lock(&g_daemon->state_lock);
        snprintf(response, sizeof(response), "\nActive accounts: %zu\n", g_daemon->account_count);
        size_t offset = strlen(response);
        
        for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
            pthread_mutex_lock(&g_daemon->accounts[i].lock);
            if (g_daemon->accounts[i].creation_time != 0) {
                offset += snprintf(response + offset, sizeof(response) - offset,
                        "ID: %s\nCreated: %lu\nExpires: %lu\n\n",
                        g_daemon->accounts[i].id,
                        g_daemon->accounts[i].creation_time,
                        g_daemon->accounts[i].expiry_time);
            }
            pthread_mutex_unlock(&g_daemon->accounts[i].lock);
        }
        pthread_mutex_unlock(&g_daemon->state_lock);
    }
    else if (strncmp(data, "help", 4) == 0) {
        snprintf(response, sizeof(response),
                "\nAvailable commands:\n"
                "create - Create a new anonymous account\n"
                "delete <id> - Delete an account by ID\n"
                "list - List all active accounts\n"
                "help - Show this help message\n"
                "quit - Disconnect from server\n\n");
    }
    else {
        snprintf(response, sizeof(response), 
                "\nUnknown command. Type 'help' for available commands.\n");
    }
    
    // Send response with actual length
    resp.size = strlen(response);
    if (net_send(endpoint, &resp) < 0) {
        printf("Failed to send response to client\n");
    }
}

// Network callbacks
static void on_client_connect(NetworkEndpoint* endpoint) {
    printf("New client connected for account creation\n");
}

static void on_client_disconnect(NetworkEndpoint* endpoint) {
    printf("Client disconnected\n");
}

bool phantom_init(PhantomDaemon* daemon, uint16_t port) {
    g_daemon = daemon;  // Store global reference
    
    pthread_mutex_init(&daemon->state_lock, NULL);
    
    daemon->accounts = calloc(MAX_ACCOUNTS, sizeof(PhantomAccount));
    if (!daemon->accounts) return false;
    
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_init(&daemon->accounts[i].lock, NULL);
    }
    
    daemon->account_count = 0;
    daemon->running = true;
    
    // Initialize network server with provided port
    NetworkEndpoint server = {
        .address = "0.0.0.0",
        .port = port,
        .protocol = NET_TCP,
        .role = NET_SERVER,
        .mode = NET_BLOCKING
    };
    
    daemon->network.endpoints = malloc(sizeof(NetworkEndpoint));
    if (!daemon->network.endpoints) {
        free(daemon->accounts);
        return false;
    }
    
    memcpy(daemon->network.endpoints, &server, sizeof(NetworkEndpoint));
    daemon->network.count = 1;
    daemon->network.on_connect = on_client_connect;
    daemon->network.on_disconnect = on_client_disconnect;
    daemon->network.on_receive = on_client_data;
    
    return net_init(daemon->network.endpoints);
}

void phantom_cleanup(PhantomDaemon* daemon) {
    pthread_mutex_lock(&daemon->state_lock);
    daemon->running = false;
    
    // Cleanup accounts
    for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_lock(&daemon->accounts[i].lock);
        memset(&daemon->accounts[i], 0, sizeof(PhantomAccount));
        pthread_mutex_unlock(&daemon->accounts[i].lock);
        pthread_mutex_destroy(&daemon->accounts[i].lock);
    }
    
    // Cleanup network endpoints
    if (daemon->network.endpoints) {
        for (size_t i = 0; i < daemon->network.count; i++) {
            pthread_mutex_destroy(&daemon->network.endpoints[i].lock);
            net_close(&daemon->network.endpoints[i]);
        }
        free(daemon->network.endpoints);
    }
    
    free(daemon->accounts);
    pthread_mutex_unlock(&daemon->state_lock);
    pthread_mutex_destroy(&daemon->state_lock);
    
    g_daemon = NULL;
}


bool phantom_create_account(PhantomDaemon* daemon, PhantomAccount* account) {
    bool success = false;
    
    pthread_mutex_lock(&daemon->state_lock);
    if (daemon->account_count < MAX_ACCOUNTS) {
        for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
            pthread_mutex_lock(&daemon->accounts[i].lock);
            if (daemon->accounts[i].creation_time == 0) {  // Found empty slot
                // Generate new account
                generate_seed(account->seed);
                generate_id(account->seed, account->id);
                account->creation_time = time(NULL);
                account->expiry_time = account->creation_time + (90 * 24 * 60 * 60); // 90 days
                
                // Copy to daemon storage
                memcpy(&daemon->accounts[i], account, sizeof(PhantomAccount));
                daemon->account_count++;
                success = true;
                pthread_mutex_unlock(&daemon->accounts[i].lock);
                break;
            }
            pthread_mutex_unlock(&daemon->accounts[i].lock);
        }
    }
    pthread_mutex_unlock(&daemon->state_lock);
    
    return success;
}

bool phantom_delete_account(PhantomDaemon* daemon, const char* id) {
    bool success = false;
    
    pthread_mutex_lock(&daemon->state_lock);
    for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_lock(&daemon->accounts[i].lock);
        if (daemon->accounts[i].creation_time != 0 && strcmp(daemon->accounts[i].id, id) == 0) {
            memset(&daemon->accounts[i], 0, sizeof(PhantomAccount));
            daemon->account_count--;
            success = true;
            pthread_mutex_unlock(&daemon->accounts[i].lock);
            break;
        }
        pthread_mutex_unlock(&daemon->accounts[i].lock);
    }
    pthread_mutex_unlock(&daemon->state_lock);
    
    return success;
}

void phantom_run(PhantomDaemon* daemon) {
    printf("PhantomID daemon starting...\n");
    net_run(&daemon->network);
}