#include <cstring>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include "wrapper.h"
#include "KittyMemory/KittyMemory.hpp"
#include "KittyMemory/MemoryPatch.hpp"
#include "KittyMemory/MemoryBackup.hpp"
#include "KittyMemory/KittyScanner.hpp"
#include "KittyMemory/KittyPtrValidator.hpp"
#include "KittyMemory/KittyUtils.hpp"
#include "KittyMemory/KittyIOFile.hpp"

static std::unordered_map<uintptr_t, MemoryPatch> g_patches;
static std::unordered_map<uintptr_t, MemoryBackup> g_backups;
static std::mutex g_patches_mutex;
static std::mutex g_backups_mutex;

#ifdef __ANDROID__
static void protection_to_string(int prot, char* out) {
    out[0] = (prot & PROT_READ) ? 'r' : '-';
    out[1] = (prot & PROT_WRITE) ? 'w' : '-';
    out[2] = (prot & PROT_EXEC) ? 'x' : '-';
    out[3] = '\0';
}
#endif

static uintptr_t g_next_patch_id = 1;
static uintptr_t g_next_backup_id = 1;

#ifdef __ANDROID__
static std::unordered_map<uintptr_t, KittyScanner::ElfScanner> g_elf_scanners;
static std::unordered_map<uintptr_t, KittyScanner::LinkerScanner> g_linker_scanners;
static std::unordered_map<uintptr_t, KittyScanner::NativeBridgeScanner> g_native_bridge_scanners;
static std::mutex g_elf_scanners_mutex;
static std::mutex g_linker_scanners_mutex;
static std::mutex g_native_bridge_scanners_mutex;
static uintptr_t g_next_elf_scanner_id = 1;
static uintptr_t g_next_linker_scanner_id = 1;
static uintptr_t g_next_native_bridge_scanner_id = 1;
#endif

#ifdef __APPLE__
static std::unordered_map<uintptr_t, KittyMemory::MemoryFileInfo> g_memory_file_infos;
static std::mutex g_file_info_mutex;
static uintptr_t g_next_file_info_id = 1;
#endif

extern "C" {

bool km_mem_read(const void* address, void* buffer, size_t len) {
    return KittyMemory::memRead(address, buffer, len);
}

#ifdef __ANDROID__
bool km_mem_write(void* address, const void* buffer, size_t len) {
    return KittyMemory::memWrite(address, buffer, len);
}

int km_mem_protect(const void* address, size_t length, int protection) {
    return KittyMemory::memProtect(address, length, protection);
}

char* km_get_process_name(void) {
    std::string name = KittyMemory::getProcessName();
    char* result = (char*)malloc(name.length() + 1);
    if (result) {
        strcpy(result, name.c_str());
    }
    return result;
}

size_t km_syscall_mem_op(km_process_vm_op_t op, uintptr_t address, void* buffer, size_t len) {
    KittyMemory::EPROCESS_VM_OP cpp_op = (op == KM_PROCESS_VM_READV) 
        ? KittyMemory::EPROCESS_VM_OP::READV 
        : KittyMemory::EPROCESS_VM_OP::WRITEV;
    return KittyMemory::syscallMemOp(cpp_op, address, buffer, len);
}

size_t km_syscall_mem_read(uintptr_t address, void* buffer, size_t len) {
    return KittyMemory::syscallMemRead(address, buffer, len);
}

size_t km_syscall_mem_write(uintptr_t address, void* buffer, size_t len) {
    return KittyMemory::syscallMemWrite(address, buffer, len);
}

bool km_dump_mem_to_disk(uintptr_t address, size_t size, const char* destination) {
    return KittyMemory::dumpMemToDisk(address, size, std::string(destination));
}

bool km_dump_mem_file_to_disk(const char* mem_file, const char* destination) {
    return KittyMemory::dumpMemFileToDisk(std::string(mem_file), std::string(destination));
}

int km_get_android_version(void) {
    return KittyUtils::Android::getVersion();
}

int km_get_android_sdk(void) {
    return KittyUtils::Android::getSDK();
}

char* km_get_external_storage(void) {
    std::string storage = KittyUtils::Android::getExternalStorage();
    char* result = (char*)malloc(storage.length() + 1);
    if (result) {
        strcpy(result, storage.c_str());
    }
    return result;
}
#endif

#ifdef __APPLE__
km_memory_status_t km_mem_write_ios(void* address, const void* buffer, size_t len) {
    return (km_memory_status_t)KittyMemory::memWrite(address, buffer, len);
}
#endif

km_patch_t km_patch_create_bytes(uintptr_t address, const void* bytes, size_t size) {
    km_patch_t result = {nullptr, 0, 0, false};

    MemoryPatch patch = MemoryPatch::createWithBytes(address, bytes, size);

    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);

        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }

    return result;
}

km_patch_t km_patch_create_hex(uintptr_t address, const char* hex) {
    km_patch_t result = {nullptr, 0, 0, false};

    MemoryPatch patch = MemoryPatch::createWithHex(address, std::string(hex));

    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);

        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }

    return result;
}

km_patch_t km_patch_create_asm(uintptr_t address, km_asm_arch_t arch, const char* asm_code, uintptr_t asm_address) {
    km_patch_t result = {nullptr, 0, 0, false};

#ifdef kNO_KEYSTONE
    (void)address;
    (void)arch;
    (void)asm_code;
    (void)asm_address;
#else
    MP_ASM_ARCH mp_arch;
    switch (arch) {
        case KM_ASM_ARM32: mp_arch = MP_ASM_ARM32; break;
        case KM_ASM_ARM64: mp_arch = MP_ASM_ARM64; break;
        case KM_ASM_X86: mp_arch = MP_ASM_x86; break;
        case KM_ASM_X86_64: mp_arch = MP_ASM_x86_64; break;
        default: return result;
    }

    MemoryPatch patch = MemoryPatch::createWithAsm(address, mp_arch, std::string(asm_code), asm_address);

    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);

        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }
#endif

    return result;
}

#ifdef __ANDROID__
km_patch_t km_patch_create_hex_lib(const char* lib_name, uintptr_t offset, const char* hex) {
    km_patch_t result = {nullptr, 0, 0, false};
    
    if (!lib_name || !hex) return result;
    
    KittyScanner::ElfScanner scanner = KittyScanner::ElfScanner::findElf(
        std::string(lib_name),
        KittyScanner::EScanElfType::Any,
        KittyScanner::EScanElfFilter::Any
    );
    
    if (!scanner.isValid()) return result;
    
    uintptr_t address = scanner.base() + offset;
    
    MemoryPatch patch = MemoryPatch::createWithHex(address, std::string(hex));
    
    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);
        
        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }
    
    return result;
}

