(() => {
    'use strict';

    // Runs in an isolated Qt WebEngine application world.  It deliberately
    // does not replace fetch/XHR, alter video timing, mute media, or rewrite
    // site responses: those tricks caused legitimate videos and web apps to
    // fail. Network blocking is handled by the native interceptor.
    if (globalThis.__litewaveShieldStop) globalThis.__litewaveShieldStop();
    document.getElementById('litewave-ad-style')?.remove();
    if (!globalThis.__litewaveShieldEnabled) return;
    if (location.protocol !== 'https:' && location.protocol !== 'http:') return;
    if (location.hostname === 'litewave.home') return;

    let active = true;
    let timer = 0;
    let observer = null;
    // C++ enables this script only for YouTube after checking the top-level
    // navigation URL. Keep the web-page script URL-agnostic: Qt's setHtml and
    // internal pages can expose a temporary data: URL during document creation.
    const isYouTube = true;

    const style = document.createElement('style');
    style.id = 'litewave-ad-style';
    style.textContent = [
        'ytd-ad-slot-renderer, ytd-display-ad-renderer,',
        'ytd-promoted-sparkles-web-renderer, ytd-promoted-video-renderer,',
        'ytd-in-feed-ad-layout-renderer, ytd-companion-slot-renderer,',
        'ytd-banner-promo-renderer, ytd-statement-banner-renderer,',
        '.ytd-action-companion-ad-renderer, #player-ads, .video-ads,',
        '.ytp-ad-overlay-container, .ytp-ad-message-container,',
        '[data-litewave-sponsored="1"] {display:none!important;}'
    ].join('\n');
    const target = document.head || document.documentElement || document.body;
    if (target) target.appendChild(style);

    function isVisible(button) {
        return !!button && !button.disabled &&
            (typeof button.getClientRects !== 'function' || button.getClientRects().length > 0);
    }

    function cleanYouTube() {
        timer = 0;
        if (!active || document.hidden || !isYouTube) return;

        // Hide only explicit YouTube sponsored tiles. Do not remove ordinary
        // recommendations, player controls, or video elements.
        document.querySelectorAll(
            'ytd-ad-slot-renderer, ytd-in-feed-ad-layout-renderer, ' +
            'ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer'
        ).forEach(ad => {
            const card = typeof ad.closest === 'function'
                ? ad.closest('ytd-rich-item-renderer, ytd-video-renderer')
                : null;
            if (card) card.setAttribute('data-litewave-sponsored', '1');
        });

        // Clicking a visible first-party Skip control is reversible and leaves
        // playback, volume, requests, and response objects untouched.
        document.querySelectorAll(
            '.ytp-ad-skip-button, .ytp-ad-skip-button-modern, ' +
            '.ytp-skip-ad-button, button.ytp-ad-skip-button-single, ' +
            'button[id^="skip-button"]'
        ).forEach(button => {
            if (isVisible(button)) {
                try { button.click(); } catch (_) {}
            }
        });
    }

    function schedule() {
        if (active && !document.hidden && isYouTube && !timer)
            timer = setTimeout(cleanYouTube, 250);
    }

    function watch() {
        if (!active || !isYouTube || document.hidden) return;
        if (!observer) observer = new MutationObserver(schedule);
        observer.observe(document.documentElement, { childList: true, subtree: true });
        schedule();
    }

    function visibilityChanged() {
        if (document.hidden) {
            observer?.disconnect();
            clearTimeout(timer);
            timer = 0;
        } else {
            watch();
        }
    }

    function pageHide(event) {
        observer?.disconnect();
        clearTimeout(timer);
        timer = 0;
        if (!event.persisted) stop();
    }

    function pageShow() { watch(); }

    function stop() {
        active = false;
        observer?.disconnect();
        clearTimeout(timer);
        style.remove();
        document.querySelectorAll('[data-litewave-sponsored="1"]').forEach(
            node => node.removeAttribute('data-litewave-sponsored'));
        document.removeEventListener('visibilitychange', visibilityChanged);
        document.removeEventListener('yt-navigate-finish', schedule);
        window.removeEventListener('pagehide', pageHide);
        window.removeEventListener('pageshow', pageShow);
        globalThis.__litewaveShieldStop = null;
    }

    globalThis.__litewaveShieldStop = stop;
    document.addEventListener('visibilitychange', visibilityChanged);
    document.addEventListener('yt-navigate-finish', schedule);
    window.addEventListener('pagehide', pageHide);
    window.addEventListener('pageshow', pageShow);
    watch();
})();
