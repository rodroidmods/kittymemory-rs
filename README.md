# kittymemory-rs

Production-ready Rust bindings for [KittyMemory](https://github.com/MJx0/KittyMemory) — a comprehensive memory manipulation library for Android and iOS.

> **⚠️ Important**: Use version **0.3.0 or higher**. Version 0.3.0 includes complete feature parity with the original KittyMemory library with 100+ new functions and advanced capabilities.
>
> **Note**: The `keystone` feature for assembly patching is currently experimental and may have build issues. Use hex/bytes patching for production. Windows currentely is not supported, you might get errors if you try to build it in a windows pc, its recomanded to use WSL2 to compile it for android.

## Features

### Core Functionality
- **Memory Operations**: Read, write, and protect memory with automatic permission handling
- **Syscall Memory Operations**: Alternative memory read/write using syscalls (Android)
- **Memory Dumping**: Dump memory regions or memory-mapped files to disk
- **Memory Patching**: Create patches from bytes, hex strings, or assembly code (with Keystone)
- **Patch Inspection**: Get current, original, and patch bytes as hex strings
- **Memory Backup**: Save and restore memory regions with byte inspection
- **Pattern Scanning**: Find byte patterns, hex patterns, IDA-style patterns, or arbitrary data
- **Pointer Validation**: Verify if pointers are readable, writable, or executable

### Android-Specific Features
- **ELF Scanner**: Comprehensive ELF analysis with symbol lookup, debug symbols, and metadata
- **Advanced ELF Introspection**: Access program headers, dynamic section, hash tables, string/symbol tables
- **LinkerScanner**: Access Android linker internals and enumerate all loaded libraries
- **LinkerScanner Extensions**: Get somain and sonext library information
- **NativeBridgeScanner**: Full support for x86/x86_64 emulation on ARM (Houdini detection)
- **NativeBridgeLinker**: dlopen, dlsym, dlerror, dladdr operations for native bridge
- **Process Maps**: Parse and filter /proc/self/maps entries with helper methods
- **ProcMap Helpers**: Validate maps, check for ELF headers, test address containment
- **RegisterNativeFn**: Find JNI native method registrations by name and signature
- **SoInfo Access**: Get detailed information about loaded shared objects
- **Android System Info**: Get Android version, SDK level, and external storage path

### iOS-Specific Features
- **MemoryFileInfo**: Access Mach-O binary information for dylibs and frameworks
- **Segment/Section Access**: Query __TEXT, __DATA and other segments/sections
- **Symbol Lookup**: Find symbols in specific files or libraries
- **Address Translation**: Convert relative offsets to absolute addresses

### Utility Functions
- **Hex Conversion**: Convert between bytes and hex strings with validation
- **Hex Dump**: Format memory dumps with ASCII representation
- **Page Helpers**: Calculate page-aligned addresses
- **File Operations**: Complete file I/O abstraction (read, write, copy, delete, stat)
- **String Utilities**: String manipulation (trim, validation, random generation)
- **File Path Utilities**: Extract filename, directory, and extension from paths
- **ZIP Utilities**: List ZIP files, extract by offset, memory-map ZIP contents (Android)

## Architecture

- **`sys`**: Raw FFI bindings (auto-generated with bindgen)
- **`safe`**: Safe Rust wrappers with RAII and error handling
- **`prelude`**: Convenient imports for common use cases

## Installation

Add to your `Cargo.toml`:

```toml
[dependencies]
kittymemory-rs = "0.3"  # Recommended: auto-updates to latest stable 0.3.x
```

With Keystone assembler support:

```toml
[dependencies]
kittymemory-rs = { version = "0.3", features = ["keystone"] }
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

// Basic patching with absolute address
let mut patch = Patch::with_hex(0x1000, "90 90 90 90")?;
patch.modify()?;
patch.restore()?;
```

#### Library-Based Patching (Android)

Create patches using library name + offset - just like the C++ API:

```rust
#[cfg(target_os = "android")]
use kittymemory::prelude::*;

// Helper to convert hex string to offset
fn string2offset(hex_str: &str) -> usize {
    let clean = hex_str.trim().trim_start_matches("0x").trim_start_matches("0X");
    usize::from_str_radix(clean, 16).unwrap_or(0)
}

// Create patch using library name + offset (like C++ MemoryPatch::createWithHex)
let mut money_patch = Patch::with_hex_lib(
    "libil2cpp.so",
    string2offset("0xD6D93C"),
    "62 01 0C 00 1E FF 2F E1"
)?;
money_patch.modify()?;

// Or with raw bytes
let mut bytes_patch = Patch::with_bytes_lib(
    "libil2cpp.so",
    0xD6D93C,
    &[0x62, 0x01, 0x0C, 0x00, 0x1E, 0xFF, 0x2F, 0xE1]
)?;
bytes_patch.modify()?;
```

#### Assembly Patching (requires `keystone` feature)

```rust
// With absolute address
let mut patch = Patch::with_asm(
    0x1000,
    AsmArch::ARM64,
    "mov x0, #42\nret",
    0x1000
)?;
patch.modify()?;

// With library name + offset (Android only)
#[cfg(target_os = "android")]
let mut asm_patch = Patch::with_asm_lib(
    "libil2cpp.so",
    0xD6D93C,
    AsmArch::ARM32,
    "mov r0, #1; bx lr"
)?;
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

Run the basic example:

```bash
cargo run --example usage
```

Run the advanced features example (showcasing new v0.3.0 features):

```bash
cargo run --example advanced_features
```

The advanced example demonstrates:
- Syscall-based memory operations (Android)
- Memory dumping to disk
- Patch and backup byte inspection
- Advanced ELF scanner capabilities
- ProcMap helper methods
- String utilities
- File path utilities
- Android system information
- Advanced linker scanner features
- NativeBridge scanner (x86 emulation support)

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

## Changelog

### Version 0.3.0 (Latest)
- **Complete Feature Parity**: Added 100+ new functions matching original KittyMemory library
- **Library-Based Patching**: Create patches using library name + offset (Android)
  - `Patch::with_hex_lib()` - Create hex patch with library name + offset
  - `Patch::with_bytes_lib()` - Create bytes patch with library name + offset
  - `Patch::with_asm_lib()` - Create assembly patch with library name + offset (requires keystone)
- **Syscall Memory Operations**: Alternative memory read/write using syscalls (Android)
- **Memory Dumping**: Dump memory regions or files to disk
- **Patch/Backup Inspection**: Get current, original, and patch bytes as hex strings
- **Advanced ELF Scanner**: Access program headers, dynamic section, hash tables, string/symbol tables
- **ELF Refresh**: Refresh ELF scanner data
- **LinkerScanner Extensions**: Get somain and sonext library information
- **NativeBridgeScanner**: Full support for x86/x86_64 emulation on ARM (Houdini detection)
- **NativeBridgeLinker**: dlopen, dlsym, dlerror, dladdr operations
- **ProcMap Helpers**: Validation, ELF detection, address containment checks
- **Android System Info**: Get Android version, SDK level, external storage
- **String Utilities**: Trim, validation, random generation
- **File Path Utilities**: Extract filename, directory, extension
- **File I/O Abstraction**: Complete file operations wrapper
- **ZIP Utilities**: List, extract, and memory-map ZIP contents (Android)
- **Documentation**: Improved rustdoc support for crates.io publishing
- **Examples**: New advanced_features example showcasing all new capabilities

### Version 0.2.6
- Previous stable release

## Disclaimer

Intended for education, research, and legitimate reverse engineering only. Users are responsible for compliance with applicable laws and platform terms.