km_patch_t km_patch_create_bytes_lib(const char* lib_name, uintptr_t offset, const void* bytes, size_t size) {
    km_patch_t result = {nullptr, 0, 0, false};
    
    if (!lib_name || !bytes || size == 0) return result;
    
    KittyScanner::ElfScanner scanner = KittyScanner::ElfScanner::findElf(
        std::string(lib_name),
        KittyScanner::EScanElfType::Any,
        KittyScanner::EScanElfFilter::Any
    );
    
    if (!scanner.isValid()) return result;
    
    uintptr_t address = scanner.base() + offset;
    
    MemoryPatch patch = MemoryPatch::createWithBytes(address, bytes, size);
    
    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);
        
        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }
    
    return result;
}

km_patch_t km_patch_create_asm_lib(const char* lib_name, uintptr_t offset, km_asm_arch_t arch, const char* asm_code) {
    km_patch_t result = {nullptr, 0, 0, false};
    
#ifdef kNO_KEYSTONE
    (void)lib_name;
    (void)offset;
    (void)arch;
    (void)asm_code;
#else
    if (!lib_name || !asm_code) return result;
    
    KittyScanner::ElfScanner scanner = KittyScanner::ElfScanner::findElf(
        std::string(lib_name),
        KittyScanner::EScanElfType::Any,
        KittyScanner::EScanElfFilter::Any
    );
    
    if (!scanner.isValid()) return result;
    
    uintptr_t address = scanner.base() + offset;
    
    MP_ASM_ARCH mp_arch;
    switch (arch) {
        case KM_ASM_ARM32: mp_arch = MP_ASM_ARM32; break;
        case KM_ASM_ARM64: mp_arch = MP_ASM_ARM64; break;
        case KM_ASM_X86: mp_arch = MP_ASM_x86; break;
        case KM_ASM_X86_64: mp_arch = MP_ASM_x86_64; break;
        default: return result;
    }
    
    MemoryPatch patch = MemoryPatch::createWithAsm(address, mp_arch, std::string(asm_code), address);
    
    if (patch.isValid()) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = g_next_patch_id++;
        g_patches[id] = std::move(patch);
        
        result.handle = (void*)id;
        result.address = g_patches[id].get_TargetAddress();
        result.size = g_patches[id].get_PatchSize();
        result.valid = true;
    }
#endif
    
    return result;
}
#endif

bool km_patch_modify(km_patch_t* patch) {
    if (!patch || !patch->valid || !patch->handle) return false;

    std::lock_guard<std::mutex> lock(g_patches_mutex);
    uintptr_t id = (uintptr_t)patch->handle;

    auto it = g_patches.find(id);
    if (it == g_patches.end()) return false;

    return it->second.Modify();
}

bool km_patch_restore(km_patch_t* patch) {
    if (!patch || !patch->valid || !patch->handle) return false;

    std::lock_guard<std::mutex> lock(g_patches_mutex);
    uintptr_t id = (uintptr_t)patch->handle;

    auto it = g_patches.find(id);
    if (it == g_patches.end()) return false;

    return it->second.Restore();
}

void km_patch_free(km_patch_t* patch) {
    if (patch && patch->handle) {
        std::lock_guard<std::mutex> lock(g_patches_mutex);
        uintptr_t id = (uintptr_t)patch->handle;
        g_patches.erase(id);
        patch->handle = nullptr;
        patch->valid = false;
    }
}

