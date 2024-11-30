// phantom_binding.c
#define NAPI_VERSION 1
#include <node_api.h>
#include "phantomid.h"

static PhantomDaemon phantom;

// Wrapper for initialization
napi_value Init(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t argc = 1;
    napi_value args[1];
    status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    
    // Get port number from args
    uint32_t port;
    status = napi_get_value_uint32(env, args[0], &port);
    
    bool result = phantom_init(&phantom, (uint16_t)port);
    
    napi_value return_value;
    status = napi_get_boolean(env, result, &return_value);
    
    return return_value;
}

// Wrapper for cleanup
napi_value Cleanup(napi_env env, napi_callback_info info) {
    phantom_cleanup(&phantom);
    
    napi_value return_value;
    napi_get_undefined(env, &return_value);
    return return_value;
}

// Wrapper for create account
napi_value CreateAccount(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t argc = 1;
    napi_value args[1];
    status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    
    // Get parent ID if provided
    char parent_id[65] = {0};
    size_t parent_id_length;
    status = napi_get_value_string_utf8(env, args[0], parent_id, 65, &parent_id_length);
    
    // Create account
    PhantomAccount account = {0};
    PhantomNode* node = phantom_tree_insert(&phantom, &account, parent_id_length > 0 ? parent_id : NULL);
    
    // Create return object
    napi_value return_obj;
    status = napi_create_object(env, &return_obj);
    
    if (node) {
        napi_value id_value;
        status = napi_create_string_utf8(env, node->account.id, strlen(node->account.id), &id_value);
        status = napi_set_named_property(env, return_obj, "id", id_value);
        
        napi_value is_root;
        status = napi_get_boolean(env, node->is_root, &is_root);
        status = napi_set_named_property(env, return_obj, "isRoot", is_root);
    }
    
    return return_obj;
}

// Initialize all functions
napi_value Init(napi_env env, napi_value exports) {
    napi_status status;
    napi_value fn;
    
    // Initialize function
    status = napi_create_function(env, NULL, 0, Init, NULL, &fn);
    status = napi_set_named_property(env, exports, "init", fn);
    
    // Cleanup function
    status = napi_create_function(env, NULL, 0, Cleanup, NULL, &fn);
    status = napi_set_named_property(env, exports, "cleanup", fn);
    
    // Create account function
    status = napi_create_function(env, NULL, 0, CreateAccount, NULL, &fn);
    status = napi_set_named_property(env, exports, "createAccount", fn);
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
