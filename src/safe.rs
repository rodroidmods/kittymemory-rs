use crate::sys;
use std::ffi::CString;
use std::marker::PhantomData;

pub type Address = usize;

pub fn mem_read<T: Sized>(address: Address) -> Result<T, &'static str> {
    unsafe {
        let mut value: T = std::mem::zeroed();
        let result = sys::km_mem_read(
            address as *const std::ffi::c_void,
            &mut value as *mut T as *mut std::ffi::c_void,
            std::mem::size_of::<T>(),
        );
        
        if result {
            Ok(value)
        } else {
            Err("Failed to read memory")
        }
    }
}

pub fn mem_read_bytes(address: Address, size: usize) -> Result<Vec<u8>, &'static str> {
    unsafe {
        let mut buffer = vec![0u8; size];
        let result = sys::km_mem_read(
            address as *const std::ffi::c_void,
            buffer.as_mut_ptr() as *mut std::ffi::c_void,
            size,
        );
        
        if result {
            Ok(buffer)
        } else {
            Err("Failed to read memory")
        }
    }
}

#[cfg(target_os = "android")]
pub fn mem_write<T: Sized>(address: Address, value: &T) -> Result<(), &'static str> {
    unsafe {
        let result = sys::km_mem_write(
            address as *mut std::ffi::c_void,
            value as *const T as *const std::ffi::c_void,
            std::mem::size_of::<T>(),
        );
        
        if result {
            Ok(())
        } else {
            Err("Failed to write memory")
        }
    }
}

#[cfg(target_os = "android")]
pub fn mem_write_bytes(address: Address, bytes: &[u8]) -> Result<(), &'static str> {
    unsafe {
        let result = sys::km_mem_write(
            address as *mut std::ffi::c_void,
            bytes.as_ptr() as *const std::ffi::c_void,
            bytes.len(),
        );
        
        if result {
            Ok(())
        } else {
            Err("Failed to write memory")
        }
    }
}

#[cfg(target_os = "android")]
pub fn mem_protect(address: Address, length: usize, protection: i32) -> Result<(), &'static str> {
    unsafe {
        let result = sys::km_mem_protect(
            address as *const std::ffi::c_void,
            length,
            protection,
        );
        
        if result == 0 {
            Ok(())
        } else {
            Err("Failed to change memory protection")
        }
    }
}

#[cfg(target_os = "android")]
pub fn get_process_name() -> Result<String, &'static str> {
    unsafe {
        let c_str_ptr = sys::km_get_process_name();
        if c_str_ptr.is_null() {
            return Err("Failed to get process name");
        }
        let c_str = std::ffi::CStr::from_ptr(c_str_ptr);
        let result = c_str.to_string_lossy().into_owned();
        libc::free(c_str_ptr as *mut std::ffi::c_void);
        Ok(result)
    }
}

#[cfg(target_os = "android")]
pub fn syscall_mem_read(address: Address, buffer: &mut [u8]) -> Result<usize, &'static str> {
    unsafe {
        let result = sys::km_syscall_mem_read(address, buffer.as_mut_ptr() as *mut std::ffi::c_void, buffer.len());
        if result > 0 {
            Ok(result)
        } else {
            Err("Syscall memory read failed")
        }
    }
}

#[cfg(target_os = "android")]
pub fn syscall_mem_write(address: Address, buffer: &[u8]) -> Result<usize, &'static str> {
    unsafe {
        let result = sys::km_syscall_mem_write(address, buffer.as_ptr() as *mut std::ffi::c_void, buffer.len());
        if result > 0 {
            Ok(result)
        } else {
            Err("Syscall memory write failed")
        }
    }
}

#[cfg(target_os = "android")]
pub fn dump_mem_to_disk(address: Address, size: usize, destination: &str) -> Result<(), &'static str> {
    unsafe {
        let c_dest = CString::new(destination).unwrap();
        let result = sys::km_dump_mem_to_disk(address, size, c_dest.as_ptr());
        if result {
            Ok(())
        } else {
            Err("Failed to dump memory to disk")
        }
    }
}

#[cfg(target_os = "android")]
pub fn dump_mem_file_to_disk(mem_file: &str, destination: &str) -> Result<(), &'static str> {
    unsafe {
        let c_mem_file = CString::new(mem_file).unwrap();
        let c_dest = CString::new(destination).unwrap();
        let result = sys::km_dump_mem_file_to_disk(c_mem_file.as_ptr(), c_dest.as_ptr());
        if result {
            Ok(())
        } else {
            Err("Failed to dump memory file to disk")
        }
    }
}

#[cfg(target_os = "android")]
pub fn get_android_version() -> i32 {
    unsafe { sys::km_get_android_version() }
}

#[cfg(target_os = "android")]
pub fn get_android_sdk() -> i32 {
    unsafe { sys::km_get_android_sdk() }
}

#[cfg(target_os = "android")]
pub fn get_external_storage() -> Result<String, &'static str> {
    unsafe {
        let c_str_ptr = sys::km_get_external_storage();
        if c_str_ptr.is_null() {
            return Err("Failed to get external storage");
        }
        let c_str = std::ffi::CStr::from_ptr(c_str_ptr);
        let result = c_str.to_string_lossy().into_owned();
        libc::free(c_str_ptr as *mut std::ffi::c_void);
        Ok(result)
    }
}

#[cfg(target_os = "ios")]
pub fn mem_write<T: Sized>(address: Address, value: &T) -> Result<(), &'static str> {
    unsafe {
        let result = sys::km_mem_write_ios(
            address as *mut std::ffi::c_void,
            value as *const T as *const std::ffi::c_void,
            std::mem::size_of::<T>(),
        );
        
        match result {
            sys::km_memory_status_t::KM_SUCCESS => Ok(()),
            sys::km_memory_status_t::KM_INV_ADDR => Err("Invalid address"),
            sys::km_memory_status_t::KM_INV_LEN => Err("Invalid length"),
            sys::km_memory_status_t::KM_INV_BUF => Err("Invalid buffer"),
            sys::km_memory_status_t::KM_ERR_PROT => Err("Protection error"),
            sys::km_memory_status_t::KM_ERR_GET_PAGEINFO => Err("Failed to get page info"),
            sys::km_memory_status_t::KM_ERR_VMWRITE => Err("VM write error"),
            _ => Err("Unknown error"),
        }
    }
}