char* km_patch_get_curr_bytes(km_patch_t* patch) {
    if (!patch || !patch->valid || !patch->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_patches_mutex);
    uintptr_t id = (uintptr_t)patch->handle;

    auto it = g_patches.find(id);
    if (it == g_patches.end()) return nullptr;

    std::string hex = it->second.get_CurrBytes();
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

char* km_patch_get_orig_bytes(km_patch_t* patch) {
    if (!patch || !patch->valid || !patch->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_patches_mutex);
    uintptr_t id = (uintptr_t)patch->handle;

    auto it = g_patches.find(id);
    if (it == g_patches.end()) return nullptr;

    std::string hex = it->second.get_OrigBytes();
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

char* km_patch_get_patch_bytes(km_patch_t* patch) {
    if (!patch || !patch->valid || !patch->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_patches_mutex);
    uintptr_t id = (uintptr_t)patch->handle;

    auto it = g_patches.find(id);
    if (it == g_patches.end()) return nullptr;

    std::string hex = it->second.get_PatchBytes();
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

km_backup_t km_backup_create(uintptr_t address, size_t size) {
    km_backup_t result = {nullptr, 0, 0, false};

    MemoryBackup backup = MemoryBackup::createBackup(address, size);

    if (backup.isValid()) {
        std::lock_guard<std::mutex> lock(g_backups_mutex);
        uintptr_t id = g_next_backup_id++;
        g_backups[id] = std::move(backup);

        result.handle = (void*)id;
        result.address = g_backups[id].get_TargetAddress();
        result.size = g_backups[id].get_BackupSize();
        result.valid = true;
    }

    return result;
}

bool km_backup_restore(km_backup_t* backup) {
    if (!backup || !backup->valid || !backup->handle) return false;

    std::lock_guard<std::mutex> lock(g_backups_mutex);
    uintptr_t id = (uintptr_t)backup->handle;

    auto it = g_backups.find(id);
    if (it == g_backups.end()) return false;

    return it->second.Restore();
}

void km_backup_free(km_backup_t* backup) {
    if (backup && backup->handle) {
        std::lock_guard<std::mutex> lock(g_backups_mutex);
        uintptr_t id = (uintptr_t)backup->handle;
        g_backups.erase(id);
        backup->handle = nullptr;
        backup->valid = false;
    }
}

char* km_backup_get_curr_bytes(km_backup_t* backup) {
    if (!backup || !backup->valid || !backup->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_backups_mutex);
    uintptr_t id = (uintptr_t)backup->handle;

    auto it = g_backups.find(id);
    if (it == g_backups.end()) return nullptr;

    std::string hex = it->second.get_CurrBytes();
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

char* km_backup_get_orig_bytes(km_backup_t* backup) {
    if (!backup || !backup->valid || !backup->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_backups_mutex);
    uintptr_t id = (uintptr_t)backup->handle;

    auto it = g_backups.find(id);
    if (it == g_backups.end()) return nullptr;

    std::string hex = it->second.get_OrigBytes();
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

uintptr_t km_find_bytes_first(uintptr_t start, uintptr_t end, const char* bytes, const char* mask) {
    return KittyScanner::findBytesFirst(start, end, bytes, std::string(mask));
}

uintptr_t km_find_hex_first(uintptr_t start, uintptr_t end, const char* hex, const char* mask) {
    return KittyScanner::findHexFirst(start, end, std::string(hex), std::string(mask));
}

uintptr_t km_find_pattern_first(uintptr_t start, uintptr_t end, const char* pattern) {
    return KittyScanner::findIdaPatternFirst(start, end, std::string(pattern));
}

uintptr_t km_find_data_first(uintptr_t start, uintptr_t end, const void* data, size_t size) {
    return KittyScanner::findDataFirst(start, end, data, size);
}

size_t km_find_bytes_all(uintptr_t start, uintptr_t end, const char* bytes, const char* mask, uintptr_t** results) {
    std::vector<uintptr_t> found = KittyScanner::findBytesAll(start, end, bytes, std::string(mask));
    if (found.empty()) {
        *results = nullptr;
        return 0;
    }

    *results = (uintptr_t*)malloc(found.size() * sizeof(uintptr_t));
    if (!*results) return 0;

    for (size_t i = 0; i < found.size(); ++i) {
        (*results)[i] = found[i];
    }

    return found.size();
}

size_t km_find_hex_all(uintptr_t start, uintptr_t end, const char* hex, const char* mask, uintptr_t** results) {
    std::vector<uintptr_t> found = KittyScanner::findHexAll(start, end, std::string(hex), std::string(mask));
    if (found.empty()) {
        *results = nullptr;
        return 0;
    }

    *results = (uintptr_t*)malloc(found.size() * sizeof(uintptr_t));
    if (!*results) return 0;

    for (size_t i = 0; i < found.size(); ++i) {
        (*results)[i] = found[i];
    }

    return found.size();
}

size_t km_find_pattern_all(uintptr_t start, uintptr_t end, const char* pattern, uintptr_t** results) {
    std::vector<uintptr_t> found = KittyScanner::findIdaPatternAll(start, end, std::string(pattern));
    if (found.empty()) {
        *results = nullptr;
        return 0;
    }

    *results = (uintptr_t*)malloc(found.size() * sizeof(uintptr_t));
    if (!*results) return 0;

    for (size_t i = 0; i < found.size(); ++i) {
        (*results)[i] = found[i];
    }

    return found.size();
}

size_t km_find_data_all(uintptr_t start, uintptr_t end, const void* data, size_t size, uintptr_t** results) {
    std::vector<uintptr_t> found = KittyScanner::findDataAll(start, end, data, size);
    if (found.empty()) {
        *results = nullptr;
        return 0;
    }

    *results = (uintptr_t*)malloc(found.size() * sizeof(uintptr_t));
    if (!*results) return 0;

    for (size_t i = 0; i < found.size(); ++i) {
        (*results)[i] = found[i];
    }

    return found.size();
}

void km_free_results(uintptr_t* results) {
    if (results) free(results);
}

#ifdef __ANDROID__
km_elf_scanner_t km_elf_scanner_create(uintptr_t base) {
    km_elf_scanner_t result = {0};
    KittyScanner::ElfScanner scanner(base, KittyMemory::getAllMaps());
    if (scanner.isValid()) {
        std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
        uintptr_t id = g_next_elf_scanner_id++;
        g_elf_scanners[id] = scanner;
        result.handle = (void*)id;
        result.valid = true;
    }
    return result;
}

km_elf_scanner_t km_elf_scanner_get_program(void) {
    km_elf_scanner_t result = {0};
    KittyScanner::ElfScanner scanner = KittyScanner::ElfScanner::getProgramElf();
    if (scanner.isValid()) {
        std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
        uintptr_t id = g_next_elf_scanner_id++;
        g_elf_scanners[id] = scanner;
        result.handle = (void*)id;
        result.valid = true;
    }
    return result;
}

km_elf_scanner_t km_elf_scanner_find(const char* path) {
    km_elf_scanner_t result = {0};
    KittyScanner::ElfScanner scanner = KittyScanner::ElfScanner::findElf(
        std::string(path),
        KittyScanner::EScanElfType::Any,
        KittyScanner::EScanElfFilter::Any
    );

    if (scanner.isValid()) {
        std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
        uintptr_t id = g_next_elf_scanner_id++;
        g_elf_scanners[id] = scanner;
        result.handle = (void*)id;
        result.valid = true;
    }

    return result;
}

uintptr_t km_elf_find_symbol(km_elf_scanner_t* scanner, const char* symbol) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.findSymbol(std::string(symbol));
}

uintptr_t km_elf_find_debug_symbol(km_elf_scanner_t* scanner, const char* symbol) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.findDebugSymbol(std::string(symbol));
}

uintptr_t km_elf_get_base(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.base();
}

uintptr_t km_elf_get_end(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.end();
}

uintptr_t km_elf_get_load_bias(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.loadBias();
}

size_t km_elf_get_load_size(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.loadSize();
}

char* km_elf_get_path(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return nullptr;

    std::string path = it->second.realPath();
    char* result = (char*)malloc(path.length() + 1);
    if (result) {
        strcpy(result, path.c_str());
    }
    return result;
}

bool km_elf_is_zipped(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    return it->second.isZipped();
}

bool km_elf_is_native(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    return it->second.isNative();
}

bool km_elf_is_emulated(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    return it->second.isEmulated();
}

bool km_elf_dump_to_disk(km_elf_scanner_t* scanner, const char* destination) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    return it->second.dumpToDisk(std::string(destination));
}

uintptr_t km_elf_get_phdr(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.phdr();
}

int km_elf_get_loads(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.loads();
}

uintptr_t km_elf_get_dynamic(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.dynamic();
}

uintptr_t km_elf_get_string_table(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.stringTable();
}

uintptr_t km_elf_get_symbol_table(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.symbolTable();
}

size_t km_elf_get_string_table_size(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.stringTableSize();
}

size_t km_elf_get_symbol_entry_size(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.symbolEntrySize();
}

uintptr_t km_elf_get_elf_hash_table(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.elfHashTable();
}

uintptr_t km_elf_get_gnu_hash_table(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return 0;

    return it->second.gnuHashTable();
}

char* km_elf_get_file_path(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return nullptr;

    std::string path = it->second.filePath();
    char* result = (char*)malloc(path.length() + 1);
    if (result) {
        strcpy(result, path.c_str());
    }
    return result;
}

char* km_elf_get_real_path(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return nullptr;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return nullptr;

    std::string path = it->second.realPath();
    char* result = (char*)malloc(path.length() + 1);
    if (result) {
        strcpy(result, path.c_str());
    }
    return result;
}

bool km_elf_is_fixed_by_soinfo(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    return it->second.isFixedBySoInfo();
}

void km_elf_refresh(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it != g_elf_scanners.end()) {
        it->second.refresh();
    }
}

void km_elf_scanner_free(km_elf_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
        uintptr_t id = (uintptr_t)scanner->handle;
        g_elf_scanners.erase(id);
        scanner->handle = nullptr;
        scanner->valid = false;
    }
}

