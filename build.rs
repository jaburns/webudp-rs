use std::env;
use std::path::PathBuf;

#[cfg(not(windows))]
fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let lib_path = out_path.join("build");

    println!("cargo:rustc-link-search={}", lib_path.display());
    println!("cargo:rustc-link-lib=WuHost");
    println!("cargo:rustc-link-lib=Wu");
    println!("cargo:rustc-link-lib=ssl");
    println!("cargo:rustc-link-lib=crypto");

    cmake::build("WebUDP");

    bindgen::Builder::default()
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("bindings.rs")
        .expect("Couldn't write bindings!");
}

#[cfg(windows)]
fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let lib_path = out_path.join("build");

    println!("cargo:rustc-link-search=win64lib");
    println!("cargo:rustc-link-search=C;/Program Files/OpenSSL-Win64/lib");
    println!("cargo:rustc-link-lib=WuHost");
    println!("cargo:rustc-link-lib=Wu");
    println!("cargo:rustc-link-lib=libssl");
    println!("cargo:rustc-link-lib=libcrypto");
}
