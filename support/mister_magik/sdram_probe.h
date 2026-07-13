#pragma once

#include <stdint.h>

bool magik_sdram_cache_valid(uint16_t cached);
uint16_t magik_sdram_size_from_osdmask(uint16_t osdmask);
uint16_t magik_sdram_ensure_cached_size(uint16_t cached, uint16_t osdmask, bool *probed);
