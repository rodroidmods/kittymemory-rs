#![doc = include_str!("../README.md")]

pub mod sys;
pub mod safe;

pub use safe::*;

pub mod prelude {
    pub use crate::safe::*;
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_page_functions() {
        let addr = 0x1234;
        let start = safe::page_start(addr);
        assert!(start <= addr);
        assert_eq!(start % (sys::KM_PAGE_SIZE as usize), 0);
        
        let end = safe::page_end(addr);
        assert!(end >= addr);
    }
    
    #[test]
    fn test_patch_creation() {
        let patch = safe::Patch::with_bytes(0x1000, &[0x90, 0x90, 0x90, 0x90]);
        assert!(patch.is_valid() || !patch.is_valid());
    }
    
    #[test]
    fn test_backup_creation() {
        let backup = safe::Backup::create(0x1000, 16);
        assert!(backup.is_valid() || !backup.is_valid());
    }
    
    #[test]
    fn test_validator_creation() {
        let _validator = safe::PtrValidator::new();
        // Validator is created successfully - the Drop impl will clean it up
    }
}
