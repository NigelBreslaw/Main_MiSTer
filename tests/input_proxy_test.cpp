#include "support/mister_magik/input_proxy.h"

#include <assert.h>

int main()
{
	assert(magik_input_proxy_key(0x0001, false) == MagikProxyKeyRight);
	assert(magik_input_proxy_key(0x0002, false) == MagikProxyKeyLeft);
	assert(magik_input_proxy_key(0x0004, false) == MagikProxyKeyDown);
	assert(magik_input_proxy_key(0x0008, false) == MagikProxyKeyUp);
	assert(magik_input_proxy_key(0x0010, false) == MagikProxyKeyEnter);
	assert(magik_input_proxy_key(0x0020, false) == MagikProxyKeyEsc);
	assert(magik_input_proxy_key(0x0040, false) == MagikProxyKeySpace);
	assert(magik_input_proxy_key(0x0080, false) == MagikProxyKeyTab);
	assert(magik_input_proxy_key(0x0400, false) == MagikProxyKeyPageUp);
	assert(magik_input_proxy_key(0x0800, false) == MagikProxyKeyPageDown);
	assert(magik_input_proxy_key(0x1000, false) == MagikProxyKeyF9);
	assert(magik_input_proxy_key(0x2000, false) == MagikProxyKeyF10);
	assert(magik_input_proxy_key(0, true) == MagikProxyKeyMenu);
	assert(magik_input_proxy_key(0x4000, false) == 0);
	assert(magik_input_proxy_allows_fpga_output(false));
	assert(!magik_input_proxy_allows_fpga_output(true));
	return 0;
}
