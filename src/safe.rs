use crate::sys;
use std::ffi::{CStr, CString};
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
        
        let c_str = CStr::from_ptr(c_str_ptr);
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
    inner: sys::km_patch_t,
    _marker: PhantomData<*mut ()>,
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
    inner: sys::km_backup_t,
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
}

#[cfg(target_os = "android")]
impl Drop for ElfScanner {
    fn drop(&mut self) {
        unsafe {
            sys::km_elf_scanner_free(&mut self.inner as *mut sys::km_elf_scanner_t);
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
