use bindgen;
use cmake;

use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=wrapper.h");

    cmake::build("WebUDP");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let lib_path = out_path.join("build");

    println!("cargo:rustc-link-search={}", lib_path.display());
    println!("cargo:rustc-link-lib=WuHost");
    println!("cargo:rustc-link-lib=Wu");
    println!("cargo:rustc-link-lib=ssl");
    println!("cargo:rustc-link-lib=crypto");

    bindgen::Builder::default()
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
