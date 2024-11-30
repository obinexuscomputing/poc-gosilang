# Run with loader
node --experimental-loader=./phantom-loader.mjs index.mjs

# Or with additional flags for debugging
node --experimental-loader=./phantom-loader.mjs --inspect index.mjs

npm install ffi-napi ref-napi ref-struct-napi