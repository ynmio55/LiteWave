const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync('assets/shield.js', 'utf8');

function fixture(host = 'www.youtube.com') {
    const state = { timers: new Map(), next: 0, style: null, observers: [], clicks: 0, ad: true };
    const events = new Map();
    const global = { __litewaveShieldEnabled: true };
    const style = { remove() { if (state.style === this) state.style = null; } };
    const visible = { disabled: false, getClientRects: () => [1], click: () => state.clicks++ };
    const hidden = { disabled: false, getClientRects: () => [], click: () => { throw Error('hidden click'); } };
    const disabled = { disabled: true, getClientRects: () => [1], click: () => { throw Error('disabled click'); } };
    const player = {
        classList: { contains: () => state.ad },
        querySelectorAll: () => [visible, hidden, disabled]
    };
    const doc = {
        hidden: false,
        head: { appendChild(node) { state.style = node; } },
        documentElement: {},
        createElement(name) { assert.equal(name, 'style'); return { ...style }; },
        getElementById() { return state.style; },
        querySelector(selector) {
            if (selector.includes('#movie_player')) return player;
            if (selector === 'video') return { muted: false, playbackRate: 1, duration: 10, currentTime: 0 };
            return null;
        },
        querySelectorAll: (selector) => {
            if (selector.includes('skip') || selector.includes('.ytp-')) return [visible];
            return [];
        },
        addEventListener(name, fn) { events.set(name, fn); },
        removeEventListener(name) { events.delete(name); }
    };
    const win = {
        addEventListener(name, fn) { events.set(name, fn); },
        removeEventListener(name) { events.delete(name); }
    };
    function Observer(callback) {
        this.callback = callback;
        this.connected = false;
        this.observe = () => { this.connected = true; };
        this.disconnect = () => { this.connected = false; };
        state.observers.push(this);
    }
    const run = new Function('globalThis','document','location','window','MutationObserver',
        'setTimeout','clearTimeout','getComputedStyle', source);
    const invoke = () => run(global, doc, { protocol: 'https:', hostname: host }, win, Observer,
        fn => { const id = ++state.next; state.timers.set(id, fn); return id; },
        id => state.timers.delete(id), () => ({ visibility: 'visible' }));
    const flush = () => { const pending = [...state.timers.values()]; state.timers.clear(); pending.forEach(fn => fn()); };
    return { state, global, doc, events, invoke, flush };
}
const nonYT = fixture('example.org');
nonYT.invoke();
assert(nonYT.state.style);
assert.equal(nonYT.state.timers.size, 0);
assert.equal(nonYT.state.observers.length, 0);
nonYT.global.__litewaveShieldEnabled = false;
nonYT.invoke();
assert.equal(nonYT.state.style, null);
assert.equal(nonYT.events.size, 0);

const yt = fixture();
yt.invoke();
assert.equal(yt.state.timers.size, 1);
yt.flush();
assert(yt.state.clicks >= 1);
yt.state.ad = false;
yt.events.get('yt-navigate-finish')();
yt.flush();
assert(yt.state.clicks >= 1);
yt.doc.hidden = true;
yt.events.get('visibilitychange')();
assert(yt.state.observers.every(o => !o.connected));
assert.equal(yt.state.timers.size, 0);
yt.doc.hidden = false;
yt.events.get('visibilitychange')();
assert.equal(yt.state.timers.size, 1);
// Reinjection disposes of the prior observer and timeout.
yt.invoke();
assert.equal(yt.state.observers.filter(o => o.connected).length, 1);
assert.equal(yt.state.timers.size, 1);
yt.events.get('pagehide')({ persisted: true });
assert.equal(yt.state.timers.size, 0);
yt.events.get('pageshow')();
assert.equal(yt.state.timers.size, 1);
yt.global.__litewaveShieldEnabled = false;
yt.invoke();
assert.equal(yt.state.style, null);
assert.equal(yt.state.timers.size, 0);
assert.equal(yt.events.size, 0);
assert(yt.state.observers.every(o => !o.connected));
console.log('PASS: cosmetic lifecycle, site scope, skip controls, hidden tabs, YouTube ad fast-skipper');