km_proc_map_t km_get_all_maps_impl(km_proc_map_t** maps) {
    std::vector<KittyMemory::ProcMap> all_maps = KittyMemory::getAllMaps();
    if (all_maps.empty()) {
        *maps = nullptr;
        return {};
    }

    *maps = (km_proc_map_t*)malloc(all_maps.size() * sizeof(km_proc_map_t));
    if (!*maps) return {};

    for (size_t i = 0; i < all_maps.size(); ++i) {
        const auto& pm = all_maps[i];
        (*maps)[i].startAddress = pm.startAddress;
        (*maps)[i].endAddress = pm.endAddress;
        (*maps)[i].length = pm.length;
        protection_to_string(pm.protection, (*maps)[i].protection);
        (*maps)[i].readable = pm.readable;
        (*maps)[i].writeable = pm.writeable;
        (*maps)[i].executable = pm.executable;
        (*maps)[i].is_private = pm.is_private;
        (*maps)[i].is_shared = pm.is_shared;
        (*maps)[i].is_ro = pm.is_ro;
        (*maps)[i].is_rw = pm.is_rw;
        (*maps)[i].is_rx = pm.is_rx;
        (*maps)[i].offset = pm.offset;
        strncpy((*maps)[i].dev, pm.dev.c_str(), 15);
        (*maps)[i].dev[15] = '\0';
        (*maps)[i].inode = pm.inode;
        strncpy((*maps)[i].pathname, pm.pathname.c_str(), 255);
        (*maps)[i].pathname[255] = '\0';
    }

    return {};
}

size_t km_get_all_maps(km_proc_map_t** maps) {
    std::vector<KittyMemory::ProcMap> all_maps = KittyMemory::getAllMaps();
    if (all_maps.empty()) {
        *maps = nullptr;
        return 0;
    }

    *maps = (km_proc_map_t*)malloc(all_maps.size() * sizeof(km_proc_map_t));
    if (!*maps) return 0;

    for (size_t i = 0; i < all_maps.size(); ++i) {
        const auto& pm = all_maps[i];
        (*maps)[i].startAddress = pm.startAddress;
        (*maps)[i].endAddress = pm.endAddress;
        (*maps)[i].length = pm.length;
        protection_to_string(pm.protection, (*maps)[i].protection);
        (*maps)[i].readable = pm.readable;
        (*maps)[i].writeable = pm.writeable;
        (*maps)[i].executable = pm.executable;
        (*maps)[i].is_private = pm.is_private;
        (*maps)[i].is_shared = pm.is_shared;
        (*maps)[i].is_ro = pm.is_ro;
        (*maps)[i].is_rw = pm.is_rw;
        (*maps)[i].is_rx = pm.is_rx;
        (*maps)[i].offset = pm.offset;
        strncpy((*maps)[i].dev, pm.dev.c_str(), 15);
        (*maps)[i].dev[15] = '\0';
        (*maps)[i].inode = pm.inode;
        strncpy((*maps)[i].pathname, pm.pathname.c_str(), 255);
        (*maps)[i].pathname[255] = '\0';
    }

    return all_maps.size();
}

size_t km_get_maps_filtered(const char* name, int filter, km_proc_map_t** maps) {
    std::vector<KittyMemory::ProcMap> filtered_maps = KittyMemory::getMaps(
        static_cast<KittyMemory::EProcMapFilter>(filter),
        std::string(name)
    );

    if (filtered_maps.empty()) {
        *maps = nullptr;
        return 0;
    }

    *maps = (km_proc_map_t*)malloc(filtered_maps.size() * sizeof(km_proc_map_t));
    if (!*maps) return 0;

    for (size_t i = 0; i < filtered_maps.size(); ++i) {
        const auto& pm = filtered_maps[i];
        (*maps)[i].startAddress = pm.startAddress;
        (*maps)[i].endAddress = pm.endAddress;
        (*maps)[i].length = pm.length;
        protection_to_string(pm.protection, (*maps)[i].protection);
        (*maps)[i].readable = pm.readable;
        (*maps)[i].writeable = pm.writeable;
        (*maps)[i].executable = pm.executable;
        (*maps)[i].is_private = pm.is_private;
        (*maps)[i].is_shared = pm.is_shared;
        (*maps)[i].is_ro = pm.is_ro;
        (*maps)[i].is_rw = pm.is_rw;
        (*maps)[i].is_rx = pm.is_rx;
        (*maps)[i].offset = pm.offset;
        strncpy((*maps)[i].dev, pm.dev.c_str(), 15);
        (*maps)[i].dev[15] = '\0';
        (*maps)[i].inode = pm.inode;
        strncpy((*maps)[i].pathname, pm.pathname.c_str(), 255);
        (*maps)[i].pathname[255] = '\0';
    }

    return filtered_maps.size();
}

bool km_get_address_map(uintptr_t address, km_proc_map_t* map) {
    if (!map) return false;

    KittyMemory::ProcMap pm = KittyMemory::getAddressMap((const void*)address);
    if (!pm.isValid()) return false;

    map->startAddress = pm.startAddress;
    map->endAddress = pm.endAddress;
    map->length = pm.length;
    protection_to_string(pm.protection, map->protection);
    map->readable = pm.readable;
    map->writeable = pm.writeable;
    map->executable = pm.executable;
    map->is_private = pm.is_private;
    map->is_shared = pm.is_shared;
    map->is_ro = pm.is_ro;
    map->is_rw = pm.is_rw;
    map->is_rx = pm.is_rx;
    map->offset = pm.offset;
    strncpy(map->dev, pm.dev.c_str(), 15);
    map->dev[15] = '\0';
    map->inode = pm.inode;
    strncpy(map->pathname, pm.pathname.c_str(), 255);
    map->pathname[255] = '\0';

    return true;
}

void km_free_maps(km_proc_map_t* maps) {
    if (maps) free(maps);
}

bool km_proc_map_is_valid(km_proc_map_t* map) {
    if (!map) return false;
    return (map->startAddress && map->endAddress && map->length);
}

bool km_proc_map_is_unknown(km_proc_map_t* map) {
    if (!map) return false;
    return (map->pathname[0] == '\0');
}

bool km_proc_map_is_valid_elf(km_proc_map_t* map) {
    if (!map || !km_proc_map_is_valid(map)) return false;
    if (map->length <= 4 || !map->readable) return false;
    return memcmp((const void*)map->startAddress, "\177ELF", 4) == 0;
}

bool km_proc_map_contains(km_proc_map_t* map, uintptr_t address) {
    if (!map) return false;
    return address >= map->startAddress && address < map->endAddress;
}

