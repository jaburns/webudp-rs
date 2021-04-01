# WebUDP-rs

: # A simple Rust wrapper around [WebUDP](https://github.com/seemk/WebUDP). On linux, run the commands below to run the demo, or just run the README file directly with `bash README.md`.

````
# sudo apt install cmake llvm-dev libclang-dev clang libssl-dev
cargo build
python3 -m http.server 8080 &
xdg-open http://localhost:8080/examples
cargo run --example server
````