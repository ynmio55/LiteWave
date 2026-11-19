#pragma once
extern "C" {
struct LiteWaveAdblockEngine;
LiteWaveAdblockEngine *lw_adblock_engine_create();
void lw_adblock_engine_destroy(LiteWaveAdblockEngine *);
int lw_adblock_engine_load_lists(const char *ads, const char *privacy);
void lw_adblock_engine_prepare_privacy();
int lw_adblock_engine_is_ready(const LiteWaveAdblockEngine *);
// 1 = block, 2 = allowed by an @@ exception, 0 = no match, -1 = still compiling.
int lw_adblock_engine_should_block(const LiteWaveAdblockEngine *, const char *url, const char *source, const char *kind, int aggressive);
int lw_adblock_engine_rule_count(const LiteWaveAdblockEngine *);
char *lw_adblock_engine_cosmetic_selectors(const LiteWaveAdblockEngine *, const char *url, const char *classes, const char *ids);
void lw_adblock_string_free(char *);
}
