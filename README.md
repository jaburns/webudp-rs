# WebUDP-rs

A simple Rust wrapper around [WebUDP](https://github.com/seemk/WebUDP)

```
./initialize
cargo run --example echo_server &
cd WebUDP/examples/client
python -m SimpleHTTPServer 8080 &
xdg-open http://localhost:8080/simple_echo_client.html
```