char* km_proc_map_to_string(km_proc_map_t* map) {
    if (!map) return nullptr;

    std::stringstream ss;
    ss << std::hex << std::uppercase;
    ss << map->startAddress << "-" << map->endAddress << " ";
    ss << map->protection << " ";
    ss << std::hex << map->offset << " ";
    ss << map->dev << " " << map->inode << " " << map->pathname;

    std::string str = ss.str();
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

bool km_elf_find_register_native(km_elf_scanner_t* scanner, const char* name, const char* signature, km_register_native_fn_t* result) {
    if (!scanner || !scanner->valid || !scanner->handle || !result) return false;

    std::lock_guard<std::mutex> lock(g_elf_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_elf_scanners.find(id);
    if (it == g_elf_scanners.end()) return false;

    KittyScanner::RegisterNativeFn fn = it->second.findRegisterNativeFn(std::string(name), std::string(signature));

    if (fn.fnPtr == nullptr) return false;

    strncpy(result->name, fn.name ? fn.name : "", 127);
    result->name[127] = '\0';
    strncpy(result->signature, fn.signature ? fn.signature : "", 255);
    result->signature[255] = '\0';
    result->fnPtr = (uintptr_t)fn.fnPtr;

    return true;
}

km_linker_scanner_t km_linker_scanner_get(void) {
    km_linker_scanner_t result = {0};
    KittyScanner::LinkerScanner scanner = KittyScanner::LinkerScanner::Get();
    if (scanner.isValid()) {
        std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
        uintptr_t id = g_next_linker_scanner_id++;
        g_linker_scanners[id] = scanner;
        result.handle = (void*)id;
        result.valid = true;
    }
    return result;
}

uintptr_t km_linker_solist(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return 0;
    return it->second.solist();
}

uintptr_t km_linker_somain(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return 0;
    return it->second.somain();
}

uintptr_t km_linker_sonext(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return 0;
    return it->second.sonext();
}

size_t km_linker_all_soinfo(km_linker_scanner_t* scanner, km_soinfo_t** infos) {
    if (!scanner || !scanner->valid || !scanner->handle) {
        *infos = nullptr;
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) {
        *infos = nullptr;
        return 0;
    }

    std::vector<KittyScanner::kitty_soinfo_t> all_info = it->second.allSoInfo();

    if (all_info.empty()) {
        *infos = nullptr;
        return 0;
    }

    *infos = (km_soinfo_t*)malloc(all_info.size() * sizeof(km_soinfo_t));
    if (!*infos) return 0;

    for (size_t i = 0; i < all_info.size(); ++i) {
        const auto& si = all_info[i];
        (*infos)[i].base = si.base;
        (*infos)[i].size = si.size;
        (*infos)[i].phdr = si.phdr;
        (*infos)[i].phnum = si.phnum;
        (*infos)[i].dyn = si.dyn;
        (*infos)[i].strtab = si.strtab;
        (*infos)[i].symtab = si.symtab;
        (*infos)[i].strsz = si.strsz;
        (*infos)[i].bias = si.bias;
        (*infos)[i].next = si.next;
        (*infos)[i].e_machine = si.e_machine;
        strncpy((*infos)[i].path, si.path.c_str(), 255);
        (*infos)[i].path[255] = '\0';
        strncpy((*infos)[i].realpath, si.realpath.c_str(), 255);
        (*infos)[i].realpath[255] = '\0';
    }

    return all_info.size();
}

bool km_linker_find_soinfo(km_linker_scanner_t* scanner, const char* name, km_soinfo_t* info) {
    if (!scanner || !scanner->valid || !scanner->handle || !info) return false;

    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return false;

    KittyScanner::kitty_soinfo_t si = it->second.findSoInfo(std::string(name));

    if (si.base == 0) return false;

    info->base = si.base;
    info->size = si.size;
    info->phdr = si.phdr;
    info->phnum = si.phnum;
    info->dyn = si.dyn;
    info->strtab = si.strtab;
    info->symtab = si.symtab;
    info->strsz = si.strsz;
    info->bias = si.bias;
    info->next = si.next;
    info->e_machine = si.e_machine;
    strncpy(info->path, si.path.c_str(), 255);
    info->path[255] = '\0';
    strncpy(info->realpath, si.realpath.c_str(), 255);
    info->realpath[255] = '\0';

    return true;
}

void km_free_soinfos(km_soinfo_t* infos) {
    if (infos) free(infos);
}

bool km_linker_get_somain_info(km_linker_scanner_t* scanner, km_soinfo_t* info) {
    if (!scanner || !scanner->valid || !scanner->handle || !info) return false;

    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return false;

    KittyScanner::kitty_soinfo_t si = it->second.somainInfo();
    if (si.base == 0) return false;

    info->base = si.base;
    info->size = si.size;
    info->phdr = si.phdr;
    info->phnum = si.phnum;
    info->dyn = si.dyn;
    info->strtab = si.strtab;
    info->symtab = si.symtab;
    info->strsz = si.strsz;
    info->bias = si.bias;
    info->next = si.next;
    info->e_machine = si.e_machine;
    strncpy(info->path, si.path.c_str(), 255);
    info->path[255] = '\0';
    strncpy(info->realpath, si.realpath.c_str(), 255);
    info->realpath[255] = '\0';

    return true;
}

bool km_linker_get_sonext_info(km_linker_scanner_t* scanner, km_soinfo_t* info) {
    if (!scanner || !scanner->valid || !scanner->handle || !info) return false;

    std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_linker_scanners.find(id);
    if (it == g_linker_scanners.end()) return false;

    KittyScanner::kitty_soinfo_t si = it->second.sonextInfo();
    if (si.base == 0) return false;

    info->base = si.base;
    info->size = si.size;
    info->phdr = si.phdr;
    info->phnum = si.phnum;
    info->dyn = si.dyn;
    info->strtab = si.strtab;
    info->symtab = si.symtab;
    info->strsz = si.strsz;
    info->bias = si.bias;
    info->next = si.next;
    info->e_machine = si.e_machine;
    strncpy(info->path, si.path.c_str(), 255);
    info->path[255] = '\0';
    strncpy(info->realpath, si.realpath.c_str(), 255);
    info->realpath[255] = '\0';

    return true;
}

void km_linker_scanner_free(km_linker_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        std::lock_guard<std::mutex> lock(g_linker_scanners_mutex);
        uintptr_t id = (uintptr_t)scanner->handle;
        g_linker_scanners.erase(id);
        scanner->handle = nullptr;
        scanner->valid = false;
    }
}

km_native_bridge_scanner_t km_native_bridge_scanner_get(void) {
    km_native_bridge_scanner_t result = {0};
    KittyScanner::NativeBridgeScanner scanner = KittyScanner::NativeBridgeScanner::Get();
    if (scanner.isValid()) {
        std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
        uintptr_t id = g_next_native_bridge_scanner_id++;
        g_native_bridge_scanners[id] = scanner;
        result.handle = (void*)id;
        result.valid = true;
    }
    return result;
}

bool km_native_bridge_scanner_is_valid(km_native_bridge_scanner_t* scanner) {
    if (!scanner || !scanner->handle) return false;
    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) return false;
    return it->second.isValid();
}

uintptr_t km_native_bridge_scanner_sodl(km_native_bridge_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) return 0;
    return it->second.sohead();
}

