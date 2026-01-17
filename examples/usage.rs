use kittymemory::prelude::*;

fn main() {
    println!("=== KittyMemory-RS Comprehensive Example ===\n");

    demo_memory_operations();
    demo_patching();
    demo_pattern_scanning();
    demo_utility_functions();
    demo_pointer_validation();

    #[cfg(target_os = "android")]
    {
        demo_android_elf_scanner();
        demo_android_linker();
        demo_android_process_maps();
    }

    #[cfg(target_os = "ios")]
    {
        demo_ios_memory_file_info();
    }

    println!("\n=== All examples completed ===");
}

fn demo_memory_operations() {
    println!("--- Memory Operations ---");

    let test_addr = 0x1000usize;

    match mem_read::<i32>(test_addr) {
        Ok(value) => println!("Read i32: {}", value),
        Err(e) => println!("Read failed (expected on invalid address): {}", e),
    }

    #[cfg(target_os = "android")]
    match mem_write(test_addr, &42i32) {
        Ok(_) => println!("Write succeeded"),
        Err(e) => println!("Write failed (expected): {}", e),
    }

    #[cfg(target_os = "android")]
    match get_process_name() {
        Ok(name) => println!("Process name: {}", name),
        Err(e) => println!("Get process name failed: {}", e),
    }

    println!();
}

#[cfg(target_os = "android")]
fn string2offset(hex_str: &str) -> usize {
    let clean = hex_str.trim().trim_start_matches("0x").trim_start_matches("0X");
    usize::from_str_radix(clean, 16).unwrap_or(0)
}

fn demo_patching() {
    println!("--- Memory Patching ---");

    let patch = Patch::with_bytes(0x1000, &[0x90, 0x90, 0x90, 0x90]);
    println!("Patch created: valid={}", patch.is_valid());
    println!("Patch address: {:#x}", patch.address());
    println!("Patch size: {}", patch.size());

    match Patch::with_hex(0x2000, "DEADBEEF") {
        Ok(p) => println!("Hex patch created: valid={}", p.is_valid()),
        Err(e) => println!("Hex patch error: {}", e),
    }

    #[cfg(feature = "keystone")]
    {
        match Patch::with_asm(0x3000, AsmArch::ARM64, "mov x0, #42\nret", 0x3000) {
            Ok(p) => println!("Assembly patch created: valid={}", p.is_valid()),
            Err(e) => println!("Assembly patch error: {}", e),
        }
    }

    #[cfg(target_os = "android")]
    demo_library_patching();

    let backup = Backup::create(0x4000, 64);
    println!("Backup created: valid={}", backup.is_valid());

    println!();
}

#[cfg(target_os = "android")]
fn demo_library_patching() {
    println!("\n--- Library-Based Patching (Android) ---");

    match Patch::with_hex_lib("libil2cpp.so", string2offset("0xD6D93C"), "62 01 0C 00 1E FF 2F E1") {
        Ok(mut patch) => {
            println!("Library hex patch created: valid={}", patch.is_valid());
            println!("  Address: {:#x}", patch.address());
            println!("  Size: {}", patch.size());
            
            if let Ok(orig) = patch.get_orig_bytes() {
                println!("  Original bytes: {}", orig);
            }
            if let Ok(patch_bytes) = patch.get_patch_bytes() {
                println!("  Patch bytes: {}", patch_bytes);
            }
        }
        Err(e) => println!("Library hex patch error (expected if lib not loaded): {}", e),
    }

    match Patch::with_bytes_lib("libc.so", 0x1000, &[0x00, 0x00, 0xA0, 0xE3]) {
        Ok(patch) => {
            println!("Library bytes patch created: valid={}", patch.is_valid());
            println!("  Address: {:#x}", patch.address());
        }
        Err(e) => println!("Library bytes patch error: {}", e),
    }

    #[cfg(feature = "keystone")]
    {
        match Patch::with_asm_lib("libil2cpp.so", 0xD6D93C, AsmArch::ARM32, "mov r0, #1; bx lr") {
            Ok(patch) => println!("Library asm patch created: valid={}", patch.is_valid()),
            Err(e) => println!("Library asm patch error: {}", e),
        }
    }
}

