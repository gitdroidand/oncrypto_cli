use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-env-changed=ONCRYPTO_LIB_PATH");

    let manifest_dir =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let library = match env::var("ONCRYPTO_LIB_PATH") {
        Ok(path) => PathBuf::from(path),

        // binding/rs/oncrypto-rs -> project root -> build-linux
        Err(_) => manifest_dir
            .join("../../../build-linux/liboncrypto.so"),
    };

    let library = library
        .canonicalize()
        .expect("liboncrypto.so was not found");

    let directory = library
        .parent()
        .expect("liboncrypto.so has no parent directory");

    println!(
        "cargo:rustc-link-search=native={}",
        directory.display()
    );

    println!("cargo:rustc-link-lib=dylib=oncrypto");

    println!(
        "cargo:rustc-link-arg=-Wl,-rpath,{}",
        directory.display()
    );
}