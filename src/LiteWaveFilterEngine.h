#pragma once
extern "C" {
struct LiteWaveAdblockEngine;
LiteWaveAdblockEngine *lw_adblock_engine_create();
void lw_adblock_engine_destroy(LiteWaveAdblockEngine *);
int lw_adblock_engine_should_block(const LiteWaveAdblockEngine *, const char *, const char *, const char *);
int lw_adblock_engine_rule_count(const LiteWaveAdblockEngine *);
}