#[cfg(target_os = "ios")]
pub fn mem_write_bytes(address: Address, bytes: &[u8]) -> Result<(), &'static str> {
    unsafe {
        let result = sys::km_mem_write_ios(
            address as *mut std::ffi::c_void,
            bytes.as_ptr() as *const std::ffi::c_void,
            bytes.len(),
        );
        
        match result {
            sys::km_memory_status_t::KM_SUCCESS => Ok(()),
            sys::km_memory_status_t::KM_INV_ADDR => Err("Invalid address"),
            sys::km_memory_status_t::KM_INV_LEN => Err("Invalid length"),
            sys::km_memory_status_t::KM_INV_BUF => Err("Invalid buffer"),
            sys::km_memory_status_t::KM_ERR_PROT => Err("Protection error"),
            sys::km_memory_status_t::KM_ERR_GET_PAGEINFO => Err("Failed to get page info"),
            sys::km_memory_status_t::KM_ERR_VMWRITE => Err("VM write error"),
            _ => Err("Unknown error"),
        }
    }
}

pub struct Patch {
    pub(crate) inner: sys::km_patch_t,
    _marker: PhantomData<*mut ()>,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy)]
pub enum AsmArch {
    ARM32 = 0,
    ARM64 = 1,
    X86 = 2,
    X86_64 = 3,
}

impl Patch {
    pub fn with_bytes(address: Address, bytes: &[u8]) -> Self {
        unsafe {
            let patch = sys::km_patch_create_bytes(
                address,
                bytes.as_ptr() as *const std::ffi::c_void,
                bytes.len(),
            );

            Self {
                inner: patch,
                _marker: PhantomData,
            }
        }
    }

    pub fn with_hex(address: Address, hex: &str) -> Result<Self, &'static str> {
        let hex_string = hex.replace(" ", "").replace("0x", "");

        if hex_string.len() % 2 != 0 {
            return Err("Hex string must have even length");
        }

        for c in hex_string.chars() {
            if !c.is_ascii_hexdigit() {
                return Err("Invalid hex character");
            }
        }

        unsafe {
            let c_hex = CString::new(hex_string).unwrap();
            let patch = sys::km_patch_create_hex(address, c_hex.as_ptr());

            Ok(Self {
                inner: patch,
                _marker: PhantomData,
            })
        }
    }

    #[cfg(target_os = "android")]
    pub fn with_hex_lib(lib_name: &str, offset: Address, hex: &str) -> Result<Self, &'static str> {
        let hex_string = hex.replace(" ", "").replace("0x", "");

        if hex_string.len() % 2 != 0 {
            return Err("Hex string must have even length");
        }

        for c in hex_string.chars() {
            if !c.is_ascii_hexdigit() {
                return Err("Invalid hex character");
            }
        }

        unsafe {
            let c_lib = CString::new(lib_name).unwrap();
            let c_hex = CString::new(hex_string).unwrap();
            let patch = sys::km_patch_create_hex_lib(c_lib.as_ptr(), offset, c_hex.as_ptr());

            if patch.valid {
                Ok(Self {
                    inner: patch,
                    _marker: PhantomData,
                })
            } else {
                Err("Failed to create patch - library not found or invalid offset")
            }
        }
    }

    #[cfg(target_os = "android")]
    pub fn with_bytes_lib(lib_name: &str, offset: Address, bytes: &[u8]) -> Result<Self, &'static str> {
        if bytes.is_empty() {
            return Err("Bytes cannot be empty");
        }

        unsafe {
            let c_lib = CString::new(lib_name).unwrap();
            let patch = sys::km_patch_create_bytes_lib(
                c_lib.as_ptr(),
                offset,
                bytes.as_ptr() as *const std::ffi::c_void,
                bytes.len(),
            );

            if patch.valid {
                Ok(Self {
                    inner: patch,
                    _marker: PhantomData,
                })
            } else {
                Err("Failed to create patch - library not found or invalid offset")
            }
        }
    }

    #[cfg(all(target_os = "android", feature = "keystone"))]
    pub fn with_asm_lib(
        lib_name: &str,
        offset: Address,
        arch: AsmArch,
        asm_code: &str,
    ) -> Result<Self, &'static str> {
        unsafe {
            let c_lib = CString::new(lib_name).unwrap();
            let c_asm = CString::new(asm_code).unwrap();
            let patch = sys::km_patch_create_asm_lib(
                c_lib.as_ptr(),
                offset,
                arch as u32,
                c_asm.as_ptr(),
            );

            if patch.valid {
                Ok(Self {
                    inner: patch,
                    _marker: PhantomData,
                })
            } else {
                Err("Failed to create assembly patch - library not found or invalid offset")
            }
        }
    }

    #[cfg(feature = "keystone")]
    pub fn with_asm(
        address: Address,
        arch: AsmArch,
        asm_code: &str,
        asm_address: Address,
    ) -> Result<Self, &'static str> {
        unsafe {
            let c_asm = CString::new(asm_code).unwrap();
            let patch = sys::km_patch_create_asm(
                address,
                arch as u32,
                c_asm.as_ptr(),
                asm_address,
            );

            if patch.valid {
                Ok(Self {
                    inner: patch,
                    _marker: PhantomData,
                })
            } else {
                Err("Failed to create assembly patch")
            }
        }
    }
    
    pub fn is_valid(&self) -> bool {
        self.inner.valid
    }
    
    pub fn size(&self) -> usize {
        self.inner.size
    }
    
    pub fn address(&self) -> Address {
        self.inner.address
    }
    
    pub fn modify(&mut self) -> Result<(), &'static str> {
        unsafe {
            let result = sys::km_patch_modify(&mut self.inner as *mut sys::km_patch_t);
            if result {
                Ok(())
            } else {
                Err("Failed to modify memory")
            }
        }
    }
    
    pub fn restore(&mut self) -> Result<(), &'static str> {
        unsafe {
            let result = sys::km_patch_restore(&mut self.inner as *mut sys::km_patch_t);
            if result {
                Ok(())
            } else {
                Err("Failed to restore memory")
            }
        }
    }

    pub fn get_curr_bytes(&self) -> Result<String, &'static str> {
        unsafe {
            let ptr = sys::km_patch_get_curr_bytes(&self.inner as *const sys::km_patch_t as *mut sys::km_patch_t);
            if ptr.is_null() {
                return Err("Failed to get current bytes");
            }
            let c_str = std::ffi::CStr::from_ptr(ptr);
            let result = c_str.to_string_lossy().into_owned();
            sys::km_free_string(ptr);
            Ok(result)
        }
    }

    pub fn get_orig_bytes(&self) -> Result<String, &'static str> {
        unsafe {
            let ptr = sys::km_patch_get_orig_bytes(&self.inner as *const sys::km_patch_t as *mut sys::km_patch_t);
            if ptr.is_null() {
                return Err("Failed to get original bytes");
            }
            let c_str = std::ffi::CStr::from_ptr(ptr);
            let result = c_str.to_string_lossy().into_owned();
            sys::km_free_string(ptr);
            Ok(result)
        }
    }

    pub fn get_patch_bytes(&self) -> Result<String, &'static str> {
        unsafe {
            let ptr = sys::km_patch_get_patch_bytes(&self.inner as *const sys::km_patch_t as *mut sys::km_patch_t);
            if ptr.is_null() {
                return Err("Failed to get patch bytes");
            }
            let c_str = std::ffi::CStr::from_ptr(ptr);
            let result = c_str.to_string_lossy().into_owned();
            sys::km_free_string(ptr);
            Ok(result)
        }
    }
}

