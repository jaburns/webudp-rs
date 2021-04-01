extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    let mut lib_path = PathBuf::from(file!());
    lib_path.pop();
    lib_path.push("WebUDP");
    lib_path.push("build");
    println!("cargo:rustc-link-search={}", lib_path.display());
    println!("cargo:rustc-link-lib=WuHost");
    println!("cargo:rustc-link-lib=Wu");
    println!("cargo:rustc-link-lib=ssl");
    println!("cargo:rustc-link-lib=crypto");
    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=build.rs");

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
