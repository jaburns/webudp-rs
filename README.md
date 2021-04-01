# WebUDP-rs

A simple Rust wrapper around [WebUDP](https://github.com/seemk/WebUDP)

```
./initialize
cargo run --example echo_server &
cd examples
python -m SimpleHTTPServer 8080 &
xdg-open http://localhost:8080/echo_server.html
```