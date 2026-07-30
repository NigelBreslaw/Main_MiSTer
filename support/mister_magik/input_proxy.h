#pragma once

#include <stdint.h>

enum MagikInputProxyKey
{
	MagikProxyKeyEsc = 1,
	MagikProxyKeyTab = 15,
	MagikProxyKeyEnter = 28,
	MagikProxyKeySpace = 57,
	MagikProxyKeyF9 = 67,
	MagikProxyKeyF10 = 68,
	MagikProxyKeyUp = 103,
	MagikProxyKeyPageUp = 104,
	MagikProxyKeyLeft = 105,
	MagikProxyKeyRight = 106,
	MagikProxyKeyDown = 108,
	MagikProxyKeyPageDown = 109,
	MagikProxyKeyMenu = 139,
};

int magik_input_proxy_key(uint32_t mask, bool osd_button);
bool magik_input_proxy_allows_fpga_output(bool launcher_mode);
