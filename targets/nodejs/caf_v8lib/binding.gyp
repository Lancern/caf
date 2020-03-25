{
  "targets": [
 {
 "target_name": "caf_v8lib",
 "type": "static_library",
 "sources": [ "src/caf_v8lib.cpp" ],
#  "cflags": ["-emit-llvm", "-S"],
#  "cflags_cc": ["-emit-llvm", "-S"],
 "include_dirs": [
 "<!(node -e \"require('nan')\")"
 ]
 }
 ]
}