#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[inline]
pub fn km_page_start_fn(x: usize) -> usize {
    x & !(KM_PAGE_SIZE as usize - 1)
}

#[inline]
pub fn km_page_end_fn(x: usize) -> usize {
    km_page_start_fn(x + KM_PAGE_SIZE as usize - 1)
}