impl Drop for Patch {
    fn drop(&mut self) {
        unsafe {
            sys::km_patch_free(&mut self.inner as *mut sys::km_patch_t);
        }
    }
}

unsafe impl Send for Patch {}
unsafe impl Sync for Patch {}

pub struct Backup {
    pub(crate) inner: sys::km_backup_t,
    _marker: PhantomData<*mut ()>,
}

impl Backup {
    pub fn create(address: Address, size: usize) -> Self {
        unsafe {
            let backup = sys::km_backup_create(address, size);
            
            Self {
                inner: backup,
                _marker: PhantomData,
            }
        }
    }
    
    pub fn is_valid(&self) -> bool {
        self.inner.valid
    }
    
    pub fn size(&self) -> usize {
        self.inner.size
    }
    
    pub fn address(&self) -> Address {
        self.inner.address
    }
    
    pub fn restore(&mut self) -> Result<(), &'static str> {
        unsafe {
            let result = sys::km_backup_restore(&mut self.inner as *mut sys::km_backup_t);
            if result {
                Ok(())
            } else {
                Err("Failed to restore memory")
            }
        }
    }

    pub fn get_curr_bytes(&self) -> Result<String, &'static str> {
        unsafe {
            let ptr = sys::km_backup_get_curr_bytes(&self.inner as *const sys::km_backup_t as *mut sys::km_backup_t);
            if ptr.is_null() {
                return Err("Failed to get current bytes");
            }
            let c_str = std::ffi::CStr::from_ptr(ptr);
            let result = c_str.to_string_lossy().into_owned();
            sys::km_free_string(ptr);
            Ok(result)
        }
    }

    pub fn get_orig_bytes(&self) -> Result<String, &'static str> {
        unsafe {
            let ptr = sys::km_backup_get_orig_bytes(&self.inner as *const sys::km_backup_t as *mut sys::km_backup_t);
            if ptr.is_null() {
                return Err("Failed to get original bytes");
            }
            let c_str = std::ffi::CStr::from_ptr(ptr);
            let result = c_str.to_string_lossy().into_owned();
            sys::km_free_string(ptr);
            Ok(result)
        }
    }
}

impl Drop for Backup {
    fn drop(&mut self) {
        unsafe {
            sys::km_backup_free(&mut self.inner as *mut sys::km_backup_t);
        }
    }
}

unsafe impl Send for Backup {}
unsafe impl Sync for Backup {}

pub fn find_bytes_first(start: Address, end: Address, bytes: &[u8], mask: &str) -> Option<Address> {
    unsafe {
        let c_mask = CString::new(mask).unwrap();
        let result = sys::km_find_bytes_first(
            start,
            end,
            bytes.as_ptr() as *const std::os::raw::c_char,
            c_mask.as_ptr(),
        );
        
        if result == 0 {
            None
        } else {
            Some(result)
        }
    }
}

pub fn find_hex_first(start: Address, end: Address, hex: &str, mask: &str) -> Option<Address> {
    unsafe {
        let c_hex = CString::new(hex).unwrap();
        let c_mask = CString::new(mask).unwrap();
        let result = sys::km_find_hex_first(start, end, c_hex.as_ptr(), c_mask.as_ptr());
        
        if result == 0 {
            None
        } else {
            Some(result)
        }
    }
}

pub fn find_pattern_first(start: Address, end: Address, pattern: &str) -> Option<Address> {
    unsafe {
        let c_pattern = CString::new(pattern).unwrap();
        let result = sys::km_find_pattern_first(start, end, c_pattern.as_ptr());
        
        if result == 0 {
            None
        } else {
            Some(result)
        }
    }
}

pub fn find_data_first<T: Sized>(start: Address, end: Address, data: &T) -> Option<Address> {
    unsafe {
        let result = sys::km_find_data_first(
            start,
            end,
            data as *const T as *const std::ffi::c_void,
            std::mem::size_of::<T>(),
        );

        if result == 0 {
            None
        } else {
            Some(result)
        }
    }
}

pub fn find_bytes_all(start: Address, end: Address, bytes: &[u8], mask: &str) -> Vec<Address> {
    unsafe {
        let c_mask = CString::new(mask).unwrap();
        let mut results_ptr: *mut usize = std::ptr::null_mut();
        let count = sys::km_find_bytes_all(
            start,
            end,
            bytes.as_ptr() as *const std::os::raw::c_char,
            c_mask.as_ptr(),
            &mut results_ptr as *mut *mut usize,
        );

        if count == 0 || results_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(results_ptr, count);
        let vec = slice.to_vec();
        sys::km_free_results(results_ptr);
        vec
    }
}

pub fn find_hex_all(start: Address, end: Address, hex: &str, mask: &str) -> Vec<Address> {
    unsafe {
        let c_hex = CString::new(hex).unwrap();
        let c_mask = CString::new(mask).unwrap();
        let mut results_ptr: *mut usize = std::ptr::null_mut();
        let count = sys::km_find_hex_all(
            start,
            end,
            c_hex.as_ptr(),
            c_mask.as_ptr(),
            &mut results_ptr as *mut *mut usize,
        );

        if count == 0 || results_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(results_ptr, count);
        let vec = slice.to_vec();
        sys::km_free_results(results_ptr);
        vec
    }
}

