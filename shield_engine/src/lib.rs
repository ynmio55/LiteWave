use std::ffi::{CStr,c_char,c_int};use std::sync::OnceLock;
use adblock::{Engine,request::Request};
const LIST:&str=concat!(include_str!("../filters/easylist_general_block.txt"), "\n", include_str!("../filters/easylist_adservers.txt"));
struct LiteWaveAdblockEngine{engine:Engine,rules:usize}
static ENGINE:OnceLock<LiteWaveAdblockEngine>=OnceLock::new();
fn shared()->&'static LiteWaveAdblockEngine{ENGINE.get_or_init(||{let rules=LIST.lines().filter(|line|!line.is_empty()&&!line.starts_with('!')).count();LiteWaveAdblockEngine{engine:Engine::new_with_list_text(LIST.to_string()),rules}})}
fn s(p:*const c_char)->String{if p.is_null(){String::new()}else{unsafe{CStr::from_ptr(p).to_string_lossy().into_owned()}}}
#[no_mangle]pub extern "C" fn lw_adblock_engine_create()->*mut LiteWaveAdblockEngine{shared()as*const _ as*mut _}
#[no_mangle]pub extern "C" fn lw_adblock_engine_destroy(_: *mut LiteWaveAdblockEngine){}
#[no_mangle]pub extern "C" fn lw_adblock_engine_rule_count(_: *const LiteWaveAdblockEngine)->c_int{shared().rules as c_int}
#[no_mangle]pub extern "C" fn lw_adblock_engine_should_block(_: *const LiteWaveAdblockEngine,u:*const c_char,f:*const c_char,k:*const c_char)->c_int{match Request::new(&s(u),&s(f),&s(k),&s(f)){Ok(r)=>shared().engine.check_network_request(&r).filter.is_some() as c_int,Err(_)=>0}}