fn demo_pattern_scanning() {
    println!("--- Pattern Scanning ---");

    let start = 0x10000000usize;
    let end = 0x20000000usize;

    if let Some(addr) = find_pattern_first(start, end, "48 8B ? ? 48 89") {
        println!("IDA pattern found at: {:#x}", addr);
    } else {
        println!("IDA pattern not found (expected in this context)");
    }

    if let Some(addr) = find_hex_first(start, end, "DEADBEEF", "xxxxxxxx") {
        println!("Hex pattern found at: {:#x}", addr);
    } else {
        println!("Hex pattern not found (expected)");
    }

    let data = 0x12345678u32;
    if let Some(addr) = find_data_first(start, end, &data) {
        println!("Data found at: {:#x}", addr);
    } else {
        println!("Data not found (expected)");
    }

    let all = find_bytes_all(start, end, &[0x90, 0x90], "xx");
    println!("Found {} occurrences of NOP pattern", all.len());

    println!();
}

fn demo_utility_functions() {
    println!("--- Utility Functions ---");

    let data = vec![0xDE, 0xAD, 0xBE, 0xEF];
    let hex = data_to_hex(&data);
    println!("Bytes to hex: {}", hex);

    match hex_to_data("CAFEBABE") {
        Ok(bytes) => println!("Hex to bytes: {:02X?}", bytes),
        Err(e) => println!("Hex conversion error: {}", e),
    }

    let dump = hex_dump(0x1000, 32);
    println!("Hex dump preview:\n{}", dump.lines().take(3).collect::<Vec<_>>().join("\n"));

    let addr = 0x12345usize;
    println!("Page start of {:#x}: {:#x}", addr, page_start(addr));
    println!("Page end of {:#x}: {:#x}", addr, page_end(addr));

    println!();
}

fn demo_pointer_validation() {
    println!("--- Pointer Validation ---");

    let mut validator = PtrValidator::new();
    validator.set_use_cache(true);

    let test_ptr = 0x1000usize;
    println!("Ptr {:#x} readable: {}", test_ptr, validator.is_ptr_readable(test_ptr, 4));
    println!("Ptr {:#x} writable: {}", test_ptr, validator.is_ptr_writable(test_ptr, 4));
    println!("Ptr {:#x} executable: {}", test_ptr, validator.is_ptr_executable(test_ptr, 4));

    println!();
}

#[cfg(target_os = "android")]
fn demo_android_elf_scanner() {
    println!("--- Android: ELF Scanner ---");

    let program = ElfScanner::get_program();
    println!("Program ELF valid: {}", program.is_valid());

    if program.is_valid() {
        println!("  Base: {:#x}", program.base());
        println!("  End: {:#x}", program.end());
        println!("  Load bias: {:#x}", program.load_bias());
        println!("  Load size: {:#x}", program.load_size());
        println!("  Is native: {}", program.is_native());
        println!("  Is zipped: {}", program.is_zipped());

        if let Some(path) = program.path() {
            println!("  Path: {}", path);
        }

        if let Some(addr) = program.find_symbol("main") {
            println!("  main symbol: {:#x}", addr);
        }
    }

    if let Some(libc) = ElfScanner::find("libc.so") {
        println!("\nlibc.so found:");
        println!("  Base: {:#x}", libc.base());
        println!("  Size: {:#x}", libc.load_size());

        if let Some(addr) = libc.find_symbol("malloc") {
            println!("  malloc: {:#x}", addr);
        }

        if let Some(addr) = libc.find_debug_symbol("free") {
            println!("  free (debug): {:#x}", addr);
        }
    }

    println!();
}