bool km_native_bridge_scanner_get_sodl_info(km_native_bridge_scanner_t* scanner, km_soinfo_t* info) {
    if (!scanner || !scanner->valid || !scanner->handle || !info) return false;

    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) return false;

    KittyScanner::kitty_soinfo_t si = it->second.soheadInfo();
    if (si.base == 0) return false;

    info->base = si.base;
    info->size = si.size;
    info->phdr = si.phdr;
    info->phnum = si.phnum;
    info->dyn = si.dyn;
    info->strtab = si.strtab;
    info->symtab = si.symtab;
    info->strsz = si.strsz;
    info->bias = si.bias;
    info->next = si.next;
    info->e_machine = si.e_machine;
    strncpy(info->path, si.path.c_str(), 255);
    info->path[255] = '\0';
    strncpy(info->realpath, si.realpath.c_str(), 255);
    info->realpath[255] = '\0';

    return true;
}

size_t km_native_bridge_scanner_all_soinfo(km_native_bridge_scanner_t* scanner, km_soinfo_t** infos) {
    if (!scanner || !scanner->valid || !scanner->handle) {
        *infos = nullptr;
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) {
        *infos = nullptr;
        return 0;
    }

    std::vector<KittyScanner::kitty_soinfo_t> all_info = it->second.allSoInfo();

    if (all_info.empty()) {
        *infos = nullptr;
        return 0;
    }

    *infos = (km_soinfo_t*)malloc(all_info.size() * sizeof(km_soinfo_t));
    if (!*infos) return 0;

    for (size_t i = 0; i < all_info.size(); ++i) {
        const auto& si = all_info[i];
        (*infos)[i].base = si.base;
        (*infos)[i].size = si.size;
        (*infos)[i].phdr = si.phdr;
        (*infos)[i].phnum = si.phnum;
        (*infos)[i].dyn = si.dyn;
        (*infos)[i].strtab = si.strtab;
        (*infos)[i].symtab = si.symtab;
        (*infos)[i].strsz = si.strsz;
        (*infos)[i].bias = si.bias;
        (*infos)[i].next = si.next;
        (*infos)[i].e_machine = si.e_machine;
        strncpy((*infos)[i].path, si.path.c_str(), 255);
        (*infos)[i].path[255] = '\0';
        strncpy((*infos)[i].realpath, si.realpath.c_str(), 255);
        (*infos)[i].realpath[255] = '\0';
    }

    return all_info.size();
}

bool km_native_bridge_scanner_find_soinfo(km_native_bridge_scanner_t* scanner, const char* name, km_soinfo_t* info) {
    if (!scanner || !scanner->valid || !scanner->handle || !info) return false;

    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) return false;

    KittyScanner::kitty_soinfo_t si = it->second.findSoInfo(std::string(name));

    if (si.base == 0) return false;

    info->base = si.base;
    info->size = si.size;
    info->phdr = si.phdr;
    info->phnum = si.phnum;
    info->dyn = si.dyn;
    info->strtab = si.strtab;
    info->symtab = si.symtab;
    info->strsz = si.strsz;
    info->bias = si.bias;
    info->next = si.next;
    info->e_machine = si.e_machine;
    strncpy(info->path, si.path.c_str(), 255);
    info->path[255] = '\0';
    strncpy(info->realpath, si.realpath.c_str(), 255);
    info->realpath[255] = '\0';

    return true;
}

bool km_native_bridge_scanner_is_houdini(km_native_bridge_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;
    std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
    uintptr_t id = (uintptr_t)scanner->handle;
    auto it = g_native_bridge_scanners.find(id);
    if (it == g_native_bridge_scanners.end()) return false;
    return it->second.isHoudini();
}

void km_native_bridge_scanner_free(km_native_bridge_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        std::lock_guard<std::mutex> lock(g_native_bridge_scanners_mutex);
        uintptr_t id = (uintptr_t)scanner->handle;
        g_native_bridge_scanners.erase(id);
        scanner->handle = nullptr;
        scanner->valid = false;
    }
}

void* km_native_bridge_linker_dlopen(const char* path, int flags) {
    return KittyScanner::NativeBridgeLinker::dlopen(std::string(path), flags);
}

void* km_native_bridge_linker_dlsym(void* handle, const char* sym_name) {
    return KittyScanner::NativeBridgeLinker::dlsym(handle, std::string(sym_name));
}

const char* km_native_bridge_linker_dlerror(void) {
    return KittyScanner::NativeBridgeLinker::dlerror();
}

bool km_native_bridge_linker_dladdr(const void* addr, km_soinfo_t* info) {
    if (!info) return false;
    KittyScanner::kitty_soinfo_t si;
    bool result = KittyScanner::NativeBridgeLinker::dladdr(addr, &si);
    if (result) {
        info->base = si.base;
        info->size = si.size;
        info->phdr = si.phdr;
        info->phnum = si.phnum;
        info->dyn = si.dyn;
        info->strtab = si.strtab;
        info->symtab = si.symtab;
        info->strsz = si.strsz;
        info->bias = si.bias;
        info->next = si.next;
        info->e_machine = si.e_machine;
        strncpy(info->path, si.path.c_str(), 255);
        info->path[255] = '\0';
        strncpy(info->realpath, si.realpath.c_str(), 255);
        info->realpath[255] = '\0';
    }
    return result;
}

#endif

#ifdef __APPLE__
km_memory_file_info_t km_get_base_info(void) {
    km_memory_file_info_t result = {0};

    KittyMemory::MemoryFileInfo info = KittyMemory::getBaseInfo();

    if (info.header) {
        std::lock_guard<std::mutex> lock(g_file_info_mutex);
        uintptr_t id = g_next_file_info_id++;
        g_memory_file_infos[id] = info;

        result.handle = (void*)id;
        result.index = info.index;
        result.name = info.name;
        result.address = info.address;
    }

    return result;
}

km_memory_file_info_t km_get_memory_file_info(const char* file_name) {
    km_memory_file_info_t result = {0};

    KittyMemory::MemoryFileInfo info = KittyMemory::getMemoryFileInfo(std::string(file_name));

    if (info.header) {
        std::lock_guard<std::mutex> lock(g_file_info_mutex);
        uintptr_t id = g_next_file_info_id++;
        g_memory_file_infos[id] = info;

        result.handle = (void*)id;
        result.index = info.index;
        result.name = info.name;
        result.address = info.address;
    }

    return result;
}

uintptr_t km_get_absolute_address(const char* file_name, uintptr_t address) {
    return KittyMemory::getAbsoluteAddress(file_name, address);
}

km_segment_data_t km_get_segment(km_memory_file_info_t* info, const char* seg_name) {
    km_segment_data_t result = {0};

    if (!info || !info->handle) return result;

    std::lock_guard<std::mutex> lock(g_file_info_mutex);
    uintptr_t id = (uintptr_t)info->handle;

    auto it = g_memory_file_infos.find(id);
    if (it == g_memory_file_infos.end()) return result;

    KittyMemory::seg_data_t seg = it->second.getSegment(seg_name);
    result.start = seg.start;
    result.end = seg.end;
    result.size = seg.size;

    return result;
}