pub fn find_pattern_all(start: Address, end: Address, pattern: &str) -> Vec<Address> {
    unsafe {
        let c_pattern = CString::new(pattern).unwrap();
        let mut results_ptr: *mut usize = std::ptr::null_mut();
        let count = sys::km_find_pattern_all(
            start,
            end,
            c_pattern.as_ptr(),
            &mut results_ptr as *mut *mut usize,
        );

        if count == 0 || results_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(results_ptr, count);
        let vec = slice.to_vec();
        sys::km_free_results(results_ptr);
        vec
    }
}

pub fn find_data_all<T: Sized>(start: Address, end: Address, data: &T) -> Vec<Address> {
    unsafe {
        let mut results_ptr: *mut usize = std::ptr::null_mut();
        let count = sys::km_find_data_all(
            start,
            end,
            data as *const T as *const std::ffi::c_void,
            std::mem::size_of::<T>(),
            &mut results_ptr as *mut *mut usize,
        );

        if count == 0 || results_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(results_ptr, count);
        let vec = slice.to_vec();
        sys::km_free_results(results_ptr);
        vec
    }
}

#[cfg(target_os = "android")]
pub struct ElfScanner {
    inner: sys::km_elf_scanner_t,
}

#[cfg(target_os = "android")]
impl ElfScanner {
    pub fn with_base(base: Address) -> Self {
        unsafe {
            let scanner = sys::km_elf_scanner_create(base);
            Self { inner: scanner }
        }
    }

    pub fn get_program() -> Self {
        unsafe {
            let scanner = sys::km_elf_scanner_get_program();
            Self { inner: scanner }
        }
    }

    pub fn find(path: &str) -> Option<Self> {
        unsafe {
            let c_path = CString::new(path).unwrap();
            let scanner = sys::km_elf_scanner_find(c_path.as_ptr());

            if scanner.valid {
                Some(Self { inner: scanner })
            } else {
                None
            }
        }
    }

    pub fn is_valid(&self) -> bool {
        self.inner.valid
    }

    pub fn find_symbol(&self, symbol: &str) -> Option<Address> {
        if !self.is_valid() {
            return None;
        }

        unsafe {
            let c_symbol = CString::new(symbol).unwrap();
            let result = sys::km_elf_find_symbol(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
                c_symbol.as_ptr(),
            );

            if result == 0 {
                None
            } else {
                Some(result)
            }
        }
    }

    pub fn find_debug_symbol(&self, symbol: &str) -> Option<Address> {
        if !self.is_valid() {
            return None;
        }

        unsafe {
            let c_symbol = CString::new(symbol).unwrap();
            let result = sys::km_elf_find_debug_symbol(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
                c_symbol.as_ptr(),
            );

            if result == 0 {
                None
            } else {
                Some(result)
            }
        }
    }

