{
    "targets": [
        {
            "target_name": "caf",
            "sources": [
                "NodeTarget.cpp"
            ],
            "include_dirs": [
                "../../../include",
                "../../../deps"
            ],
            "defines": [
                "NODE_WANT_INTERNALS=1",
                "CAF_ENABLE_AFL_DEFER=1"
            ],
            "libraries": [
                "-L<!(pwd)/../../../build/lib",
                "-lCAFV8Target",
                "-lCAFBasic"
            ]
        }
    ]
}