#[cfg(target_os = "android")]
fn demo_android_linker() {
    println!("--- Android: LinkerScanner ---");

    let linker = LinkerScanner::get();
    if !linker.is_valid() {
        println!("LinkerScanner not valid");
        return;
    }

    println!("Linker solist: {:#x}", linker.solist());
    println!("Linker somain: {:#x}", linker.somain());

    let all_libs = linker.all_soinfo();
    println!("\nLoaded libraries: {}", all_libs.len());

    for (i, lib) in all_libs.iter().take(5).enumerate() {
        println!("  [{}] {}", i, lib.path);
        println!("      base={:#x} size={:#x} dynamic={:#x}",
            lib.base, lib.size, lib.dynamic);
    }

    if all_libs.len() > 5 {
        println!("  ... and {} more", all_libs.len() - 5);
    }

    if let Some(libc) = linker.find_soinfo("libc.so") {
        println!("\nlibc.so info:");
        println!("  base: {:#x}", libc.base);
        println!("  size: {:#x}", libc.size);
        println!("  phdr: {:#x}", libc.phdr);
        println!("  bias: {:#x}", libc.bias);
    }

    println!();
}

#[cfg(target_os = "android")]
fn demo_android_process_maps() {
    println!("--- Android: Process Maps ---");

    let all_maps = get_all_maps();
    println!("Total memory maps: {}", all_maps.len());

    for (i, map) in all_maps.iter().take(5).enumerate() {
        println!("  [{}] {:#x}-{:#x} {} {}",
            i,
            map.start_address,
            map.end_address,
            map.protection,
            map.pathname
        );
    }

    if all_maps.len() > 5 {
        println!("  ... and {} more", all_maps.len() - 5);
    }

    let libc_maps = get_maps_filtered("libc.so", ProcMapFilter::Contains);
    println!("\nlibc.so related maps: {}", libc_maps.len());
    for map in libc_maps.iter().take(3) {
        println!("  {:#x}-{:#x} {} readable={} writable={} executable={}",
            map.start_address,
            map.end_address,
            map.protection,
            map.readable,
            map.writeable,
            map.executable
        );
    }

    if let Some(map) = get_address_map(0x1000) {
        println!("\nMap at {:#x}:", 0x1000);
        println!("  Range: {:#x}-{:#x}", map.start_address, map.end_address);
        println!("  Protection: {}", map.protection);
        println!("  Path: {}", map.pathname);
    }

    println!();
}

#[cfg(target_os = "ios")]
fn demo_ios_memory_file_info() {
    println!("--- iOS: MemoryFileInfo ---");

    let base = MemoryFileInfo::get_base_info();
    println!("Base executable:");
    println!("  Name: {}", base.name());
    println!("  Address: {:#x}", base.address());
    println!("  Index: {}", base.index());

    let text_seg = base.get_segment("__TEXT");
    println!("  __TEXT segment: {:#x}-{:#x} (size: {:#x})",
        text_seg.start, text_seg.end, text_seg.size);

    let text_sect = base.get_section("__TEXT", "__text");
    println!("  __text section: {:#x}-{:#x} (size: {:#x})",
        text_sect.start, text_sect.end, text_sect.size);

    if let Some(addr) = base.find_symbol("_main") {
        println!("  _main symbol: {:#x}", addr);
    }

    if let Some(libsystem) = MemoryFileInfo::get_file_info("libSystem.dylib") {
        println!("\nlibSystem.dylib:");
        println!("  Name: {}", libsystem.name());
        println!("  Address: {:#x}", libsystem.address());

        if let Some(addr) = libsystem.find_symbol("_malloc") {
            println!("  _malloc: {:#x}", addr);
        }
    }

    if let Some(abs_addr) = Some(get_absolute_address(None, 0x1000)) {
        println!("\nAbsolute address of base+{:#x}: {:#x}", 0x1000, abs_addr);
    }

    println!();
}
