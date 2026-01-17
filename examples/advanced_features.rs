use kittymemory::prelude::*;

fn main() {
    println!("=== KittyMemory-RS Advanced Features Example ===\n");

    demo_syscall_operations();
    demo_memory_dumping();
    demo_patch_inspection();
    demo_backup_inspection();
    demo_advanced_elf_scanner();
    demo_procmap_helpers();
    demo_string_utilities();
    demo_file_utilities();

    #[cfg(target_os = "android")]
    {
        demo_android_advanced();
        demo_native_bridge();
    }

    println!("\n=== All advanced examples completed ===");
}

#[cfg(target_os = "android")]
fn demo_syscall_operations() {
    println!("--- Syscall Memory Operations ---");

    let test_addr = 0x1000usize;
    let mut buffer = vec![0u8; 16];

    match syscall_mem_read(test_addr, &mut buffer) {
        Ok(bytes_read) => println!("Syscall read {} bytes from {:#x}", bytes_read, test_addr),
        Err(e) => println!("Syscall read failed (expected): {}", e),
    }

    let data = vec![0xAA, 0xBB, 0xCC, 0xDD];
    match syscall_mem_write(test_addr, &data) {
        Ok(bytes_written) => println!("Syscall wrote {} bytes to {:#x}", bytes_written, test_addr),
        Err(e) => println!("Syscall write failed (expected): {}", e),
    }

    println!();
}

#[cfg(not(target_os = "android"))]
fn demo_syscall_operations() {
    println!("--- Syscall Memory Operations (Android only) ---\n");
}

#[cfg(target_os = "android")]
fn demo_memory_dumping() {
    println!("--- Memory Dumping ---");

    match dump_mem_to_disk(0x1000, 4096, "/tmp/memory_dump.bin") {
        Ok(()) => println!("Successfully dumped memory to /tmp/memory_dump.bin"),
        Err(e) => println!("Memory dump failed (expected): {}", e),
    }

    match dump_mem_file_to_disk("/proc/self/maps", "/tmp/maps_dump.txt") {
        Ok(()) => println!("Successfully dumped /proc/self/maps"),
        Err(e) => println!("File dump failed (expected): {}", e),
    }

    println!();
}

#[cfg(not(target_os = "android"))]
fn demo_memory_dumping() {
    println!("--- Memory Dumping (Android only) ---\n");
}

fn demo_patch_inspection() {
    println!("--- Patch Byte Inspection ---");

    let patch = Patch::with_bytes(0x1000, &[0x90, 0x90, 0x90, 0x90]);
    println!("Patch created: valid={}", patch.is_valid());

    if let Ok(curr_bytes) = patch.get_curr_bytes() {
        println!("Current bytes: {}", curr_bytes);
    }

    if let Ok(orig_bytes) = patch.get_orig_bytes() {
        println!("Original bytes: {}", orig_bytes);
    }

    if let Ok(patch_bytes) = patch.get_patch_bytes() {
        println!("Patch bytes: {}", patch_bytes);
    }

    println!();
}

fn demo_backup_inspection() {
    println!("--- Backup Byte Inspection ---");

    let backup = Backup::create(0x1000, 64);
    println!("Backup created: valid={}", backup.is_valid());

    if let Ok(curr_bytes) = backup.get_curr_bytes() {
        println!("Current bytes: {}", curr_bytes);
    }

    if let Ok(orig_bytes) = backup.get_orig_bytes() {
        println!("Original bytes: {}", orig_bytes);
    }

    println!();
}

#[cfg(target_os = "android")]
fn demo_advanced_elf_scanner() {
    println!("--- Advanced ELF Scanner ---");

    if let Some(elf) = ElfScanner::find("libc.so") {
        println!("libc.so found:");
        println!("  Base: {:#x}", elf.base());
        println!("  End: {:#x}", elf.end());
        println!("  Load Size: {:#x}", elf.load_size());
        println!("  Load Bias: {:#x}", elf.load_bias());
        println!("  Is Native: {}", elf.is_native());
        println!("  Is Emulated: {}", elf.is_emulated());
        println!("  Is Zipped: {}", elf.is_zipped());

        println!("\n  Advanced Metadata:");
        println!("    PHDR: {:#x}", elf.phdr());
        println!("    Loads: {}", elf.loads());
        println!("    Dynamic: {:#x}", elf.dynamic());
        println!("    String Table: {:#x}", elf.string_table());
        println!("    Symbol Table: {:#x}", elf.symbol_table());
        println!("    String Table Size: {}", elf.string_table_size());
        println!("    Symbol Entry Size: {}", elf.symbol_entry_size());
        println!("    ELF Hash Table: {:#x}", elf.elf_hash_table());
        println!("    GNU Hash Table: {:#x}", elf.gnu_hash_table());

        if let Some(path) = elf.file_path() {
            println!("    File Path: {}", path);
        }

        if let Some(real_path) = elf.real_path() {
            println!("    Real Path: {}", real_path);
        }

        println!("    Fixed by SoInfo: {}", elf.is_fixed_by_soinfo());
    }

    println!();
}

