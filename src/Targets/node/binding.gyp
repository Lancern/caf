{
  "targets": [
    {
      "target_name": "CAFNodeTarget",
      "type": "static_library",
      "sources": [ "src/NodeTarget.cpp" ],
      #  "cflags": ["-emit-llvm", "-S"],
      #  "cflags_cc": ["-emit-llvm", "-S"],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      'defines': [
        'NODE_WANT_INTERNALS=1',
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
        'NODE_TAG="<(node_tag)"',
      ],
    }
  ]
}
