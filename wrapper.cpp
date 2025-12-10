#include <cstring>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "wrapper.h"
#include "KittyMemory/KittyMemory.hpp"
#include "KittyMemory/MemoryPatch.hpp"
#include "KittyMemory/MemoryBackup.hpp"
#include "KittyMemory/KittyScanner.hpp"
#include "KittyMemory/KittyPtrValidator.hpp"

static std::unordered_map<uintptr_t, MemoryPatch> g_patches;
static std::unordered_map<uintptr_t, MemoryBackup> g_backups;
static std::mutex g_patches_mutex;
static std::mutex g_backups_mutex;
static uintptr_t g_next_patch_id = 1;
static uintptr_t g_next_backup_id = 1;

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

uintptr_t km_elf_find_debug_symbol(km_elf_scanner_t* scanner, const char* symbol) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->findDebugSymbol(std::string(symbol));
}

uintptr_t km_elf_get_base(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->base();
}

uintptr_t km_elf_get_end(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->end();
}

uintptr_t km_elf_get_load_bias(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->loadBias();
}

size_t km_elf_get_load_size(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->loadSize();
}

const char* km_elf_get_path(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return nullptr;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->realPath().c_str();
}

bool km_elf_is_zipped(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->isZipped();
}

bool km_elf_is_native(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->isNative();
}

bool km_elf_is_emulated(km_elf_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->isEmulated();
}

bool km_elf_dump_to_disk(km_elf_scanner_t* scanner, const char* destination) {
    if (!scanner || !scanner->valid || !scanner->handle) return false;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    return elf->dumpToDisk(std::string(destination));
}

size_t km_get_all_maps(km_proc_map_t** maps) {
    std::vector<ProcMap> all_maps = KittyMemory::getAllMaps();
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
        strncpy((*maps)[i].protection, pm.protection.c_str(), 4);
        (*maps)[i].protection[4] = '\0';
        (*maps)[i].readable = pm.readable;
        (*maps)[i].writeable = pm.writeable;
        (*maps)[i].executable = pm.executable;
        (*maps)[i].is_private = pm.is_private;
        (*maps)[i].is_shared = pm.is_shared;
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
    std::vector<ProcMap> filtered_maps = KittyMemory::getMaps(
        static_cast<EProcMapFilter>(filter),
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
        strncpy((*maps)[i].protection, pm.protection.c_str(), 4);
        (*maps)[i].protection[4] = '\0';
        (*maps)[i].readable = pm.readable;
        (*maps)[i].writeable = pm.writeable;
        (*maps)[i].executable = pm.executable;
        (*maps)[i].is_private = pm.is_private;
        (*maps)[i].is_shared = pm.is_shared;
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

    ProcMap pm = KittyMemory::getAddressMap((const void*)address);
    if (!pm.isValid()) return false;

    map->startAddress = pm.startAddress;
    map->endAddress = pm.endAddress;
    map->length = pm.length;
    strncpy(map->protection, pm.protection.c_str(), 4);
    map->protection[4] = '\0';
    map->readable = pm.readable;
    map->writeable = pm.writeable;
    map->executable = pm.executable;
    map->is_private = pm.is_private;
    map->is_shared = pm.is_shared;
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

bool km_elf_find_register_native(km_elf_scanner_t* scanner, const char* name, const char* signature, km_register_native_fn_t* result) {
    if (!scanner || !scanner->valid || !scanner->handle || !result) return false;

    KittyScanner::ElfScanner* elf = (KittyScanner::ElfScanner*)scanner->handle;
    RegisterNativeFn fn = elf->findRegisterNativeFn(std::string(name), std::string(signature));

    if (fn.fnPtr == 0) return false;

    strncpy(result->name, fn.name.c_str(), 127);
    result->name[127] = '\0';
    strncpy(result->signature, fn.signature.c_str(), 255);
    result->signature[255] = '\0';
    result->fnPtr = fn.fnPtr;

    return true;
}

km_linker_scanner_t km_linker_scanner_get(void) {
    km_linker_scanner_t result = {0};
    KittyScanner::LinkerScanner* scanner = new KittyScanner::LinkerScanner(KittyScanner::LinkerScanner::Get());
    result.handle = scanner;
    result.valid = scanner->isValid();
    return result;
}

uintptr_t km_linker_solist(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    KittyScanner::LinkerScanner* linker = (KittyScanner::LinkerScanner*)scanner->handle;
    return linker->solist();
}

uintptr_t km_linker_somain(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    KittyScanner::LinkerScanner* linker = (KittyScanner::LinkerScanner*)scanner->handle;
    return linker->somain();
}

uintptr_t km_linker_sonext(km_linker_scanner_t* scanner) {
    if (!scanner || !scanner->valid || !scanner->handle) return 0;
    KittyScanner::LinkerScanner* linker = (KittyScanner::LinkerScanner*)scanner->handle;
    return linker->sonext();
}

size_t km_linker_all_soinfo(km_linker_scanner_t* scanner, km_soinfo_t** infos) {
    if (!scanner || !scanner->valid || !scanner->handle) {
        *infos = nullptr;
        return 0;
    }

    KittyScanner::LinkerScanner* linker = (KittyScanner::LinkerScanner*)scanner->handle;
    std::vector<kitty_soinfo_t> all_info = linker->allSoInfo();

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

    KittyScanner::LinkerScanner* linker = (KittyScanner::LinkerScanner*)scanner->handle;
    kitty_soinfo_t si = linker->findSoInfo(std::string(name));

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

void km_linker_scanner_free(km_linker_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        delete (KittyScanner::LinkerScanner*)scanner->handle;
        scanner->handle = nullptr;
    }
}

void km_elf_scanner_free(km_elf_scanner_t* scanner) {
    if (scanner && scanner->handle) {
        delete (KittyScanner::ElfScanner*)scanner->handle;
        scanner->handle = nullptr;
    }
}
#endif

#ifdef __APPLE__
static std::unordered_map<uintptr_t, KittyMemory::MemoryFileInfo> g_memory_file_infos;
static std::mutex g_file_info_mutex;
static uintptr_t g_next_file_info_id = 1;

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
    std::string hex = KittyUtils::data2Hex(data, len);
    char* result = (char*)malloc(hex.length() + 1);
    if (result) {
        strcpy(result, hex.c_str());
    }
    return result;
}

bool km_hex_to_data(const char* hex, void* data, size_t data_len) {
    if (!hex || !data) return false;

    std::string hex_str(hex);
    if (!KittyUtils::String::ValidateHex(hex_str)) return false;

    size_t expected_len = hex_str.length() / 2;
    if (expected_len != data_len) return false;

    KittyUtils::dataFromHex(hex_str, data);
    return true;
}

char* km_hex_dump(const void* address, size_t len) {
    std::string dump = KittyUtils::HexDump<16, true>(address, len);
    char* result = (char*)malloc(dump.length() + 1);
    if (result) {
        strcpy(result, dump.c_str());
    }
    return result;
}

void km_free_string(char* str) {
    if (str) free(str);
}

}
