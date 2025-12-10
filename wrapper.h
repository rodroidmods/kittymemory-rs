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
    void* handle;
    uintptr_t address;
    size_t size;
    bool valid;
} km_patch_t;

typedef enum {
    KM_ASM_ARM32 = 0,
    KM_ASM_ARM64 = 1,
    KM_ASM_X86 = 2,
    KM_ASM_X86_64 = 3
} km_asm_arch_t;

km_patch_t km_patch_create_bytes(uintptr_t address, const void* bytes, size_t size);
km_patch_t km_patch_create_hex(uintptr_t address, const char* hex);
km_patch_t km_patch_create_asm(uintptr_t address, km_asm_arch_t arch, const char* asm_code, uintptr_t asm_address);
bool km_patch_modify(km_patch_t* patch);
bool km_patch_restore(km_patch_t* patch);
void km_patch_free(km_patch_t* patch);

typedef struct {
    void* handle;
    uintptr_t address;
    size_t size;
    bool valid;
} km_backup_t;

km_backup_t km_backup_create(uintptr_t address, size_t size);
bool km_backup_restore(km_backup_t* backup);
void km_backup_free(km_backup_t* backup);

uintptr_t km_find_bytes_first(uintptr_t start, uintptr_t end, const char* bytes, const char* mask);
uintptr_t km_find_hex_first(uintptr_t start, uintptr_t end, const char* hex, const char* mask);
uintptr_t km_find_pattern_first(uintptr_t start, uintptr_t end, const char* pattern);
uintptr_t km_find_data_first(uintptr_t start, uintptr_t end, const void* data, size_t size);

size_t km_find_bytes_all(uintptr_t start, uintptr_t end, const char* bytes, const char* mask, uintptr_t** results);
size_t km_find_hex_all(uintptr_t start, uintptr_t end, const char* hex, const char* mask, uintptr_t** results);
size_t km_find_pattern_all(uintptr_t start, uintptr_t end, const char* pattern, uintptr_t** results);
size_t km_find_data_all(uintptr_t start, uintptr_t end, const void* data, size_t size, uintptr_t** results);
void km_free_results(uintptr_t* results);

typedef struct {
    void* handle;
    bool valid;
} km_elf_scanner_t;

#ifdef __ANDROID__
km_elf_scanner_t km_elf_scanner_create(uintptr_t base);
km_elf_scanner_t km_elf_scanner_get_program(void);
km_elf_scanner_t km_elf_scanner_find(const char* path);
uintptr_t km_elf_find_symbol(km_elf_scanner_t* scanner, const char* symbol);
uintptr_t km_elf_find_debug_symbol(km_elf_scanner_t* scanner, const char* symbol);
uintptr_t km_elf_get_base(km_elf_scanner_t* scanner);
uintptr_t km_elf_get_end(km_elf_scanner_t* scanner);
uintptr_t km_elf_get_load_bias(km_elf_scanner_t* scanner);
size_t km_elf_get_load_size(km_elf_scanner_t* scanner);
const char* km_elf_get_path(km_elf_scanner_t* scanner);
bool km_elf_is_zipped(km_elf_scanner_t* scanner);
bool km_elf_is_native(km_elf_scanner_t* scanner);
bool km_elf_is_emulated(km_elf_scanner_t* scanner);
bool km_elf_dump_to_disk(km_elf_scanner_t* scanner, const char* destination);
void km_elf_scanner_free(km_elf_scanner_t* scanner);

typedef struct {
    uintptr_t startAddress;
    uintptr_t endAddress;
    size_t length;
    char protection[5];
    bool readable;
    bool writeable;
    bool executable;
    bool is_private;
    bool is_shared;
    uintptr_t offset;
    char dev[16];
    unsigned long inode;
    char pathname[256];
} km_proc_map_t;

size_t km_get_all_maps(km_proc_map_t** maps);
size_t km_get_maps_filtered(const char* name, int filter, km_proc_map_t** maps);
bool km_get_address_map(uintptr_t address, km_proc_map_t* map);
void km_free_maps(km_proc_map_t* maps);

typedef struct {
    uintptr_t base;
    uintptr_t size;
    uintptr_t phdr;
    unsigned int phnum;
    uintptr_t dyn;
    uintptr_t strtab;
    uintptr_t symtab;
    size_t strsz;
    uintptr_t bias;
    uintptr_t next;
    unsigned int e_machine;
    char path[256];
    char realpath[256];
} km_soinfo_t;

typedef struct {
    void* handle;
    bool valid;
} km_linker_scanner_t;

km_linker_scanner_t km_linker_scanner_get(void);
uintptr_t km_linker_solist(km_linker_scanner_t* scanner);
uintptr_t km_linker_somain(km_linker_scanner_t* scanner);
uintptr_t km_linker_sonext(km_linker_scanner_t* scanner);
size_t km_linker_all_soinfo(km_linker_scanner_t* scanner, km_soinfo_t** infos);
bool km_linker_find_soinfo(km_linker_scanner_t* scanner, const char* name, km_soinfo_t* info);
void km_free_soinfos(km_soinfo_t* infos);
void km_linker_scanner_free(km_linker_scanner_t* scanner);

typedef struct {
    char name[128];
    char signature[256];
    uintptr_t fnPtr;
} km_register_native_fn_t;

bool km_elf_find_register_native(km_elf_scanner_t* scanner, const char* name, const char* signature, km_register_native_fn_t* result);
#endif

#ifdef __APPLE__
typedef struct {
    uintptr_t start;
    uintptr_t end;
    size_t size;
} km_segment_data_t;

typedef struct {
    void* handle;
    uint32_t index;
    const char* name;
    uintptr_t address;
} km_memory_file_info_t;

km_memory_file_info_t km_get_base_info(void);
km_memory_file_info_t km_get_memory_file_info(const char* file_name);
uintptr_t km_get_absolute_address(const char* file_name, uintptr_t address);
km_segment_data_t km_get_segment(km_memory_file_info_t* info, const char* seg_name);
km_segment_data_t km_get_section(km_memory_file_info_t* info, const char* seg_name, const char* sect_name);
uintptr_t km_find_symbol_in_lib(const char* lib, const char* symbol);
uintptr_t km_find_symbol_in_file(km_memory_file_info_t* info, const char* symbol);
void km_free_memory_file_info(km_memory_file_info_t* info);
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

char* km_data_to_hex(const void* data, size_t len);
bool km_hex_to_data(const char* hex, void* data, size_t data_len);
char* km_hex_dump(const void* address, size_t len);
void km_free_string(char* str);

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