km_segment_data_t km_get_section(km_memory_file_info_t* info, const char* seg_name, const char* sect_name) {
    km_segment_data_t result = {0};

    if (!info || !info->handle) return result;

    std::lock_guard<std::mutex> lock(g_file_info_mutex);
    uintptr_t id = (uintptr_t)info->handle;

    auto it = g_memory_file_infos.find(id);
    if (it == g_memory_file_infos.end()) return result;

    KittyMemory::seg_data_t sect = it->second.getSection(seg_name, sect_name);
    result.start = sect.start;
    result.end = sect.end;
    result.size = sect.size;

    return result;
}

uintptr_t km_find_symbol_in_lib(const char* lib, const char* symbol) {
    return KittyScanner::findSymbol(std::string(lib), std::string(symbol));
}

uintptr_t km_find_symbol_in_file(km_memory_file_info_t* info, const char* symbol) {
    if (!info || !info->handle) return 0;

    std::lock_guard<std::mutex> lock(g_file_info_mutex);
    uintptr_t id = (uintptr_t)info->handle;

    auto it = g_memory_file_infos.find(id);
    if (it == g_memory_file_infos.end()) return 0;

    return KittyScanner::findSymbol(it->second, std::string(symbol));
}

void km_free_memory_file_info(km_memory_file_info_t* info) {
    if (info && info->handle) {
        std::lock_guard<std::mutex> lock(g_file_info_mutex);
        uintptr_t id = (uintptr_t)info->handle;
        g_memory_file_infos.erase(id);
        info->handle = nullptr;
    }
}
#endif

km_ptr_validator_t km_validator_create(void) {
    km_ptr_validator_t result;
    result.handle = new KittyPtrValidator();
    return result;
}

void km_validator_set_cache(km_ptr_validator_t* validator, bool use_cache) {
    if (validator && validator->handle) {
        ((KittyPtrValidator*)validator->handle)->setUseCache(use_cache);
    }
}

bool km_validator_is_readable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len) {
    if (!validator || !validator->handle) return false;
    return ((KittyPtrValidator*)validator->handle)->isPtrReadable(ptr, len);
}

bool km_validator_is_writable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len) {
    if (!validator || !validator->handle) return false;
    return ((KittyPtrValidator*)validator->handle)->isPtrWritable(ptr, len);
}

bool km_validator_is_executable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len) {
    if (!validator || !validator->handle) return false;
    return ((KittyPtrValidator*)validator->handle)->isPtrExecutable(ptr, len);
}

void km_validator_free(km_ptr_validator_t* validator) {
    if (validator && validator->handle) {
        delete (KittyPtrValidator*)validator->handle;
        validator->handle = nullptr;
    }
}

char* km_data_to_hex(const void* data, size_t len) {
    std::string hex = KittyUtils::Data::toHex(data, len);
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

bool km_hex_to_data(const char* hex, void* data, size_t data_len) {
    if (!hex || !data) return false;

    std::string hex_str(hex);
    if (!KittyUtils::String::validateHex(hex_str)) return false;

    size_t expected_len = hex_str.length() / 2;
    if (expected_len != data_len) return false;

    KittyUtils::Data::fromHex(hex_str, data);
    return true;
}

char* km_hex_dump(const void* address, size_t len) {
    std::string dump = KittyUtils::Data::hexDump<16, true>(address, len);
    char* result = (char*)malloc(dump.length() + 1);
    if (result) {
        strcpy(result, dump.c_str());
    }
    return result;
}

void km_free_string(char* str) {
    if (str) free(str);
}

char* km_file_name_from_path(const char* file_path) {
    if (!file_path) return nullptr;
    std::string name = KittyUtils::Path::fileName(std::string(file_path));
    char* result = (char*)malloc(name.length() + 1);
    if (result) {
        strcpy(result, name.c_str());
    }
    return result;
}

char* km_file_directory(const char* file_path) {
    if (!file_path) return nullptr;
    std::string dir = KittyUtils::Path::fileDirectory(std::string(file_path));
    char* result = (char*)malloc(dir.length() + 1);
    if (result) {
        strcpy(result, dir.c_str());
    }
    return result;
}

char* km_file_extension(const char* file_path) {
    if (!file_path) return nullptr;
    std::string ext = KittyUtils::Path::fileExtension(std::string(file_path));
    char* result = (char*)malloc(ext.length() + 1);
    if (result) {
        strcpy(result, ext.c_str());
    }
    return result;
}

bool km_string_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    return KittyUtils::String::startsWith(std::string(str), std::string(prefix));
}

bool km_string_contains(const char* str, const char* substr) {
    if (!str || !substr) return false;
    return KittyUtils::String::contains(std::string(str), std::string(substr));
}

bool km_string_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    return KittyUtils::String::endsWith(std::string(str), std::string(suffix));
}

char* km_string_trim(const char* str) {
    if (!str) return nullptr;
    std::string trimmed(str);
    KittyUtils::String::trim(trimmed);
    char* result = (char*)malloc(trimmed.length() + 1);
    if (result) {
        strcpy(result, trimmed.c_str());
    }
    return result;
}

bool km_string_validate_hex(const char* hex) {
    if (!hex) return false;
    std::string hex_str(hex);
    return KittyUtils::String::validateHex(hex_str);
}

char* km_string_fmt(const char* fmt, ...) {
    if (!fmt) return nullptr;
    va_list args;
    va_start(args, fmt);
    std::string result = KittyUtils::String::fmt(fmt);
    va_end(args);
    char* c_result = (char*)malloc(result.length() + 1);
    if (c_result) {
        strcpy(c_result, result.c_str());
    }
    return c_result;
}

char* km_string_random(size_t length) {
    std::string random_str = KittyUtils::randomString(length);
    char* result = (char*)malloc(random_str.length() + 1);
    if (result) {
        strcpy(result, random_str.c_str());
    }
    return result;
}

km_io_file_t km_io_file_create(const char* file_path, int flags, int mode) {
    km_io_file_t result = {nullptr, 0, nullptr, 0, 0, 0};
    if (!file_path) return result;
    KittyIOFile* file = new KittyIOFile(std::string(file_path), flags, mode);
    result.handle = file;
    result.fd = file->fd();
    result.file_path = file->path().c_str();
    result.flags = file->flags();
    result.mode = file->mode();
    result.error = file->lastError();
    return result;
}

km_io_file_t km_io_file_create_simple(const char* file_path, int flags) {
    km_io_file_t result = {nullptr, 0, nullptr, 0, 0, 0};
    if (!file_path) return result;
    KittyIOFile* file = new KittyIOFile(std::string(file_path), flags);
    result.handle = file;
    result.fd = file->fd();
    result.file_path = file->path().c_str();
    result.flags = file->flags();
    result.mode = file->mode();
    result.error = file->lastError();
    return result;
}

bool km_io_file_open(km_io_file_t* file) {
    if (!file || !file->handle) return false;
    return ((KittyIOFile*)file->handle)->open();
}

