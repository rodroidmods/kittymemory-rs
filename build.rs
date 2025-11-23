use std::env;
use std::path::PathBuf;

fn main() {
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    
    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=wrapper.cpp");
    println!("cargo:rerun-if-changed=KittyMemory/KittyMemory");

    let kittymemory_dir = PathBuf::from("KittyMemory");

    let mut build = cc::Build::new();
    
    build
        .cpp(true)
        .flag_if_supported("-std=c++17")
        .include(".")
        .include(&kittymemory_dir)
        .include("KittyMemory/Deps/Keystone/includes")
        .flag("-include")
        .flag("kittymemory_fix.hpp")
        .file("wrapper.cpp");

        if cfg!(feature = "keystone") {
        let target_arch = env::var("CARGO_CFG_TARGET_ARCH")
            .expect("CARGO_CFG_TARGET_ARCH not set — are you cross-compiling properly?");

        let keystone_lib_dir = match (target_os.as_str(), target_arch.as_str()) {
            ("android", "aarch64") => "KittyMemory/Deps/Keystone/libs-android/arm64-v8a",
            ("android", "arm")     => "KittyMemory/Deps/Keystone/libs-android/armeabi-v7a",
            ("android", "x86")     => "KittyMemory/Deps/Keystone/libs-android/x86",
            ("android", "x86_64")  => "KittyMemory/Deps/Keystone/libs-android/x86_64",
            ("ios", _)             => "KittyMemory/Deps/Keystone/libs-ios",
            _ => panic!("Unsupported platform/arch combo for keystone: {} {}", target_os, target_arch),
        };

        println!("cargo:rustc-link-search=native={}", keystone_lib_dir);
        println!("cargo:rustc-link-lib=static=keystone");
    } else {
        build.define("kNO_KEYSTONE", None);
    }

    let sources = vec![
        "KittyUtils.cpp",
        "KittyMemory.cpp",
        "MemoryPatch.cpp",
        "MemoryBackup.cpp",
        "KittyScanner.cpp",
        "KittyAsm.cpp",
        "KittyPtrValidator.cpp",
        "KittyIOFile.cpp",
    ];

    for source in sources {
        let source_path = kittymemory_dir.join(source);
        build.file(&source_path);
    }

    if target_os == "android" {
        build.define("__ANDROID__", None);
        println!("cargo:rustc-link-lib=log");
    } else if target_os == "ios" {
        build.define("__APPLE__", None);
        println!("cargo:rustc-link-lib=framework=Foundation");
    }

    if cfg!(feature = "keystone") {
        let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();
        let keystone_lib_dir = match (target_os.as_str(), target_arch.as_str()) {
            ("android", "aarch64") => "KittyMemory/Deps/Keystone/libs/Android/arm64-v8a",
            ("android", "arm") => "KittyMemory/Deps/Keystone/libs/Android/armeabi-v7a",
            ("android", "x86") => "KittyMemory/Deps/Keystone/libs/Android/x86",
            ("android", "x86_64") => "KittyMemory/Deps/Keystone/libs/Android/x86_64",
            ("ios", _) => "KittyMemory/Deps/Keystone/libs/iOS",
            _ => panic!("Unsupported platform for keystone"),
        };
        
        println!("cargo:rustc-link-search=native={}", keystone_lib_dir);
        println!("cargo:rustc-link-lib=static=keystone");
    } else {
        build.define("kNO_KEYSTONE", None);
    }

    build.compile("kittymemory");

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .allowlist_function("km_.*")
        .allowlist_type("km_.*")
        .allowlist_var("KM_.*")
        .derive_debug(true)
        .derive_default(true)
        .derive_copy(true)
        .derive_eq(true)
        .derive_hash(true)
        .derive_ord(true)
        .derive_partialeq(true)
        .derive_partialord(true)
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
