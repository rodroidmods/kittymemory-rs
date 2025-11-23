#include <cstring>
#include <cstdlib>
#include <functional>
#include "wrapper.h"
#include "KittyMemory/KittyMemory.hpp"
#include "KittyMemory/MemoryPatch.hpp"
#include "KittyMemory/MemoryBackup.hpp"
#include "KittyMemory/KittyScanner.hpp"
#include "KittyMemory/KittyPtrValidator.hpp"

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
#endif

#ifdef __APPLE__
km_memory_status_t km_mem_write_ios(void* address, const void* buffer, size_t len) {
    return (km_memory_status_t)KittyMemory::memWrite(address, buffer, len);
}
#endif

km_patch_t km_patch_create_bytes(uintptr_t address, const void* bytes, size_t size) {
    km_patch_t result = {0};
    
    MemoryPatch patch = MemoryPatch::createWithBytes(address, bytes, size);
    
    result.address = patch.get_TargetAddress();
    result.size = patch.get_PatchSize();
    result.valid = patch.isValid();
    
    if (result.valid) {
        std::string orig = patch.get_OrigBytes();
        std::string patch_bytes = patch.get_PatchBytes();
        
        result.orig_bytes = malloc(orig.length() + 1);
        result.patch_bytes = malloc(patch_bytes.length() + 1);
        
        if (result.orig_bytes && result.patch_bytes) {
            strcpy((char*)result.orig_bytes, orig.c_str());
            strcpy((char*)result.patch_bytes, patch_bytes.c_str());
        }
    }
    
    return result;
}

km_patch_t km_patch_create_hex(uintptr_t address, const char* hex) {
    km_patch_t result = {0};
    
    MemoryPatch patch = MemoryPatch::createWithHex(address, std::string(hex));
    
    result.address = patch.get_TargetAddress();
    result.size = patch.get_PatchSize();
    result.valid = patch.isValid();
    
    if (result.valid) {
        std::string orig = patch.get_OrigBytes();
        std::string patch_bytes = patch.get_PatchBytes();
        
        result.orig_bytes = malloc(orig.length() + 1);
        result.patch_bytes = malloc(patch_bytes.length() + 1);
        
        if (result.orig_bytes && result.patch_bytes) {
            strcpy((char*)result.orig_bytes, orig.c_str());
            strcpy((char*)result.patch_bytes, patch_bytes.c_str());
        }
    }
    
    return result;
}

bool km_patch_modify(km_patch_t* patch) {
    if (!patch || !patch->valid) return false;
    
    MemoryPatch mp = MemoryPatch::createWithBytes(patch->address, patch->patch_bytes, patch->size);
    return mp.Modify();
}

bool km_patch_restore(km_patch_t* patch) {
    if (!patch || !patch->valid) return false;
    
    MemoryPatch mp = MemoryPatch::createWithBytes(patch->address, patch->orig_bytes, patch->size);
    return mp.Restore();
}

void km_patch_free(km_patch_t* patch) {
    if (patch) {
        if (patch->orig_bytes) free(patch->orig_bytes);
        if (patch->patch_bytes) free(patch->patch_bytes);
        patch->orig_bytes = nullptr;
        patch->patch_bytes = nullptr;
    }
}

km_backup_t km_backup_create(uintptr_t address, size_t size) {
    km_backup_t result = {0};
    
    MemoryBackup backup = MemoryBackup::createBackup(address, size);
    
    result.address = backup.get_TargetAddress();
    result.size = backup.get_BackupSize();
    result.valid = backup.isValid();
    
    if (result.valid) {
        std::string orig = backup.get_OrigBytes();
        result.orig_bytes = malloc(orig.length() + 1);
        if (result.orig_bytes) {
            strcpy((char*)result.orig_bytes, orig.c_str());
        }
    }
    
    return result;
}

bool km_backup_restore(km_backup_t* backup) {
    if (!backup || !backup->valid) return false;
    
    MemoryBackup mb = MemoryBackup::createBackup(backup->address, backup->size);
    return mb.Restore();
}

void km_backup_free(km_backup_t* backup) {
    if (backup && backup->orig_bytes) {
        free(backup->orig_bytes);
        backup->orig_bytes = nullptr;
    }
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

#ifdef __ANDROID__
km_elf_scanner_t km_elf_scanner_create(uintptr_t base) {
    km_elf_scanner_t result = {0};
    KittyScanner::ElfScanner* scanner = new KittyScanner::ElfScanner(base, KittyMemory::getAllMaps());
    result.handle = scanner;
    result.valid = scanner->isValid();
    return result;
}

km_elf_scanner_t km_elf_scanner_get_program(void) {
    km_elf_scanner_t result = {0};
    KittyScanner::ElfScanner* scanner = new KittyScanner::ElfScanner(KittyScanner::ElfScanner::getProgramElf());
    result.handle = scanner;
    result.valid = scanner->isValid();
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
        KittyScanner::ElfScanner* scanner_ptr = new KittyScanner::ElfScanner(scanner);
        result.handle = scanner_ptr;
        result.valid = true;
    }
    
    return result;
}

uintptr_t km_elf_find_symbol(km_elf_scanner_t* scanner, const char* symbol) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    
    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->findSymbol(std::string(symbol));
}

void km_elf_scanner_free(km_elf_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        delete (KittyScanner::ElfScanner*)scanner->handle;
        scanner->handle = nullptr;
    }
}
#endif

#ifdef __APPLE__
uintptr_t km_find_symbol_in_lib(const char* lib, const char* symbol) {
    return KittyScanner::findSymbol(std::string(lib), std::string(symbol));
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

}
