{
    "targets": [
        {
            "target_name": "caf",
            "sources": [
                "NodeTarget.cpp"
            ],
            "include_dirs": [
                "../../../include"
            ],
            "defines": [
                "NODE_WANT_INTERNALS=1"
            ],
            "libraries": [
                "-L<!(pwd)/../../../build/lib",
                "-lCAFV8Target"
            ]
        }
    ]
}