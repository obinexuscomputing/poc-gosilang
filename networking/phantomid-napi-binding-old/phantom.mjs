// phantom.mjs
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

// FFI approach with ESM
const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');

export class PhantomID {
  #daemon;
  #lib;

  constructor() {
    this.#daemon = ref.alloc('pointer');
    this.#lib = this.#loadLibrary();
  }

  #loadLibrary() {
    return ffi.Library('libphantomid', {
      'phantom_init': ['bool', ['pointer', 'uint16']],
      'phantom_cleanup': ['void', ['pointer']],
      'phantom_tree_insert': ['pointer', ['pointer', 'pointer', 'string']],
      'phantom_message_send': ['bool', ['pointer', 'string', 'string', 'string']]
    });
  }

  async init(port = 8888) {
    return this.#lib.phantom_init(this.#daemon, port);
  }

  async createAccount(parentId = null) {
    // Implementation
  }

  async sendMessage(fromId, toId, content) {
    // Implementation
  }
}

// For N-API approach
export const PhantomNative = require('./build/Release/phantom_binding.node');