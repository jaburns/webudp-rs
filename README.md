# WebUDP-rs

A simple Rust wrapper around [WebUDP](https://github.com/seemk/WebUDP).

### System build requirements

#### Linux i.e. Ubuntu
```
sudo apt install cmake llvm-dev libclang-dev clang libssl-dev
```

#### Windows
- [Install OpenSSL for Windows](https://wiki.openssl.org/index.php/Binaries)

### Running the example on Linux
````
python3 -m http.server 8080 &
xdg-open http://localhost:8080/examples
cargo run --example server
````

### Updating Windows binaries

On Linux everything is built from source, and `bindings.rs` is generated from the C headers. On Windows, WebUDP is included as
pre-compiled binaries, and the latest `bindings.rs` is used without generating a new one. This is why `bindings.rs` is included
in the repo at all, for easier Windows support. The Windows binaries can be rebuilt using Visual Studio's cmake though:
- Ensure VS's cmake.exe is in the system path
- Navigate to this repo in powershell and run the following:
```
cd WebUDP
mkdir build64
cd build64
cmake -G"Visual Studio 16 2019" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release
cmake --build .
cp .\Debug\*.lib ..\..\win64lib
```
