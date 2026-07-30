#include "input_proxy.h"

namespace
{
constexpr uint32_t kJoyRight = 0x0001;
constexpr uint32_t kJoyLeft = 0x0002;
constexpr uint32_t kJoyDown = 0x0004;
constexpr uint32_t kJoyUp = 0x0008;
constexpr uint32_t kJoyA = 0x0010;
constexpr uint32_t kJoyB = 0x0020;
constexpr uint32_t kMenuY = 0x0040;
constexpr uint32_t kMenuX = 0x0080;
constexpr uint32_t kJoyL = 0x0400;
constexpr uint32_t kJoyR = 0x0800;
constexpr uint32_t kMenuStart = 0x1000;
constexpr uint32_t kMenuSelect = 0x2000;
}

int magik_input_proxy_key(uint32_t mask, bool osd_button)
{
	if (osd_button) return MagikProxyKeyMenu;

	switch (mask)
	{
	case kJoyRight: return MagikProxyKeyRight;
	case kJoyLeft: return MagikProxyKeyLeft;
	case kJoyDown: return MagikProxyKeyDown;
	case kJoyUp: return MagikProxyKeyUp;
	case kJoyA: return MagikProxyKeyEnter;
	case kJoyB: return MagikProxyKeyEsc;
	case kMenuY: return MagikProxyKeySpace;
	case kMenuX: return MagikProxyKeyTab;
	case kJoyL: return MagikProxyKeyPageUp;
	case kJoyR: return MagikProxyKeyPageDown;
	case kMenuStart: return MagikProxyKeyF9;
	case kMenuSelect: return MagikProxyKeyF10;
	default: return 0;
	}
}

bool magik_input_proxy_allows_fpga_output(bool launcher_mode)
{
	return !launcher_mode;
}
