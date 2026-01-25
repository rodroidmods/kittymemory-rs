use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    // For docs.rs builds, use pre-generated bindings since network access is blocked
    if env::var("DOCS_RS").is_ok() {
        println!("cargo:warning=Using pre-generated bindings.rs for docs.rs");
        println!("cargo:rerun-if-changed=bindings.rs");
        // Copy the pre-generated bindings.rs to OUT_DIR
        let src = PathBuf::from("bindings.rs");
        let dst = out_path.join("bindings.rs");
        fs::copy(&src, &dst).expect("Failed to copy pre-generated bindings.rs");
        println!("cargo:warning=Copied bindings.rs to {}", dst.display());
        return;
    }

    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();

    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=wrapper.cpp");
    println!("cargo:rerun-if-env-changed=KITTYMEMORY_PATH");
    println!("cargo:rerun-if-env-changed=CARGO_FEATURE_KEYSTONE");

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    let kittymemory_dir = if let Ok(p) = env::var("KITTYMEMORY_PATH") {
        PathBuf::from(p)
    } else if PathBuf::from("KittyMemory").exists() {
        PathBuf::from("KittyMemory")
    } else {
        let dest = out_dir.join("KittyMemory");
        if !dest.exists() {
            println!("cargo:warning=Cloning KittyMemory into {}", dest.display());
            // Clone without --depth 1 to ensure submodules work properly
            let status = Command::new("git")
                .args(&[
                    "clone",
                    "--recursive",
                    "https://github.com/MJx0/KittyMemory",
                    dest.to_str().unwrap(),
                ])
                .status()
                .expect("Failed to spawn git - ensure `git` is installed and in PATH");
            if !status.success() {
                panic!("git clone failed with status: {}", status);
            }
            println!("cargo:warning=Cloned KittyMemory with submodules");
        } else {
            // If dest exists, ensure submodules are initialized
            println!(
                "cargo:warning=KittyMemory already exists, ensuring submodules are initialized"
            );
            let sm_status = Command::new("git")
                .current_dir(&dest)
                .args(&["submodule", "update", "--init", "--recursive"])
                .status();

            match sm_status {
                Ok(s) if s.success() => {
                    println!("cargo:warning=Initialized git submodules for KittyMemory");
                }
                Ok(s) => {
                    println!(
                        "cargo:warning=git submodule update exited with {} — continuing",
                        s
                    );
                }
                Err(e) => {
                    println!(
                        "cargo:warning=Failed to run git submodule update: {} — continuing",
                        e
                    );
                }
            }
        }
        dest
    };

    let mut src_root = kittymemory_dir.clone();
    let sample_source = src_root.join("KittyUtils.cpp");
    if !sample_source.exists() {
        let nested = kittymemory_dir.join("KittyMemory");
        if nested.exists() {
            src_root = nested;
        }
    }

    let mut build = cc::Build::new();

    build
        .cpp(true)
        .flag_if_supported("-std=c++17")
        .include(&kittymemory_dir)
        .include(&src_root)
        .include(kittymemory_dir.join("Deps/Keystone/includes"))
        .flag("-include")
        .flag("kittymemory_fix.hpp")
        .file("wrapper.cpp");

    if cfg!(feature = "keystone") {
        let target_arch = env::var("CARGO_CFG_TARGET_ARCH")
            .expect("CARGO_CFG_TARGET_ARCH not set — are you cross-compiling properly?");

        let candidates = vec![
            out_dir.join("KittyMemory/KittyMemory/Deps/Keystone"),
            kittymemory_dir.join("Deps/Keystone"),
            kittymemory_dir.join("Deps/keystone"),
            kittymemory_dir.join("KittyMemory/Deps/Keystone"),
            kittymemory_dir.join("KittyMemory/Deps/keystone"),
            src_root.join("Deps/Keystone"),
            src_root.join("Deps/keystone"),
            src_root.join("KittyMemory/Deps/Keystone"),
            src_root.join("KittyMemory/Deps/keystone"),
            out_dir.join("KittyMemory/Deps/Keystone"),
            out_dir.join("KittyMemory/Deps/keystone"),
            out_dir.join("KittyMemory/KittyMemory/Deps/Keystone"),
            out_dir.join("KittyMemory/KittyMemory/Deps/keystone"),
        ];

        fn find_lib_dir(cands: &[PathBuf], target_os: &str, target_arch: &str) -> Option<PathBuf> {
            // First pass: check standard paths
            for base in cands {
                let libdir = match (target_os, target_arch) {
                    ("android", "aarch64") => base.join("libs-android/arm64-v8a"),
                    ("android", "arm") => base.join("libs-android/armeabi-v7a"),
                    ("android", "x86") => base.join("libs-android/x86"),
                    ("android", "x86_64") => base.join("libs-android/x86_64"),
                    ("ios", "aarch64") => base.join("libs-ios/arm64"),
                    _ => base.join("libs"),
                };

                println!(
                    "cargo:warning=Checking libdir: {} (exists={})",
                    libdir.display(),
                    libdir.exists()
                );
                if libdir.exists() {
                    return Some(libdir);
                }

                let alt = match (target_os, target_arch) {
                    ("android", "aarch64") => base.join("libs/Android/arm64-v8a"),
                    ("android", "arm") => base.join("libs/Android/armeabi-v7a"),
                    ("android", "x86") => base.join("libs/Android/x86"),
                    ("android", "x86_64") => base.join("libs/Android/x86_64"),
                    ("ios", "aarch64") => base.join("libs-ios/arm64e"),
                    _ => base.join("libs"),
                };
                println!(
                    "cargo:warning=Checking alt libdir: {} (exists={})",
                    alt.display(),
                    alt.exists()
                );
                if alt.exists() {
                    return Some(alt);
                }
            }

            // Second pass: recursive search for libkeystone.a
            for base in cands {
                if !base.exists() {
                    continue;
                }

                println!("cargo:warning=Searching recursively in: {}", base.display());

                fn search_dir(dir: &PathBuf, depth: usize) -> Option<PathBuf> {
                    if depth == 0 {
                        return None;
                    }
                    let entries = match fs::read_dir(dir) {
                        Ok(e) => e,
                        Err(_) => return None,
                    };
                    for entry in entries.flatten() {
                        let p = entry.path();
                        if p.is_dir() {
                            if let Some(found) = search_dir(&p, depth - 1) {
                                return Some(found);
                            }
                        } else if p.is_file() {
                            if let Some(name) = p.file_name().and_then(|n| n.to_str()) {
                                let lname = name.to_lowercase();
                                if lname == "libkeystone.a"
                                    || (lname.contains("keystone") && lname.ends_with(".a"))
                                {
                                    println!(
                                        "cargo:warning=Found keystone lib at: {}",
                                        p.display()
                                    );
                                    return p.parent().map(|pp| pp.to_path_buf());
                                }
                            }
                        }
                    }
                    None
                }

                if let Some(found) = search_dir(base, 6) {
                    println!("cargo:warning=Found keystone lib dir: {}", found.display());
                    return Some(found);
                }
            }

            None
        }

        for c in &candidates {
            println!(
                "cargo:warning=Keystone candidate path: {} (exists={})",
                c.display(),
                c.exists()
            );
            if c.exists() {
                // List contents of the candidate directory
                if let Ok(entries) = fs::read_dir(c) {
                    for entry in entries.flatten() {
                        println!("cargo:warning=  -> {}", entry.path().display());
                    }
                }
            }
        }

        if let Some(keystone_lib_dir) =
            find_lib_dir(&candidates, target_os.as_str(), target_arch.as_str())
        {
            println!(
                "cargo:rustc-link-search=native={}",
                keystone_lib_dir.display()
            );
            println!("cargo:rustc-link-lib=static=keystone");
        } else {
            println!("cargo:warning=Keystone libs not found in expected Deps paths; disabling keystone feature");
            build.define("kNO_KEYSTONE", None);
        }
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
        let source_path = src_root.join(source);
        build.file(&source_path);
    }

    if target_os == "android" {
        build.define("__ANDROID__", None);
        println!("cargo:rustc-link-lib=log");
    } else if target_os == "ios" {
        build.define("__APPLE__", None);
        println!("cargo:rustc-link-lib=framework=Foundation");
    }

    build.flag_if_supported("-Wno-return-type");

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

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
