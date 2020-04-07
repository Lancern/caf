let global = this;

// Initialize all native modules.
builtinModules = [
    "assert",
    "async_hooks",
    "buffer",
    "child_process",
    "cluster",
    "console",
    "constants",
    "crypto",
    "dgram",
    "dns",
    "domain",
    "events",
    "fs",
    "http",
    "http2",
    "https",
    "inspector",
    "module",
    "net",
    "os",
    "path",
    "perf_hooks",
    "process",
    "punycode",
    "querystring",
    "readline",
    "repl",
    "stream",
    "string_decoder",
    "sys",
    "timers",
    "tls",
    "trace_events",
    "tty",
    "url",
    "util",
    "v8",
    "vm",
    "worker_threads",
    "zlib"
]
builtinModules.forEach(name => {
    let module = require(name);
    Object.defineProperty(global, name, {
        enumerable: true,
        value: module,
        writable: false
    });
});

try {
    require('./caf').run(global)
} catch (e) {
    // Do nothing.
}
