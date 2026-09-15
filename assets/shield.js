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

    // YouTube Player API Response Interceptor
    // Intercepts fetch & XHR for youtubei/v1/player to strip ad manifests before playback
    if (isYouTube && !globalThis.__litewaveYtPatched) {
        globalThis.__litewaveYtPatched = true;

        const cleanYtJson = (json) => {
            if (!json || typeof json !== 'object') return json;
            delete json.adPlacements;
            delete json.playerAds;
            delete json.adSlots;
            delete json.adBreakHeartbeatParams;
            if (json.playerResponse) {
                delete json.playerResponse.adPlacements;
                delete json.playerResponse.playerAds;
                delete json.playerResponse.adSlots;
            }
            return json;
        };

        if (typeof window !== 'undefined' && typeof window.fetch === 'function') {
            const origFetch = window.fetch;
            window.fetch = async function(...args) {
                const response = await origFetch.apply(this, args);
                const url = typeof args[0] === 'string' ? args[0] : (args[0] && args[0].url ? args[0].url : '');
                if (url.includes('/youtubei/v1/player')) {
                    try {
                        const clone = response.clone();
                        const json = await clone.json();
                        const cleaned = cleanYtJson(json);
                        return new Response(JSON.stringify(cleaned), {
                            status: response.status,
                            statusText: response.statusText,
                            headers: response.headers
                        });
                    } catch (e) {}
                }
                return response;
            };
        }

        if (typeof window !== 'undefined' && typeof window.XMLHttpRequest === 'function') {
            const origOpen = window.XMLHttpRequest.prototype.open;
            const origSend = window.XMLHttpRequest.prototype.send;
            window.XMLHttpRequest.prototype.open = function(method, url, ...rest) {
                this.__litewaveUrl = url;
                return origOpen.call(this, method, url, ...rest);
            };
            window.XMLHttpRequest.prototype.send = function(...args) {
                if (this.__litewaveUrl && typeof this.__litewaveUrl === 'string' && this.__litewaveUrl.includes('/youtubei/v1/player')) {
                    this.addEventListener('readystatechange', function() {
                        if (this.readyState === 4 && this.responseText) {
                            try {
                                const json = JSON.parse(this.responseText);
                                const cleaned = cleanYtJson(json);
                                Object.defineProperty(this, 'responseText', { value: JSON.stringify(cleaned) });
                                Object.defineProperty(this, 'response', { value: JSON.stringify(cleaned) });
                            } catch (e) {}
                        }
                    }, { once: true });
                }
                return origSend.apply(this, args);
            };
        }
    }

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
    const target = document.head || document.documentElement || document.body;
    if (target) target.appendChild(style);

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
            video.muted = true;
            video.playbackRate = 16.0;
            if (!isNaN(video.duration) && video.duration > 0 && isFinite(video.duration)) {
                video.currentTime = video.duration;
            }
            if (typeof video.setAttribute === 'function') {
                video.setAttribute('data-litewave-ad-muted', '1');
            }
        } else if (video && typeof video.hasAttribute === 'function' && video.hasAttribute('data-litewave-ad-muted')) {
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
