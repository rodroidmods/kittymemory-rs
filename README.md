# kittymemory-rs

Advanced Rust bindings for [KittyMemory](https://github.com/MJx0/KittyMemory) — a memory manipulation library for Android and iOS.

## Highlights

- Automatic FFI binding generation with `bindgen`
- Safe Rust wrappers for common operations (`safe` module)
- Low-level FFI surface in `sys` for direct calls
- Memory read/write, patching, backup, pattern scanning, symbol lookup and pointer validation

## Quick Architecture

- `sys` — raw, bindgen-generated FFI bindings (unsafe)
- `safe` — ergonomic wrappers built on top of `sys` (safe where possible)

## Installation

Add to your `Cargo.toml` (local or Git):

```toml
[dependencies]
kittymemory = { path = "path/to/kittymemory-rs" }
```

Or use the Git source:

```toml
[dependencies]
kittymemory = { git = "https://github.com/rodroidmods/kittymemory-rs", branch = "main" }
```

## Requirements

- Rust 1.70+
- C++ compiler (g++/clang++)
- `libclang` (for bindgen)
- Android NDK (for Android target builds)
- Xcode toolchain (for iOS target builds)

## Basic Usage

Use the provided `safe` wrappers for most tasks; use `sys` for raw FFI when you need full control.

Read a typed value:

```rust
use kittymemory::prelude::*;

let addr: Address = 0x1234_5678;
let value: i32 = mem_read(addr).expect("read failed");
println!("value = {}", value);
```

Write memory (Android):

```rust
use kittymemory::prelude::*;

let v: i32 = 42;
mem_write(addr, &v).expect("write failed");
```

Apply a patch:

```rust
use kittymemory::prelude::*;

let mut patch = Patch::with_hex(addr, "90 90 90 90").expect("bad hex");
if patch.is_valid() {
    patch.modify().expect("apply failed");
    // ...later
    patch.restore().expect("restore failed");
}
```

Pattern scan:

```rust
use kittymemory::prelude::*;

if let Some(p) = find_pattern_first(0x1000_0000, 0x2000_0000, "48 8B ? ? 48 89") {
    println!("found at {:#x}", p);
}
```

Pointer validation:

```rust
use kittymemory::prelude::*;

let mut v = PtrValidator::new();
v.set_use_cache(true);
println!("readable: {}", v.is_ptr_readable(0x1000, 4));
```

If you prefer raw FFI, import `sys` directly:

```rust
use kittymemory::sys;
unsafe {
    let mut x: i32 = 0;
    sys::km_mem_read(0x1000 as *const _, &mut x as *mut _ as *mut _, 4);
}
```

## Building

### Local development (host)

```bash
cargo build
```

### Android (recommended via `cargo-ndk`)

Install `cargo-ndk` and build for `arm64`:

```bash
cargo install cargo-ndk
cargo ndk -t arm64-v8a build --release --features keystone
```

Alternative (manual NDK target):

```bash
rustup target add aarch64-linux-android
cargo build --target aarch64-linux-android --features android --release
```

### iOS (cross-build)

```bash
rustup target add aarch64-apple-ios
cargo build --target aarch64-apple-ios --features ios --release
```

Notes:
- `build.rs` compiles the C++ sources and runs `bindgen` during the build. The builder host needs `libclang` and a working C++ toolchain.
- The `keystone` feature requires Keystone static libraries for the target platform (see `KittyMemory/Deps/Keystone`).

## Examples

Run the low-level FFI example (requires successful native build):

```bash
cargo run --example usage
```

## Documentation

```bash
cargo doc --open
```

## Safety

- `safe` provides ergonomic, safer wrappers; prefer it when possible.
- `sys` is raw FFI and `unsafe` — you must manage ownership and free C allocations where required (use `km_patch_free`, `km_backup_free`, etc.).

## Platform Notes

- Android: ELF parsing, process map enumeration, memory protection APIs.
- iOS: Mach-O symbol lookup and detailed memory status codes.

## How It Works

1. `wrapper.cpp` / `wrapper.h` expose C++ APIs as C functions.
2. `build.rs` compiles the native sources and runs `bindgen` to generate `bindings.rs`.
3. `src/safe.rs` adds Rust-friendly wrappers on top of `sys`.

## License

MIT

## Credits

- **Original Library**: [KittyMemory](https://github.com/MJx0/KittyMemory) by MJx0
- **Rust Bindings / Maintenance**: Rodroid Dev
- **Telegram Group**: https://t.me/+QylrYL1GNsJiYjc0
- **Telegram Channel**: https://t.me/+WmudnO0-xoNhMDQ8

## Contributing

Contributions welcome — open issues or submit pull requests.

## Disclaimer

Intended for education, research and legitimate modding only. Comply with laws and platform terms.
