const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');

// Define structures
const PhantomAccount = Struct({
  'seed': refArray(ref.types.uint8, 32),
  'id': refArray(ref.types.char, 65),
  'creation_time': ref.types.uint64,
  'expiry_time': ref.types.uint64
});

// Create FFI interface
const phantomLib = ffi.Library('libphantomid', {
  'phantom_init': ['bool', ['pointer', 'uint16']],
  'phantom_cleanup': ['void', ['pointer']],
  'phantom_tree_insert': ['pointer', ['pointer', PhantomAccount, 'string']],
  'phantom_tree_delete': ['bool', ['pointer', 'string']],
  'phantom_message_send': ['bool', ['pointer', 'string', 'string', 'string']]
});

class PhantomID {
  constructor() {
    this.daemon = ref.alloc('pointer');
  }

  init(port = 8888) {
    return phantomLib.phantom_init(this.daemon, port);
  }

  cleanup() {
    phantomLib.phantom_cleanup(this.daemon);
  }

  createAccount(parentId = null) {
    const account = new PhantomAccount();
    const node = phantomLib.phantom_tree_insert(this.daemon, account, parentId);
    
    if (node.isNull()) {
      throw new Error('Failed to create account');
    }
    
    return {
      id: node.account.id,
      isRoot: node.is_root
    };
  }

  sendMessage(fromId, toId, message) {
    return phantomLib.phantom_message_send(this.daemon, fromId, toId, message);
  }
}

module.exports = PhantomID;
