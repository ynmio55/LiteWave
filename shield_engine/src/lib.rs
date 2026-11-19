use std::collections::HashSet;
use std::ffi::{c_char, c_int, CStr, CString};
use std::sync::{Arc, Once, OnceLock, RwLock};

use adblock::lists::{FilterSet, ParseOptions};
use adblock::{request::Request, Engine};

const BUNDLED_ADS: &str = include_str!("../filters/easylist.txt");
const BUNDLED_PRIVACY: &str = include_str!("../filters/easyprivacy.txt");

/// Opaque handle kept for ABI compatibility; all state is process-wide so
/// every profile shares a single compiled rule set.
pub struct LiteWaveAdblockEngine {
    _private: u8,
}

struct Lists {
    ads: Engine,
    ads_rules: usize,
    privacy_text: String,
    privacy_rules: usize,
    // EasyPrivacy is only needed by Aggressive mode, so it is compiled on
    // demand to keep Standard mode's memory footprint small.
    privacy: OnceLock<Engine>,
}

static HANDLE: LiteWaveAdblockEngine = LiteWaveAdblockEngine { _private: 0 };
static STATE: RwLock<Option<Arc<Lists>>> = RwLock::new(None);
static WARMUP: Once = Once::new();

fn rule_count(text: &str) -> usize {
    text.lines()
        .map(str::trim)
        .filter(|l| !l.is_empty() && !l.starts_with('!') && !l.starts_with('['))
        .count()
}

fn compile(text: &str) -> Engine {
    let mut set = FilterSet::new(false);
    set.add_filter_list(text.to_owned(), ParseOptions::default());
    Engine::new_with_filter_set(set)
}

fn looks_like_list(text: &str) -> bool {
    text.len() > 10_000 && text.trim_start().starts_with("[Adblock Plus")
}

fn current() -> Option<Arc<Lists>> {
    STATE.read().ok().and_then(|s| s.clone())
}

fn install(ads_text: &str, privacy_text: &str, keep_privacy_hot: bool, only_if_empty: bool) -> usize {
    let lists = Lists {
        ads: compile(ads_text),
        ads_rules: rule_count(ads_text),
        privacy_text: privacy_text.to_owned(),
        privacy_rules: rule_count(privacy_text),
        privacy: OnceLock::new(),
    };
    if keep_privacy_hot {
        let _ = lists.privacy.set(compile(privacy_text));
    }
    let total = lists.ads_rules + lists.privacy_rules;
    if let Ok(mut state) = STATE.write() {
        // The bundled warm-up must never replace fresher lists that finished
        // compiling first.
        if !(only_if_empty && state.is_some()) {
            *state = Some(Arc::new(lists));
        }
    }
    total
}

fn s(p: *const c_char) -> String {
    if p.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() }
    }
}

fn owned(text: String) -> *mut c_char {
    CString::new(text.replace('\0', "")).map_or(std::ptr::null_mut(), CString::into_raw)
}

/// Returns the shared handle and starts compiling the bundled lists on a
/// background thread so browser startup is never blocked by rule parsing.
#[no_mangle]
pub extern "C" fn lw_adblock_engine_create() -> *mut LiteWaveAdblockEngine {
    WARMUP.call_once(|| {
        std::thread::spawn(|| {
            if current().is_none() {
                install(BUNDLED_ADS, BUNDLED_PRIVACY, false, true);
            }
        });
    });
    &HANDLE as *const _ as *mut _
}

#[no_mangle]
pub extern "C" fn lw_adblock_engine_destroy(_: *mut LiteWaveAdblockEngine) {}

/// Synchronously compiles and installs new list texts. Null or malformed
/// input falls back to the bundled snapshot. Returns the active rule count.
/// Intended to be called from a worker thread.
#[no_mangle]
pub extern "C" fn lw_adblock_engine_load_lists(
    ads: *const c_char,
    privacy: *const c_char,
) -> c_int {
    let ads = s(ads);
    let privacy = s(privacy);
    let ads = if looks_like_list(&ads) { ads.as_str() } else { BUNDLED_ADS };
    let privacy = if looks_like_list(&privacy) { privacy.as_str() } else { BUNDLED_PRIVACY };
    let hot = current().is_some_and(|l| l.privacy.get().is_some());
    install(ads, privacy, hot, false) as c_int
}

/// Compiles EasyPrivacy for Aggressive mode. Blocks; call from a worker.
#[no_mangle]
pub extern "C" fn lw_adblock_engine_prepare_privacy() {
    if current().is_none() {
        install(BUNDLED_ADS, BUNDLED_PRIVACY, true, true);
    }
    if let Some(lists) = current() {
        lists.privacy.get_or_init(|| compile(&lists.privacy_text));
    }
}

#[no_mangle]
pub extern "C" fn lw_adblock_engine_is_ready(_: *const LiteWaveAdblockEngine) -> c_int {
    current().is_some() as c_int
}

