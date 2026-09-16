use std::ffi::{CStr,c_char,c_int};use std::sync::OnceLock;
use adblock::{Engine,lists::{FilterSet,ParseOptions},request::Request};
const LIST:&str=include_str!("../filters/easylist_general_block.txt");
struct LiteWaveAdblockEngine{engine:Engine,rules:usize}
static ENGINE:OnceLock<LiteWaveAdblockEngine>=OnceLock::new();
fn shared()->&'static LiteWaveAdblockEngine{ENGINE.get_or_init(||{let mut filters=FilterSet::new(false);filters.add_filter_list(LIST,ParseOptions::default());let rules=filters.filters.len();LiteWaveAdblockEngine{engine:Engine::from_filter_set(filters,true),rules}})}
fn s(p:*const c_char)->String{if p.is_null(){String::new()}else{unsafe{CStr::from_ptr(p).to_string_lossy().into_owned()}}}
#[no_mangle]pub extern "C" fn lw_adblock_engine_create()->*mut LiteWaveAdblockEngine{shared()as*const _ as*mut _}
#[no_mangle]pub extern "C" fn lw_adblock_engine_destroy(_: *mut LiteWaveAdblockEngine){}
#[no_mangle]pub extern "C" fn lw_adblock_engine_rule_count(_: *const LiteWaveAdblockEngine)->c_int{shared().rules as c_int}
#[no_mangle]pub extern "C" fn lw_adblock_engine_should_block(_: *const LiteWaveAdblockEngine,u:*const c_char,f:*const c_char,k:*const c_char)->c_int{match Request::new(&s(u),&s(f),&s(k)){Ok(r)=>shared().engine.check_network_request(&r).matched as c_int,Err(_)=>0}}