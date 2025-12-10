# kittymemory-rs

Production-ready Rust bindings for [KittyMemory](https://github.com/MJx0/KittyMemory) — a comprehensive memory manipulation library for Android and iOS.

> **⚠️ Important**: Use version **0.2.5 or higher**. Earlier versions (0.2.0-0.2.4) contain compilation issues for Android targets. Version 0.2.5+ is the official stable release.
>
> **Note**: The `keystone` feature for assembly patching is currently experimental and may have build issues. Use hex/bytes patching for production.

## Features

### Core Functionality
- **Memory Operations**: Read, write, and protect memory with automatic permission handling
- **Memory Patching**: Create patches from bytes, hex strings, or assembly code (with Keystone)
- **Memory Backup**: Save and restore memory regions
- **Pattern Scanning**: Find byte patterns, hex patterns, IDA-style patterns, or arbitrary data
- **Pointer Validation**: Verify if pointers are readable, writable, or executable

### Android-Specific Features
- **ELF Scanner**: Comprehensive ELF analysis with symbol lookup, debug symbols, and metadata
- **LinkerScanner**: Access Android linker internals and enumerate all loaded libraries
- **Process Maps**: Parse and filter /proc/self/maps entries
- **RegisterNativeFn**: Find JNI native method registrations by name and signature
- **SoInfo Access**: Get detailed information about loaded shared objects

### iOS-Specific Features
- **MemoryFileInfo**: Access Mach-O binary information for dylibs and frameworks
- **Segment/Section Access**: Query __TEXT, __DATA and other segments/sections
- **Symbol Lookup**: Find symbols in specific files or libraries
- **Address Translation**: Convert relative offsets to absolute addresses

### Utility Functions
- **Hex Conversion**: Convert between bytes and hex strings
- **Hex Dump**: Format memory dumps with ASCII representation
- **Page Helpers**: Calculate page-aligned addresses

## Architecture

- **`sys`**: Raw FFI bindings (auto-generated with bindgen)
- **`safe`**: Safe Rust wrappers with RAII and error handling
- **`prelude`**: Convenient imports for common use cases

## Installation

Add to your `Cargo.toml`:

```toml
[dependencies]
kittymemory-rs = "0.2"  # Recommended: auto-updates to latest stable 0.2.x
```

With Keystone assembler support:

```toml
[dependencies]
kittymemory-rs = { version = "0.2", features = ["keystone"] }
```

Or from GitHub:

```toml
[dependencies]
kittymemory = { git = "https://github.com/rodroidmods/kittymemory-rs", branch = "main" }
```

## Requirements

- Rust 1.70+
- C++ compiler (g++/clang++)
- `libclang` (for bindgen)
- Android NDK (for Android targets)
- Xcode (for iOS targets)

## Quick Examples

### Memory Operations

```rust
use kittymemory_rs::prelude::*;

let addr = 0x12345678;
let value: i32 = mem_read(addr)?;
mem_write(addr, &42i32)?;
```

### Memory Patching

```rust
use kittymemory_rs::prelude::*;

let mut patch = Patch::with_hex(0x1000, "90 90 90 90")?;
patch.modify()?;
patch.restore()?;
```

With assembly (requires `keystone` feature):

```rust
let mut patch = Patch::with_asm(
    0x1000,
    AsmArch::ARM64,
    "mov x0, #42\nret",
    0x1000
)?;
patch.modify()?;
```

### Pattern Scanning

```rust
use kittymemory_rs::prelude::*;

if let Some(addr) = find_pattern_first(0x10000000, 0x20000000, "48 8B ? ? 48 89") {
    println!("Found at {:#x}", addr);
}

let all_matches = find_hex_all(0x10000000, 0x20000000, "DEADBEEF", "xxxxxxxx");
for addr in all_matches {
    println!("Match at {:#x}", addr);
}
```

### Android: ELF Scanner

```rust
#[cfg(target_os = "android")]
use kittymemory::prelude::*;

let elf = ElfScanner::find("libil2cpp.so").expect("Library not found");
if let Some(addr) = elf.find_symbol("il2cpp_init") {
    println!("il2cpp_init at {:#x}", addr);
}

println!("Base: {:#x}", elf.base());
println!("Size: {:#x}", elf.load_size());
println!("Native: {}", elf.is_native());
```

### Android: LinkerScanner

```rust
#[cfg(target_os = "android")]
use kittymemory::prelude::*;

let linker = LinkerScanner::get();
for lib in linker.all_soinfo() {
    println!("{}: base={:#x} size={:#x}", lib.path, lib.base, lib.size);
}

if let Some(info) = linker.find_soinfo("libc.so") {
    println!("libc base: {:#x}", info.base);
}
```

### Android: Process Maps

```rust
#[cfg(target_os = "android")]
use kittymemory::prelude::*;

let maps = get_all_maps();
for map in maps {
    println!("{:#x}-{:#x} {} {}",
        map.start_address, map.end_address, map.protection, map.pathname);
}

let lib_maps = get_maps_filtered("libunity.so", ProcMapFilter::Contains);
```

### iOS: MemoryFileInfo

```rust
#[cfg(target_os = "ios")]
use kittymemory::prelude::*;

let base = MemoryFileInfo::get_base_info();
println!("Base executable: {}", base.name());

if let Some(lib) = MemoryFileInfo::get_file_info("libSystem.dylib") {
    let text = lib.get_segment("__TEXT");
    println!("__TEXT: {:#x}-{:#x}", text.start, text.end);

    if let Some(addr) = lib.find_symbol("_malloc") {
        println!("malloc at {:#x}", addr);
    }
}
```

### Utility Functions

```rust
use kittymemory_rs::prelude::*;

let data = vec![0xDE, 0xAD, 0xBE, 0xEF];
let hex = data_to_hex(&data);
println!("Hex: {}", hex);

let bytes = hex_to_data("DEADBEEF")?;

let dump = hex_dump(0x1000, 64);
println!("{}", dump);
```

## Building

### Desktop (Development)

```bash
cargo build
```

### Android

```bash
cargo install cargo-ndk
cargo ndk -t arm64-v8a build --release
```

### iOS

```bash
rustup target add aarch64-apple-ios
cargo build --target aarch64-apple-ios --release
```

## Examples

Run the comprehensive example:

```bash
cargo run --example usage
```

## Documentation

Generate and open the documentation:

```bash
cargo doc --open
```

## Feature Flags

- **`keystone`**: Enable assembly patching with Keystone assembler
- **`android`**: Android-specific features (auto-detected)
- **`ios`**: iOS-specific features (auto-detected)

## Safety

- **`safe` module**: RAII wrappers with automatic cleanup and error handling
- **`sys` module**: Raw FFI - requires manual memory management and `unsafe` blocks

## Platform Support Matrix

| Feature | Android | iOS | Cross-Platform |
|---------|---------|-----|----------------|
| Memory R/W | ✅ | ✅ | ✅ |
| Patching | ✅ | ✅ | ✅ |
| Pattern Scanning | ✅ | ✅ | ✅ |
| ELF Analysis | ✅ | N/A | - |
| Mach-O Analysis | N/A | ✅ | - |
| LinkerScanner | ✅ | N/A | - |
| Process Maps | ✅ | N/A | - |
| JNI Support | ✅ | N/A | - |
| Pointer Validation | ✅ | ✅ | ✅ |

## License

MIT

## Credits

- **Original Library**: [KittyMemory](https://github.com/MJx0/KittyMemory) by MJx0
- **Rust Bindings**: Rodroid Dev
- **Community**:
  - Telegram Group: https://t.me/+QylrYL1GNsJiYjc0
  - Telegram Channel: https://t.me/+WmudnO0-xoNhMDQ8

## Contributing

Contributions welcome! Open issues or submit pull requests.

## Disclaimer

Intended for education, research, and legitimate reverse engineering only. Users are responsible for compliance with applicable laws and platform terms.
