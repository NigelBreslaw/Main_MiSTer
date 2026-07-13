#pragma once

#include <stdint.h>

bool magik_neogeo_graphics_memory_warning_needed(
    uint16_t ram_sz,
    bool dual_sdr,
    bool digital_io,
    uint32_t crom_start,
    uint32_t crom_sz_max);