    pub fn base(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_base(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn end(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_end(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn load_bias(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_load_bias(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn load_size(&self) -> usize {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_load_size(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn path(&self) -> Option<String> {
        if !self.is_valid() {
            return None;
        }
        unsafe {
            let c_path = sys::km_elf_get_path(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            );
            if c_path.is_null() {
                None
            } else {
                let c_str = std::ffi::CStr::from_ptr(c_path);
                let result = c_str.to_string_lossy().into_owned();
                sys::km_free_string(c_path);
                Some(result)
            }
        }
    }

    pub fn is_zipped(&self) -> bool {
        if !self.is_valid() {
            return false;
        }
        unsafe {
            sys::km_elf_is_zipped(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn is_native(&self) -> bool {
        if !self.is_valid() {
            return false;
        }
        unsafe {
            sys::km_elf_is_native(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn is_emulated(&self) -> bool {
        if !self.is_valid() {
            return false;
        }
        unsafe {
            sys::km_elf_is_emulated(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn dump_to_disk(&self, destination: &str) -> Result<(), &'static str> {
        if !self.is_valid() {
            return Err("Invalid scanner");
        }
        unsafe {
            let c_dest = CString::new(destination).unwrap();
            let result = sys::km_elf_dump_to_disk(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
                c_dest.as_ptr(),
            );
            if result {
                Ok(())
            } else {
                Err("Failed to dump ELF")
            }
        }
    }

    pub fn phdr(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_phdr(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn loads(&self) -> i32 {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_loads(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn dynamic(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_dynamic(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn string_table(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_string_table(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn symbol_table(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_symbol_table(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn string_table_size(&self) -> usize {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_string_table_size(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn symbol_entry_size(&self) -> usize {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_symbol_entry_size(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn elf_hash_table(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_elf_hash_table(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn gnu_hash_table(&self) -> Address {
        if !self.is_valid() {
            return 0;
        }
        unsafe {
            sys::km_elf_get_gnu_hash_table(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn file_path(&self) -> Option<String> {
        if !self.is_valid() {
            return None;
        }
        unsafe {
            let c_path = sys::km_elf_get_file_path(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            );
            if c_path.is_null() {
                None
            } else {
                let c_str = std::ffi::CStr::from_ptr(c_path);
                let result = c_str.to_string_lossy().into_owned();
                sys::km_free_string(c_path);
                Some(result)
            }
        }
    }

    pub fn real_path(&self) -> Option<String> {
        if !self.is_valid() {
            return None;
        }
        unsafe {
            let c_path = sys::km_elf_get_real_path(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            );
            if c_path.is_null() {
                None
            } else {
                let c_str = std::ffi::CStr::from_ptr(c_path);
                let result = c_str.to_string_lossy().into_owned();
                sys::km_free_string(c_path);
                Some(result)
            }
        }
    }

    pub fn is_fixed_by_soinfo(&self) -> bool {
        if !self.is_valid() {
            return false;
        }
        unsafe {
            sys::km_elf_is_fixed_by_soinfo(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
            )
        }
    }

    pub fn refresh(&mut self) {
        if self.is_valid() {
            unsafe {
                sys::km_elf_refresh(&mut self.inner as *mut sys::km_elf_scanner_t);
            }
        }
    }
}

#[cfg(target_os = "android")]
impl ElfScanner {
    pub fn find_register_native(&self, name: &str, signature: &str) -> Option<RegisterNativeFn> {
        if !self.is_valid() {
            return None;
        }

        unsafe {
            let c_name = CString::new(name).unwrap();
            let c_sig = CString::new(signature).unwrap();
            let mut result: sys::km_register_native_fn_t = std::mem::zeroed();

            let success = sys::km_elf_find_register_native(
                &self.inner as *const sys::km_elf_scanner_t as *mut sys::km_elf_scanner_t,
                c_name.as_ptr(),
                c_sig.as_ptr(),
                &mut result as *mut sys::km_register_native_fn_t,
            );

            if success {
                Some(RegisterNativeFn {
                    name: std::ffi::CStr::from_ptr(result.name.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    signature: std::ffi::CStr::from_ptr(result.signature.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    fn_ptr: result.fnPtr,
                })
            } else {
                None
            }
        }
    }
}

#[cfg(target_os = "android")]
impl Drop for ElfScanner {
    fn drop(&mut self) {
        unsafe {
            sys::km_elf_scanner_free(&mut self.inner as *mut sys::km_elf_scanner_t);
        }
    }
}

#[cfg(target_os = "android")]
#[derive(Debug, Clone)]
pub struct RegisterNativeFn {
    pub name: String,
    pub signature: String,
    pub fn_ptr: Address,
}

#[cfg(target_os = "android")]
#[derive(Debug, Clone)]
pub struct SoInfo {
    pub base: Address,
    pub size: usize,
    pub phdr: Address,
    pub phnum: u32,
    pub dynamic: Address,
    pub strtab: Address,
    pub symtab: Address,
    pub strsz: usize,
    pub bias: Address,
    pub next: Address,
    pub e_machine: u32,
    pub path: String,
    pub realpath: String,
}

#[cfg(target_os = "android")]
pub struct LinkerScanner {
    inner: sys::km_linker_scanner_t,
}

#[cfg(target_os = "android")]
impl LinkerScanner {
    pub fn get() -> Self {
        unsafe {
            let scanner = sys::km_linker_scanner_get();
            Self { inner: scanner }
        }
    }

    pub fn is_valid(&self) -> bool {
        self.inner.valid
    }

    pub fn solist(&self) -> Address {
        unsafe {
            sys::km_linker_solist(&self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t)
        }
    }

    pub fn somain(&self) -> Address {
        unsafe {
            sys::km_linker_somain(&self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t)
        }
    }

    pub fn sonext(&self) -> Address {
        unsafe {
            sys::km_linker_sonext(&self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t)
        }
    }

    pub fn all_soinfo(&self) -> Vec<SoInfo> {
        unsafe {
            let mut infos_ptr: *mut sys::km_soinfo_t = std::ptr::null_mut();
            let count = sys::km_linker_all_soinfo(
                &self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t,
                &mut infos_ptr as *mut *mut sys::km_soinfo_t,
            );

            if count == 0 || infos_ptr.is_null() {
                return Vec::new();
            }

            let slice = std::slice::from_raw_parts(infos_ptr, count);
            let vec: Vec<SoInfo> = slice
                .iter()
                .map(|si| SoInfo {
                    base: si.base,
                    size: si.size,
                    phdr: si.phdr,
                    phnum: si.phnum,
                    dynamic: si.dyn_,
                    strtab: si.strtab,
                    symtab: si.symtab,
                    strsz: si.strsz,
                    bias: si.bias,
                    next: si.next,
                    e_machine: si.e_machine,
                    path: std::ffi::CStr::from_ptr(si.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(si.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
                .collect();

            sys::km_free_soinfos(infos_ptr);
            vec
        }
    }

    pub fn find_soinfo(&self, name: &str) -> Option<SoInfo> {
        unsafe {
            let c_name = CString::new(name).unwrap();
            let mut info: sys::km_soinfo_t = std::mem::zeroed();

            let success = sys::km_linker_find_soinfo(
                &self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t,
                c_name.as_ptr(),
                &mut info as *mut sys::km_soinfo_t,
            );

            if success {
                Some(SoInfo {
                    base: info.base,
                    size: info.size,
                    phdr: info.phdr,
                    phnum: info.phnum,
                    dynamic: info.dyn_,
                    strtab: info.strtab,
                    symtab: info.symtab,
                    strsz: info.strsz,
                    bias: info.bias,
                    next: info.next,
                    e_machine: info.e_machine,
                    path: std::ffi::CStr::from_ptr(info.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(info.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
            } else {
                None
            }
        }
    }

    pub fn somain_info(&self) -> Option<SoInfo> {
        unsafe {
            let mut info: sys::km_soinfo_t = std::mem::zeroed();
            let success = sys::km_linker_get_somain_info(
                &self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t,
                &mut info as *mut sys::km_soinfo_t,
            );

            if success {
                Some(SoInfo {
                    base: info.base,
                    size: info.size,
                    phdr: info.phdr,
                    phnum: info.phnum,
                    dynamic: info.dyn_,
                    strtab: info.strtab,
                    symtab: info.symtab,
                    strsz: info.strsz,
                    bias: info.bias,
                    next: info.next,
                    e_machine: info.e_machine,
                    path: std::ffi::CStr::from_ptr(info.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(info.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
            } else {
                None
            }
        }
    }

    pub fn sonext_info(&self) -> Option<SoInfo> {
        unsafe {
            let mut info: sys::km_soinfo_t = std::mem::zeroed();
            let success = sys::km_linker_get_sonext_info(
                &self.inner as *const sys::km_linker_scanner_t as *mut sys::km_linker_scanner_t,
                &mut info as *mut sys::km_soinfo_t,
            );

            if success {
                Some(SoInfo {
                    base: info.base,
                    size: info.size,
                    phdr: info.phdr,
                    phnum: info.phnum,
                    dynamic: info.dyn_,
                    strtab: info.strtab,
                    symtab: info.symtab,
                    strsz: info.strsz,
                    bias: info.bias,
                    next: info.next,
                    e_machine: info.e_machine,
                    path: std::ffi::CStr::from_ptr(info.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(info.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
            } else {
                None
            }
        }
    }
}

#[cfg(target_os = "android")]
impl Drop for LinkerScanner {
    fn drop(&mut self) {
        unsafe {
            sys::km_linker_scanner_free(&mut self.inner as *mut sys::km_linker_scanner_t);
        }
    }
}

#[cfg(target_os = "android")]
#[derive(Debug, Clone)]
pub struct ProcMap {
    pub start_address: Address,
    pub end_address: Address,
    pub length: usize,
    pub protection: String,
    pub readable: bool,
    pub writeable: bool,
    pub executable: bool,
    pub is_private: bool,
    pub is_shared: bool,
    pub is_ro: bool,
    pub is_rw: bool,
    pub is_rx: bool,
    pub offset: usize,
    pub dev: String,
    pub inode: u64,
    pub pathname: String,
}

#[cfg(target_os = "android")]
impl ProcMap {
    pub fn is_valid(&self) -> bool {
        self.start_address != 0 && self.end_address != 0 && self.length != 0
    }

    pub fn is_unknown(&self) -> bool {
        self.pathname.is_empty()
    }

    pub fn is_valid_elf(&self) -> bool {
        if !self.is_valid() || self.length <= 4 || !self.readable {
            return false;
        }
        unsafe {
            let magic = std::slice::from_raw_parts(self.start_address as *const u8, 4);
            magic == b"\x7fELF"
        }
    }

    pub fn contains(&self, address: Address) -> bool {
        address >= self.start_address && address < self.end_address
    }

    pub fn to_string(&self) -> String {
        format!(
            "{:x}-{:x} {} {:x} {} {} {}",
            self.start_address,
            self.end_address,
            self.protection,
            self.offset,
            self.dev,
            self.inode,
            self.pathname
        )
    }
}

#[cfg(target_os = "android")]
#[repr(i32)]
pub enum ProcMapFilter {
    Equal = 0,
    Contains = 1,
    StartWith = 2,
    EndWith = 3,
}

#[cfg(target_os = "android")]
pub fn get_all_maps() -> Vec<ProcMap> {
    unsafe {
        let mut maps_ptr: *mut sys::km_proc_map_t = std::ptr::null_mut();
        let count = sys::km_get_all_maps(&mut maps_ptr as *mut *mut sys::km_proc_map_t);

        if count == 0 || maps_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(maps_ptr, count);
        let vec: Vec<ProcMap> = slice
            .iter()
            .map(|m| ProcMap {
                start_address: m.startAddress,
                end_address: m.endAddress,
                length: m.length,
                protection: std::ffi::CStr::from_ptr(m.protection.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                readable: m.readable,
                writeable: m.writeable,
                executable: m.executable,
                is_private: m.is_private,
                is_shared: m.is_shared,
                is_ro: m.is_ro,
                is_rw: m.is_rw,
                is_rx: m.is_rx,
                offset: m.offset,
                dev: std::ffi::CStr::from_ptr(m.dev.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                inode: m.inode,
                pathname: std::ffi::CStr::from_ptr(m.pathname.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
            })
            .collect();

        sys::km_free_maps(maps_ptr);
        vec
    }
}

#[cfg(target_os = "android")]
pub fn get_maps_filtered(name: &str, filter: ProcMapFilter) -> Vec<ProcMap> {
    unsafe {
        let c_name = CString::new(name).unwrap();
        let mut maps_ptr: *mut sys::km_proc_map_t = std::ptr::null_mut();
        let count = sys::km_get_maps_filtered(
            c_name.as_ptr(),
            filter as i32,
            &mut maps_ptr as *mut *mut sys::km_proc_map_t,
        );

        if count == 0 || maps_ptr.is_null() {
            return Vec::new();
        }

        let slice = std::slice::from_raw_parts(maps_ptr, count);
        let vec: Vec<ProcMap> = slice
            .iter()
            .map(|m| ProcMap {
                start_address: m.startAddress,
                end_address: m.endAddress,
                length: m.length,
                protection: std::ffi::CStr::from_ptr(m.protection.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                readable: m.readable,
                writeable: m.writeable,
                executable: m.executable,
                is_private: m.is_private,
                is_shared: m.is_shared,
                is_ro: m.is_ro,
                is_rw: m.is_rw,
                is_rx: m.is_rx,
                offset: m.offset,
                dev: std::ffi::CStr::from_ptr(m.dev.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                inode: m.inode,
                pathname: std::ffi::CStr::from_ptr(m.pathname.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
            })
            .collect();

        sys::km_free_maps(maps_ptr);
        vec
    }
}

#[cfg(target_os = "android")]
pub fn get_address_map(address: Address) -> Option<ProcMap> {
    unsafe {
        let mut map: sys::km_proc_map_t = std::mem::zeroed();
        let result = sys::km_get_address_map(address, &mut map as *mut sys::km_proc_map_t);

        if result {
            Some(ProcMap {
                start_address: map.startAddress,
                end_address: map.endAddress,
                length: map.length,
                protection: std::ffi::CStr::from_ptr(map.protection.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                readable: map.readable,
                writeable: map.writeable,
                executable: map.executable,
                is_private: map.is_private,
                is_shared: map.is_shared,
                is_ro: map.is_ro,
                is_rw: map.is_rw,
                is_rx: map.is_rx,
                offset: map.offset,
                dev: std::ffi::CStr::from_ptr(map.dev.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
                inode: map.inode,
                pathname: std::ffi::CStr::from_ptr(map.pathname.as_ptr())
                    .to_string_lossy()
                    .into_owned(),
            })
        } else {
            None
        }
    }
}

#[cfg(target_os = "android")]
pub struct NativeBridgeScanner {
    inner: sys::km_native_bridge_scanner_t,
}

#[cfg(target_os = "android")]
impl NativeBridgeScanner {
    pub fn get() -> Self {
        unsafe {
            let scanner = sys::km_native_bridge_scanner_get();
            Self { inner: scanner }
        }
    }

    pub fn is_valid(&self) -> bool {
        unsafe { sys::km_native_bridge_scanner_is_valid(&self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t) }
    }

    pub fn sodl(&self) -> Address {
        unsafe {
            sys::km_native_bridge_scanner_sodl(&self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t)
        }
    }

    pub fn sodl_info(&self) -> Option<SoInfo> {
        unsafe {
            let mut info: sys::km_soinfo_t = std::mem::zeroed();
            let success = sys::km_native_bridge_scanner_get_sodl_info(
                &self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t,
                &mut info as *mut sys::km_soinfo_t,
            );

            if success {
                Some(SoInfo {
                    base: info.base,
                    size: info.size,
                    phdr: info.phdr,
                    phnum: info.phnum,
                    dynamic: info.dyn_,
                    strtab: info.strtab,
                    symtab: info.symtab,
                    strsz: info.strsz,
                    bias: info.bias,
                    next: info.next,
                    e_machine: info.e_machine,
                    path: std::ffi::CStr::from_ptr(info.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(info.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
            } else {
                None
            }
        }
    }

    pub fn all_soinfo(&self) -> Vec<SoInfo> {
        unsafe {
            let mut infos_ptr: *mut sys::km_soinfo_t = std::ptr::null_mut();
            let count = sys::km_native_bridge_scanner_all_soinfo(
                &self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t,
                &mut infos_ptr as *mut *mut sys::km_soinfo_t,
            );

            if count == 0 || infos_ptr.is_null() {
                return Vec::new();
            }

            let slice = std::slice::from_raw_parts(infos_ptr, count);
            let vec: Vec<SoInfo> = slice
                .iter()
                .map(|si| SoInfo {
                    base: si.base,
                    size: si.size,
                    phdr: si.phdr,
                    phnum: si.phnum,
                    dynamic: si.dyn_,
                    strtab: si.strtab,
                    symtab: si.symtab,
                    strsz: si.strsz,
                    bias: si.bias,
                    next: si.next,
                    e_machine: si.e_machine,
                    path: std::ffi::CStr::from_ptr(si.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(si.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
                .collect();

            sys::km_free_soinfos(infos_ptr);
            vec
        }
    }

    pub fn find_soinfo(&self, name: &str) -> Option<SoInfo> {
        unsafe {
            let c_name = CString::new(name).unwrap();
            let mut info: sys::km_soinfo_t = std::mem::zeroed();

            let success = sys::km_native_bridge_scanner_find_soinfo(
                &self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t,
                c_name.as_ptr(),
                &mut info as *mut sys::km_soinfo_t,
            );

            if success {
                Some(SoInfo {
                    base: info.base,
                    size: info.size,
                    phdr: info.phdr,
                    phnum: info.phnum,
                    dynamic: info.dyn_,
                    strtab: info.strtab,
                    symtab: info.symtab,
                    strsz: info.strsz,
                    bias: info.bias,
                    next: info.next,
                    e_machine: info.e_machine,
                    path: std::ffi::CStr::from_ptr(info.path.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    realpath: std::ffi::CStr::from_ptr(info.realpath.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                })
            } else {
                None
            }
        }
    }

    pub fn is_houdini(&self) -> bool {
        unsafe {
            sys::km_native_bridge_scanner_is_houdini(&self.inner as *const sys::km_native_bridge_scanner_t as *mut sys::km_native_bridge_scanner_t)
        }
    }
}

#[cfg(target_os = "android")]
impl Drop for NativeBridgeScanner {
    fn drop(&mut self) {
        unsafe {
            sys::km_native_bridge_scanner_free(&mut self.inner as *mut sys::km_native_bridge_scanner_t);
        }
    }
}

#[cfg(target_os = "ios")]
#[derive(Debug, Clone)]
pub struct SegmentData {
    pub start: Address,
    pub end: Address,
    pub size: usize,
}

#[cfg(target_os = "ios")]
pub struct MemoryFileInfo {
    inner: sys::km_memory_file_info_t,
}

#[cfg(target_os = "ios")]
impl MemoryFileInfo {
    pub fn get_base_info() -> Self {
        unsafe {
            let info = sys::km_get_base_info();
            Self { inner: info }
        }
    }

    pub fn get_file_info(file_name: &str) -> Option<Self> {
        unsafe {
            let c_name = CString::new(file_name).unwrap();
            let info = sys::km_get_memory_file_info(c_name.as_ptr());

            if info.handle.is_null() {
                None
            } else {
                Some(Self { inner: info })
            }
        }
    }

    pub fn index(&self) -> u32 {
        self.inner.index
    }

    pub fn name(&self) -> &str {
        unsafe {
            if self.inner.name.is_null() {
                ""
            } else {
                std::ffi::CStr::from_ptr(self.inner.name)
                    .to_str()
                    .unwrap_or("")
            }
        }
    }

    pub fn address(&self) -> Address {
        self.inner.address as Address
    }

    pub fn get_segment(&self, seg_name: &str) -> SegmentData {
        unsafe {
            let c_seg = CString::new(seg_name).unwrap();
            let seg = sys::km_get_segment(
                &self.inner as *const sys::km_memory_file_info_t as *mut sys::km_memory_file_info_t,
                c_seg.as_ptr(),
            );

            SegmentData {
                start: seg.start,
                end: seg.end,
                size: seg.size,
            }
        }
    }

    pub fn get_section(&self, seg_name: &str, sect_name: &str) -> SegmentData {
        unsafe {
            let c_seg = CString::new(seg_name).unwrap();
            let c_sect = CString::new(sect_name).unwrap();
            let sect = sys::km_get_section(
                &self.inner as *const sys::km_memory_file_info_t as *mut sys::km_memory_file_info_t,
                c_seg.as_ptr(),
                c_sect.as_ptr(),
            );

            SegmentData {
                start: sect.start,
                end: sect.end,
                size: sect.size,
            }
        }
    }

    pub fn find_symbol(&self, symbol: &str) -> Option<Address> {
        unsafe {
            let c_symbol = CString::new(symbol).unwrap();
            let result = sys::km_find_symbol_in_file(
                &self.inner as *const sys::km_memory_file_info_t as *mut sys::km_memory_file_info_t,
                c_symbol.as_ptr(),
            );

            if result == 0 {
                None
            } else {
                Some(result)
            }
        }
    }
}

#[cfg(target_os = "ios")]
impl Drop for MemoryFileInfo {
    fn drop(&mut self) {
        unsafe {
            sys::km_free_memory_file_info(&mut self.inner as *mut sys::km_memory_file_info_t);
        }
    }
}

#[cfg(target_os = "ios")]
pub fn get_absolute_address(file_name: Option<&str>, address: Address) -> Address {
    unsafe {
        if let Some(name) = file_name {
            let c_name = CString::new(name).unwrap();
            sys::km_get_absolute_address(c_name.as_ptr(), address)
        } else {
            sys::km_get_absolute_address(std::ptr::null(), address)
        }
    }
}

#[cfg(target_os = "ios")]
pub fn find_symbol_in_lib(lib: &str, symbol: &str) -> Option<Address> {
    unsafe {
        let c_lib = CString::new(lib).unwrap();
        let c_symbol = CString::new(symbol).unwrap();
        let result = sys::km_find_symbol_in_lib(c_lib.as_ptr(), c_symbol.as_ptr());

        if result == 0 {
            None
        } else {
            Some(result)
        }
    }
}

pub struct PtrValidator {
    inner: sys::km_ptr_validator_t,
}

impl PtrValidator {
    pub fn new() -> Self {
        unsafe {
            let validator = sys::km_validator_create();
            Self { inner: validator }
        }
    }
    
    pub fn set_use_cache(&mut self, use_cache: bool) {
        unsafe {
            sys::km_validator_set_cache(&mut self.inner as *mut sys::km_ptr_validator_t, use_cache);
        }
    }
    
    pub fn is_ptr_readable(&mut self, ptr: Address, len: usize) -> bool {
        unsafe {
            sys::km_validator_is_readable(&mut self.inner as *mut sys::km_ptr_validator_t, ptr, len)
        }
    }
    
    pub fn is_ptr_writable(&mut self, ptr: Address, len: usize) -> bool {
        unsafe {
            sys::km_validator_is_writable(&mut self.inner as *mut sys::km_ptr_validator_t, ptr, len)
        }
    }
    
    pub fn is_ptr_executable(&mut self, ptr: Address, len: usize) -> bool {
        unsafe {
            sys::km_validator_is_executable(&mut self.inner as *mut sys::km_ptr_validator_t, ptr, len)
        }
    }
}

impl Default for PtrValidator {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for PtrValidator {
    fn drop(&mut self) {
        unsafe {
            sys::km_validator_free(&mut self.inner as *mut sys::km_ptr_validator_t);
        }
    }
}

unsafe impl Send for PtrValidator {}
unsafe impl Sync for PtrValidator {}

pub fn page_start(x: Address) -> Address {
    sys::km_page_start_fn(x)
}

pub fn page_end(x: Address) -> Address {
    sys::km_page_end_fn(x)
}

pub fn data_to_hex(data: &[u8]) -> String {
    unsafe {
        let hex_ptr = sys::km_data_to_hex(data.as_ptr() as *const std::ffi::c_void, data.len());
        if hex_ptr.is_null() {
            return String::new();
        }
        let c_str = std::ffi::CStr::from_ptr(hex_ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(hex_ptr);
        result
    }
}

pub fn hex_to_data(hex: &str) -> Result<Vec<u8>, &'static str> {
    let hex_clean = hex.replace(" ", "").replace("0x", "");

    if hex_clean.len() % 2 != 0 {
        return Err("Hex string must have even length");
    }

    let data_len = hex_clean.len() / 2;
    let mut data = vec![0u8; data_len];

    unsafe {
        let c_hex = CString::new(hex_clean).unwrap();
        let success = sys::km_hex_to_data(
            c_hex.as_ptr(),
            data.as_mut_ptr() as *mut std::ffi::c_void,
            data_len,
        );

        if success {
            Ok(data)
        } else {
            Err("Invalid hex string")
        }
    }
}

pub fn hex_dump(address: Address, len: usize) -> String {
    unsafe {
        let dump_ptr = sys::km_hex_dump(address as *const std::ffi::c_void, len);
        if dump_ptr.is_null() {
            return String::new();
        }
        let c_str = std::ffi::CStr::from_ptr(dump_ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(dump_ptr);
        result
    }
}

pub fn file_name_from_path(file_path: &str) -> Result<String, &'static str> {
    unsafe {
        let c_path = CString::new(file_path).unwrap();
        let ptr = sys::km_file_name_from_path(c_path.as_ptr());
        if ptr.is_null() {
            return Err("Failed to get file name");
        }
        let c_str = std::ffi::CStr::from_ptr(ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(ptr);
        Ok(result)
    }
}

pub fn file_directory(file_path: &str) -> Result<String, &'static str> {
    unsafe {
        let c_path = CString::new(file_path).unwrap();
        let ptr = sys::km_file_directory(c_path.as_ptr());
        if ptr.is_null() {
            return Err("Failed to get file directory");
        }
        let c_str = std::ffi::CStr::from_ptr(ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(ptr);
        Ok(result)
    }
}

pub fn file_extension(file_path: &str) -> Result<String, &'static str> {
    unsafe {
        let c_path = CString::new(file_path).unwrap();
        let ptr = sys::km_file_extension(c_path.as_ptr());
        if ptr.is_null() {
            return Err("Failed to get file extension");
        }
        let c_str = std::ffi::CStr::from_ptr(ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(ptr);
        Ok(result)
    }
}

pub fn string_starts_with(str: &str, prefix: &str) -> bool {
    unsafe {
        let c_str = CString::new(str).unwrap();
        let c_prefix = CString::new(prefix).unwrap();
        sys::km_string_starts_with(c_str.as_ptr(), c_prefix.as_ptr())
    }
}

pub fn string_contains(str: &str, substr: &str) -> bool {
    unsafe {
        let c_str = CString::new(str).unwrap();
        let c_substr = CString::new(substr).unwrap();
        sys::km_string_contains(c_str.as_ptr(), c_substr.as_ptr())
    }
}

pub fn string_ends_with(str: &str, suffix: &str) -> bool {
    unsafe {
        let c_str = CString::new(str).unwrap();
        let c_suffix = CString::new(suffix).unwrap();
        sys::km_string_ends_with(c_str.as_ptr(), c_suffix.as_ptr())
    }
}

pub fn string_trim(str: &str) -> Result<String, &'static str> {
    unsafe {
        let c_str = CString::new(str).unwrap();
        let ptr = sys::km_string_trim(c_str.as_ptr());
        if ptr.is_null() {
            return Err("Failed to trim string");
        }
        let c_result = std::ffi::CStr::from_ptr(ptr);
        let result = c_result.to_string_lossy().into_owned();
        sys::km_free_string(ptr);
        Ok(result)
    }
}

pub fn string_validate_hex(hex: &str) -> bool {
    unsafe {
        let c_hex = CString::new(hex).unwrap();
        sys::km_string_validate_hex(c_hex.as_ptr())
    }
}

pub fn string_random(length: usize) -> Result<String, &'static str> {
    unsafe {
        let ptr = sys::km_string_random(length);
        if ptr.is_null() {
            return Err("Failed to generate random string");
        }
        let c_str = std::ffi::CStr::from_ptr(ptr);
        let result = c_str.to_string_lossy().into_owned();
        sys::km_free_string(ptr);
        Ok(result)
    }
}
