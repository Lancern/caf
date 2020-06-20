'use strict';

class FunctionInfo {
    constructor(id, name, func) {
        this._id = id;
        this._name = name;
        this._func = func;
        return this;
    }

    get id() {
        return this._id;
    }

    get name() {
        return this._name;
    }

    get func() {
        return this._func;
    }

    canonicalize() {
        return {
            id: this._id,
            name: this._name,
        };
    }
}

function findFunctions() {
    let funcs = new Array();

    let queue = new Array();
    let visited = new Set();
    queue.push(['', this]);
    visited.add(this);

    const excludedProperties = ['caller', 'callee', 'arguments'];
    const cafNames = ['caf', 'CAFFuzzer'];

    let id = 0;
    while (queue.length > 0) {
        const [path, obj] = queue.shift();
        if (typeof(obj) === 'undefined' || obj === null) {
            continue;
        }

        if (typeof(obj) === 'function') {
            funcs.push(new FunctionInfo(id, path, obj));
            ++id;
        }

        for (const name of Object.getOwnPropertyNames(obj)) {
            if (excludedProperties.includes(name)) {
                continue;
            }

            if (cafNames.includes(name)) {
                continue;
            }

            let children;
            try {
                children = obj[name];
            } catch (e) {
                continue;
            }

            if (typeof(children) === 'undefined' || children === null) {
                continue;
            }

            if (visited.has(children)) {
                continue;
            }

            let childrenPath = path;
            if (childrenPath.length > 0) {
                childrenPath += '.';
            }
            childrenPath += name;

            queue.push([childrenPath, children]);
            visited.add(children);
        }
    }

    return funcs;
}

global = this;

if (require) {
    // We're fuzzing nodejs.
    // Import all native modules and mount their module objects onto the global object.
    const moduleList = [
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
    ];
    const modules = moduleList.map(name => [name, require(name)]);
    for (const [name, moduleObj] of modules) {
        Object.defineProperty(this, name, {
            enumerable: true,
            value: moduleObj,
            writable: false,
        });
    }
}

const funcs = findFunctions.apply(global);

// Dump the JSON database.
console.log(JSON.stringify(funcs.map(f => f.name)));