#[no_mangle]
pub extern "C" fn lw_adblock_engine_rule_count(_: *const LiteWaveAdblockEngine) -> c_int {
    current().map_or(0, |l| l.ads_rules + l.privacy_rules) as c_int
}

/// 1 = block, 2 = explicitly allowed by an `@@` exception, 0 = no rule
/// matched, -1 = rules are still compiling.
#[no_mangle]
pub extern "C" fn lw_adblock_engine_should_block(
    _: *const LiteWaveAdblockEngine,
    url: *const c_char,
    source: *const c_char,
    kind: *const c_char,
    aggressive: c_int,
) -> c_int {
    let Some(lists) = current() else { return -1 };
    let Ok(request) = Request::new(&s(url), &s(source), &s(kind), "get") else {
        return 0;
    };
    let verdict = |r: adblock::blocker::BlockerResult| {
        if r.should_block() {
            1
        } else if r.exception.is_some() {
            2
        } else {
            0
        }
    };
    let ads = verdict(lists.ads.check_network_request(&request));
    if ads != 0 || aggressive == 0 {
        return ads;
    }
    lists.privacy.get().map_or(0, |p| verdict(p.check_network_request(&request)))
}

/// Element-hiding selectors for `url`, honouring `#@#` exceptions and
/// `$generichide`. `classes` and `ids` are newline-separated values seen in
/// the page. Returns one selector per line; free with `lw_adblock_string_free`.
#[no_mangle]
pub extern "C" fn lw_adblock_engine_cosmetic_selectors(
    _: *const LiteWaveAdblockEngine,
    url: *const c_char,
    classes: *const c_char,
    ids: *const c_char,
) -> *mut c_char {
    let Some(lists) = current() else { return owned(String::new()) };
    let resources = lists.ads.url_cosmetic_resources(&s(url));
    let mut selectors: HashSet<String> = resources.hide_selectors;
    if !resources.generichide {
        let classes = s(classes);
        let ids = s(ids);
        selectors.extend(lists.ads.hidden_class_id_selectors(
            classes.lines().filter(|v| !v.is_empty()),
            ids.lines().filter(|v| !v.is_empty()),
            &resources.exceptions,
        ));
    }
    owned(selectors.into_iter().collect::<Vec<_>>().join("\n"))
}

#[no_mangle]
pub extern "C" fn lw_adblock_string_free(p: *mut c_char) {
    if !p.is_null() {
        unsafe { drop(CString::from_raw(p)) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn check(url: &str, source: &str, kind: &str, aggressive: bool) -> c_int {
        let u = CString::new(url).unwrap();
        let f = CString::new(source).unwrap();
        let k = CString::new(kind).unwrap();
        lw_adblock_engine_should_block(&HANDLE, u.as_ptr(), f.as_ptr(), k.as_ptr(), aggressive as c_int)
    }

    fn lists_verdict(list: &str, url: &str) -> c_int {
        let r = compile(list).check_network_request(
            &Request::new(url, "https://site.test/", "script", "get").unwrap());
        if r.should_block() { 1 } else if r.exception.is_some() { 2 } else { 0 }
    }

    #[test]
    fn bundled_lists_block_and_honour_exceptions() {
        let started = std::time::Instant::now();
        install(BUNDLED_ADS, BUNDLED_PRIVACY, true, false);
        eprintln!("compiled bundled lists in {:?}", started.elapsed());

        assert_eq!(check("https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js",
                         "https://example.com/", "script", false), 1);
        assert_eq!(check("https://www.google-analytics.com/analytics.js",
                         "https://example.com/", "script", false), 0);
        assert_eq!(check("https://www.google-analytics.com/analytics.js",
                         "https://example.com/", "script", true), 1);
        assert_eq!(check("https://cdn.example.com/app.js", "https://example.com/", "script", true), 0);
        assert_eq!(lists_verdict("||ads.example.net^\n@@||ads.example.net/allowed^",
                                 "https://ads.example.net/allowed/x.js"), 2);

        let mut set = FilterSet::new(false);
        set.add_filter_list("||ads.example.net^\n@@||ads.example.net/allowed^".to_owned(), ParseOptions::default());
        let engine = Engine::new_with_filter_set(set);
        let req = |u| Request::new(u, "https://site.test/", "script", "get").unwrap();
        assert!(engine.check_network_request(&req("https://ads.example.net/x.js")).should_block());
        assert!(!engine.check_network_request(&req("https://ads.example.net/allowed/x.js")).should_block());

        let u = CString::new("https://example.com/").unwrap();
        let c = CString::new("adsbygoogle\nnot-an-ad").unwrap();
        let i = CString::new("").unwrap();
        let p = lw_adblock_engine_cosmetic_selectors(&HANDLE, u.as_ptr(), c.as_ptr(), i.as_ptr());
        let css = unsafe { CStr::from_ptr(p) }.to_string_lossy().into_owned();
        lw_adblock_string_free(p);
        assert!(css.lines().any(|l| l.contains("adsbygoogle")), "{css}");
    }
}