#[cfg(not(target_os = "android"))]
fn demo_advanced_elf_scanner() {
    println!("--- Advanced ELF Scanner (Android only) ---\n");
}

#[cfg(target_os = "android")]
fn demo_procmap_helpers() {
    println!("--- ProcMap Helper Methods ---");

    if let Some(map) = get_address_map(0x1000) {
        println!("Map at {:#x}:", 0x1000);
        println!("  Is Valid: {}", map.is_valid());
        println!("  Is Unknown: {}", map.is_unknown());
        println!("  Is Valid ELF: {}", map.is_valid_elf());
        println!("  Contains {:#x}: {}", 0x1500, map.contains(0x1500));
        println!("  String: {}", map.to_string());
    }

    println!();
}

#[cfg(not(target_os = "android"))]
fn demo_procmap_helpers() {
    println!("--- ProcMap Helper Methods (Android only) ---\n");
}

fn demo_string_utilities() {
    println!("--- String Utilities ---");

    let test_str = "hello_world";
    println!("String: {}", test_str);
    println!("  Starts with 'hello': {}", string_starts_with(test_str, "hello"));
    println!("  Contains 'world': {}", string_contains(test_str, "world"));
    println!("  Ends with 'world': {}", string_ends_with(test_str, "world"));

    if let Ok(trimmed) = string_trim("  spaces  ") {
        println!("  Trimmed: '{}'", trimmed);
    }

    println!("  Validate hex 'DEADBEEF': {}", string_validate_hex("DEADBEEF"));
    println!("  Validate hex 'GGGGGGGG': {}", string_validate_hex("GGGGGGGG"));

    if let Ok(random) = string_random(16) {
        println!("  Random string (16 chars): {}", random);
    }

    println!();
}

fn demo_file_utilities() {
    println!("--- File Path Utilities ---");

    let path = "/home/user/documents/file.txt";
    println!("Path: {}", path);

    if let Ok(name) = file_name_from_path(path) {
        println!("  Filename: {}", name);
    }

    if let Ok(dir) = file_directory(path) {
        println!("  Directory: {}", dir);
    }

    if let Ok(ext) = file_extension(path) {
        println!("  Extension: {}", ext);
    }

    println!();
}

#[cfg(target_os = "android")]
fn demo_android_advanced() {
    println!("--- Android System Information ---");

    let version = get_android_version();
    let sdk = get_android_sdk();
    println!("Android Version: {}", version);
    println!("Android SDK: {}", sdk);

    if let Ok(storage) = get_external_storage() {
        println!("External Storage: {}", storage);
    }

    println!("\n--- Advanced Linker Scanner ---");

    let linker = LinkerScanner::get();
    if linker.is_valid() {
        println!("Linker valid");
        println!("  Solist: {:#x}", linker.solist());
        println!("  Somain: {:#x}", linker.somain());
        println!("  Sonext: {:#x}", linker.sonext());

        if let Some(somain_info) = linker.somain_info() {
            println!("\n  Somain Info:");
            println!("    Base: {:#x}", somain_info.base);
            println!("    Size: {:#x}", somain_info.size);
            println!("    Path: {}", somain_info.path);
        }

        if let Some(sonext_info) = linker.sonext_info() {
            println!("\n  Sonext Info:");
            println!("    Base: {:#x}", sonext_info.base);
            println!("    Size: {:#x}", sonext_info.size);
            println!("    Path: {}", sonext_info.path);
        }
    }

    println!();
}

#[cfg(not(target_os = "android"))]
#[allow(dead_code)]
fn demo_android_advanced() {
    // Stub for non-Android platforms
}

#[cfg(target_os = "android")]
fn demo_native_bridge() {
    println!("--- NativeBridge Scanner ---");

    let nb = NativeBridgeScanner::get();
    if nb.is_valid() {
        println!("NativeBridge valid");
        println!("  SODL: {:#x}", nb.sodl());
        println!("  Is Houdini: {}", nb.is_houdini());

        if let Some(sodl_info) = nb.sodl_info() {
            println!("\n  SODL Info:");
            println!("    Base: {:#x}", sodl_info.base);
            println!("    Size: {:#x}", sodl_info.size);
            println!("    Path: {}", sodl_info.path);
        }

        let all_libs = nb.all_soinfo();
        println!("\n  Total libraries: {}", all_libs.len());
        for (i, lib) in all_libs.iter().take(3).enumerate() {
            println!("    [{}] {} ({:#x})", i, lib.path, lib.base);
        }
    } else {
        println!("NativeBridge not available (expected on native ARM)");
    }

    println!();
}

#[cfg(not(target_os = "android"))]
#[allow(dead_code)]
fn demo_native_bridge() {
    // Stub for non-Android platforms
}
