# WebUDP-rs

A simple Rust wrapper around [WebUDP](https://github.com/seemk/WebUDP)

```
sudo apt install cmake llvm-dev libclang-dev clang libssl-dev
git submodule update --init
cargo run --example server &
cd examples
python3 -m http.server 8080 &
xdg-open http://localhost:8080/
```
