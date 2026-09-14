(() => {
    'use strict';
    // ApplicationWorld only. Config comes from native code, not the website.
    if (globalThis.__litewaveShieldStop) globalThis.__litewaveShieldStop();
    document.getElementById('litewave-ad-style')?.remove();
    if (!globalThis.__litewaveShieldEnabled) return;
    if (location.protocol !== 'https:' && location.protocol !== 'http:') return;
    if (location.hostname === 'litewave.home') return;

    let active = true;
    let timer = 0;
    let observer = null;
    const isYouTube = location.hostname === 'youtube.com' ||
        location.hostname.endsWith('.youtube.com');
    const style = document.createElement('style');
    style.id = 'litewave-ad-style';
    style.textContent = [
        '.adsbygoogle, [data-ad-client][data-ad-slot], [id^="div-gpt-ad-"],',
        '[id^="google_ads_iframe"], iframe[src*="//ad.doubleclick.net/"],',
        'iframe[src*="//ads.exoclick.com/"], iframe[src*="//syndication.exoclick.com/"]',
        '{display:none!important;}',
        isYouTube ? [
            'ytd-ad-slot-renderer, ytd-display-ad-renderer,',
            'ytd-promoted-sparkles-web-renderer, ytd-promoted-video-renderer,',
            'ytd-in-feed-ad-layout-renderer, ytd-companion-slot-renderer,',
            '[data-litewave-sponsored="1"] {display:none!important;}'
        ].join('\n') : ''
    ].join('\n');
    (document.head || document.documentElement).appendChild(style);

    function cleanYouTube() {
        timer = 0;
        if (!active || document.hidden) return;
        // Hide only explicit ad renderers, not the video player or skip controls.
        document.querySelectorAll('ytd-ad-slot-renderer, ytd-in-feed-ad-layout-renderer')
            .forEach(ad => {
                const card = ad.closest('ytd-rich-item-renderer, ytd-video-renderer');
                if (card) card.setAttribute('data-litewave-sponsored', '1');
            });
        const player = document.querySelector('#movie_player');
        if (!player || (!player.classList.contains('ad-showing') &&
                        !player.classList.contains('ad-interrupting'))) return;
        player.querySelectorAll(
            '.ytp-ad-skip-button, .ytp-ad-skip-button-modern, ' +
            '.ytp-skip-ad-button, .ytp-ad-overlay-close-button'
        ).forEach(button => {
            if (!button.disabled && button.getClientRects().length &&
                getComputedStyle(button).visibility !== 'hidden') button.click();
        });
        // Never modify currentTime, playbackRate, muted, src, or DRM responses.
    }
    function schedule() {
        if (active && !document.hidden && !timer) timer = setTimeout(cleanYouTube, 250);
    }
    function watch() {
        if (!active || !isYouTube || document.hidden) return;
        if (!observer) observer = new MutationObserver(schedule);
        observer.observe(document.documentElement, {
            childList: true, subtree: true, attributes: true, attributeFilter: ['class']
        });
        schedule();
    }
    function visibilityChanged() {
        if (document.hidden) {
            observer?.disconnect();
            clearTimeout(timer);
            timer = 0;
        } else watch();
    }
    const stop = () => {
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
    };
    function pageHide(event) {
        observer?.disconnect();
        clearTimeout(timer);
        timer = 0;
        if (!event.persisted) stop();
    }
    function pageShow() { watch(); }
    globalThis.__litewaveShieldStop = stop;
    document.addEventListener('visibilitychange', visibilityChanged);
    document.addEventListener('yt-navigate-finish', schedule);
    window.addEventListener('pagehide', pageHide);
    window.addEventListener('pageshow', pageShow);
    watch();
})();
