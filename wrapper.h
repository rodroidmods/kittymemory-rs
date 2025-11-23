#ifndef KITTYMEMORY_WRAPPER_H
#define KITTYMEMORY_WRAPPER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool km_mem_read(const void* address, void* buffer, size_t len);

#ifdef __ANDROID__
bool km_mem_write(void* address, const void* buffer, size_t len);
int km_mem_protect(const void* address, size_t length, int protection);
char* km_get_process_name(void);
#endif

#ifdef __APPLE__
typedef enum {
    KM_SUCCESS = 1,
    KM_FAILED = 0,
    KM_INV_ADDR = 2,
    KM_INV_LEN = 3,
    KM_INV_BUF = 4,
    KM_ERR_PROT = 5,
    KM_ERR_GET_PAGEINFO = 6,
    KM_ERR_VMWRITE = 7
} km_memory_status_t;

km_memory_status_t km_mem_write_ios(void* address, const void* buffer, size_t len);
#endif

typedef struct {
    uintptr_t address;
    size_t size;
    void* orig_bytes;
    void* patch_bytes;
    bool valid;
} km_patch_t;

km_patch_t km_patch_create_bytes(uintptr_t address, const void* bytes, size_t size);
km_patch_t km_patch_create_hex(uintptr_t address, const char* hex);
bool km_patch_modify(km_patch_t* patch);
bool km_patch_restore(km_patch_t* patch);
void km_patch_free(km_patch_t* patch);

typedef struct {
    uintptr_t address;
    size_t size;
    void* orig_bytes;
    bool valid;
} km_backup_t;

km_backup_t km_backup_create(uintptr_t address, size_t size);
bool km_backup_restore(km_backup_t* backup);
void km_backup_free(km_backup_t* backup);

uintptr_t km_find_bytes_first(uintptr_t start, uintptr_t end, const char* bytes, const char* mask);
uintptr_t km_find_hex_first(uintptr_t start, uintptr_t end, const char* hex, const char* mask);
uintptr_t km_find_pattern_first(uintptr_t start, uintptr_t end, const char* pattern);
uintptr_t km_find_data_first(uintptr_t start, uintptr_t end, const void* data, size_t size);

typedef struct {
    void* handle;
    bool valid;
} km_elf_scanner_t;

#ifdef __ANDROID__
km_elf_scanner_t km_elf_scanner_create(uintptr_t base);
km_elf_scanner_t km_elf_scanner_get_program(void);
km_elf_scanner_t km_elf_scanner_find(const char* path);
uintptr_t km_elf_find_symbol(km_elf_scanner_t* scanner, const char* symbol);
void km_elf_scanner_free(km_elf_scanner_t* scanner);
#endif

#ifdef __APPLE__
uintptr_t km_find_symbol_in_lib(const char* lib, const char* symbol);
#endif

typedef struct {
    void* handle;
} km_ptr_validator_t;

km_ptr_validator_t km_validator_create(void);
void km_validator_set_cache(km_ptr_validator_t* validator, bool use_cache);
bool km_validator_is_readable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len);
bool km_validator_is_writable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len);
bool km_validator_is_executable(km_ptr_validator_t* validator, uintptr_t ptr, size_t len);
void km_validator_free(km_ptr_validator_t* validator);

#define KM_PAGE_SIZE 4096

static inline uintptr_t km_page_start(uintptr_t x) {
    return x & ~(KM_PAGE_SIZE - 1);
}

static inline uintptr_t km_page_end(uintptr_t x) {
    return km_page_start(x + KM_PAGE_SIZE - 1);
}

#ifdef __cplusplus
}
#endif

#endif
