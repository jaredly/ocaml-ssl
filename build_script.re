open Bsb_internals;

let ( +/ ) = Filename.concat;

gcc(
    ~includes=[
        "/usr/local/opt/openssl/include"
    ],
    "lib" +/ "ssl_stubs.o",
    ["src" +/ "ssl_stubs.c"]
);