bool km_io_file_close(km_io_file_t* file) {
    if (!file || !file->handle) return false;
    return ((KittyIOFile*)file->handle)->close();
}

ssize_t km_io_file_read(km_io_file_t* file, void* buffer, size_t len) {
    if (!file || !file->handle || !buffer) return -1;
    return ((KittyIOFile*)file->handle)->read(buffer, len);
}

ssize_t km_io_file_write(km_io_file_t* file, const void* buffer, size_t len) {
    if (!file || !file->handle || !buffer) return -1;
    return ((KittyIOFile*)file->handle)->write(buffer, len);
}

ssize_t km_io_file_read_offset(km_io_file_t* file, uintptr_t offset, void* buffer, size_t len) {
    if (!file || !file->handle || !buffer) return -1;
    return ((KittyIOFile*)file->handle)->pread(offset, buffer, len);
}

ssize_t km_io_file_write_offset(km_io_file_t* file, uintptr_t offset, const void* buffer, size_t len) {
    if (!file || !file->handle || !buffer) return -1;
    return ((KittyIOFile*)file->handle)->pwrite(offset, buffer, len);
}

bool km_io_file_exists(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.exists();
}

bool km_io_file_can_read(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.canRead();
}

bool km_io_file_can_write(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.canWrite();
}

bool km_io_file_can_execute(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.canExecute();
}

bool km_io_file_is_file(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.isFile();
}

bool km_io_file_delete(const char* file_path) {
    if (!file_path) return false;
    KittyIOFile file(std::string(file_path), 0);
    return file.remove();
}

bool km_io_file_read_to_string(const char* file_path, char** out_str) {
    if (!file_path || !out_str) return false;
    std::string str;
    if (!KittyIOFile::readFileToString(std::string(file_path), &str)) return false;
    *out_str = (char*)malloc(str.length() + 1);
    if (!*out_str) return false;
    strcpy(*out_str, str.c_str());
    return true;
}

bool km_io_file_read_to_buffer(const char* file_path, void** out_buffer, size_t* out_size) {
    if (!file_path || !out_buffer || !out_size) return false;
    std::vector<char> buf;
    if (!KittyIOFile::readFileToBuffer(std::string(file_path), &buf)) return false;
    *out_buffer = malloc(buf.size());
    if (!*out_buffer) return false;
    memcpy(*out_buffer, buf.data(), buf.size());
    *out_size = buf.size();
    return true;
}

bool km_io_file_write_to_file(km_io_file_t* file, uintptr_t offset, size_t len, const char* dest_path) {
    if (!file || !file->handle || !dest_path) return false;
    return ((KittyIOFile*)file->handle)->writeOffsetToFile(offset, len, std::string(dest_path));
}

bool km_io_file_write_to_file_simple(km_io_file_t* file, const char* dest_path) {
    if (!file || !file->handle || !dest_path) return false;
    return ((KittyIOFile*)file->handle)->writeToFile(std::string(dest_path));
}

bool km_io_file_write_to_fd(km_io_file_t* file, int fd) {
    if (!file || !file->handle) return false;
    return ((KittyIOFile*)file->handle)->writeToFd(fd);
}

bool km_io_file_copy(const char* src_path, const char* dst_path) {
    if (!src_path || !dst_path) return false;
    return KittyIOFile::copy(std::string(src_path), std::string(dst_path));
}

int km_io_file_last_error(km_io_file_t* file) {
    if (!file || !file->handle) return -1;
    return ((KittyIOFile*)file->handle)->lastError();
}

char* km_io_file_last_str_error(km_io_file_t* file) {
    if (!file || !file->handle) return nullptr;
    std::string error = ((KittyIOFile*)file->handle)->lastStrError();
    char* result = (char*)malloc(error.length() + 1);
    if (result) {
        strcpy(result, error.c_str());
    }
    return result;
}

void km_io_file_free(km_io_file_t* file) {
    if (file && file->handle) {
        delete (KittyIOFile*)file->handle;
        file->handle = nullptr;
    }
}

#ifdef __ANDROID__
size_t km_zip_list_files(const char* zip_path, km_zip_file_info_t** out_files) {
    if (!zip_path || !out_files) return 0;
    std::vector<KittyUtils::Zip::ZipEntryInfo> files = KittyUtils::Zip::listEntriesInZip(std::string(zip_path));
    if (files.empty()) {
        *out_files = nullptr;
        return 0;
    }

    *out_files = (km_zip_file_info_t*)malloc(files.size() * sizeof(km_zip_file_info_t));
    if (!*out_files) return 0;

    for (size_t i = 0; i < files.size(); ++i) {
        const auto& f = files[i];
        strncpy((*out_files)[i].file_name, f.fileName.c_str(), 255);
        (*out_files)[i].file_name[255] = '\0';
        (*out_files)[i].compressed_size = f.compressedSize;
        (*out_files)[i].uncompressed_size = f.uncompressedSize;
        (*out_files)[i].compression_method = f.compressionMethod;
        (*out_files)[i].crc32 = f.crc32;
        (*out_files)[i].mod_time = f.modTime;
        (*out_files)[i].mod_date = f.modDate;
        (*out_files)[i].data_offset = f.dataOffset;
    }

    return files.size();
}

km_zip_file_info_t km_zip_get_file_info_by_offset(const char* zip_path, uint64_t data_offset) {
    km_zip_file_info_t result = {0};
    if (!zip_path) return result;

    KittyUtils::Zip::ZipEntryInfo info;
    bool ok = KittyUtils::Zip::findEntryInfoByDataOffset(std::string(zip_path), data_offset, &info);
    if (!ok) return result;

    strncpy(result.file_name, info.fileName.c_str(), 255);
    result.file_name[255] = '\0';
    result.compressed_size = info.compressedSize;
    result.uncompressed_size = info.uncompressedSize;
    result.compression_method = info.compressionMethod;
    result.crc32 = info.crc32;
    result.mod_time = info.modTime;
    result.mod_date = info.modDate;
    result.data_offset = info.dataOffset;

    return result;
}

km_zip_file_mmap_t km_zip_mmap_file_by_offset(const char* zip_path, uint64_t data_offset) {
    km_zip_file_mmap_t result = {0};
    if (!zip_path) return result;

    KittyUtils::Zip::ZipEntryMMap mmap;
    bool ok = KittyUtils::Zip::mmapEntryByDataOffset(std::string(zip_path), data_offset, &mmap);
    if (!ok) return result;

    result.data = mmap.data;
    result.size = mmap.size;

    return result;
}

void km_zip_free_file_list(km_zip_file_info_t* files) {
    if (files) free(files);
}

void km_zip_free_mmap(km_zip_file_mmap_t* mmap) {
    if (mmap && mmap->data) {
        munmap(mmap->data, mmap->size);
        mmap->data = nullptr;
        mmap->size = 0;
    }
}
#endif

}