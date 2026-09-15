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
            'ytd-banner-promo-renderer, ytd-statement-banner-renderer,',
            '.ytd-action-companion-ad-renderer, #player-ads, .video-ads,',
            '.ytp-ad-overlay-container, .ytp-ad-message-container,',
            '[data-litewave-sponsored="1"] {display:none!important;}'
        ].join('\n') : ''
    ].join('\n');
    (document.head || document.documentElement).appendChild(style);

    function cleanYouTube() {
        timer = 0;
        if (!active || document.hidden) return;

        // Hide ad cards and sponsored slots on YouTube
        document.querySelectorAll('ytd-ad-slot-renderer, ytd-in-feed-ad-layout-renderer, ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer')
            .forEach(ad => {
                const card = ad.closest('ytd-rich-item-renderer, ytd-video-renderer');
                if (card) card.setAttribute('data-litewave-sponsored', '1');
            });

        const player = document.querySelector('#movie_player');
        const isAdShowing = player && (
            player.classList.contains('ad-showing') ||
            player.classList.contains('ad-interrupting')
        );

        const video = document.querySelector('video');
        if (isAdShowing && video) {
            if (!video.hasAttribute('data-litewave-ad-muted')) {
                video.setAttribute('data-litewave-ad-muted', '1');
                video.muted = true;
                video.playbackRate = 16.0;
            }
        } else if (video && video.hasAttribute('data-litewave-ad-muted')) {
            video.removeAttribute('data-litewave-ad-muted');
            video.playbackRate = 1.0;
            video.muted = false;
        }

        // Auto-click Skip Ad buttons safely
        const skipButtons = document.querySelectorAll(
            '.ytp-ad-skip-button, .ytp-ad-skip-button-modern, ' +
            '.ytp-skip-ad-button, .ytp-ad-skip-button-slot, ' +
            '.ytp-ad-skip-button-container, .ytp-ad-overlay-close-button, ' +
            'button.ytp-ad-skip-button-single, .ytp-ad-skip-button-text, ' +
            'button[id^="skip-button"], .ytp-ad-preview-container'
        );
        skipButtons.forEach(btn => {
            if (btn && !btn.disabled) {
                try {
                    btn.click();
                    if (typeof btn.onclick === 'function') btn.onclick();
                } catch (e) {}
            }
        });
    }

    function schedule() {
        if (active && !document.hidden && !timer) timer = setTimeout(cleanYouTube, 200);
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
