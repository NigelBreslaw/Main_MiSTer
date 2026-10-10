#include "support/mister_magik/user_io_config_map.h"

#include <assert.h>

int main()
{
	UserIoFrameworkConfig framework = {};
	framework.vga_scaler = 1;
	framework.vga_sog = 1;
	framework.csync = 1;
	framework.vga_mode_int = 1;
	framework.forced_scandoubler = 1;
	framework.hdmi_audio_96k = 1;
	framework.dvi_mode = 1;
	framework.hdmi_limited = 3;
	framework.direct_video = 2;

	const uint16_t expected_without_fb =
		BUTTON1 |
		CONF_VGA_SCALER |
		CONF_VGA_SOG |
		CONF_CSYNC |
		CONF_YPBPR |
		CONF_FORCED_SCANDOUBLER |
		CONF_AUDIO_96K |
		CONF_DVI |
		CONF_HDMI_LIMITED1 |
		CONF_HDMI_LIMITED2 |
		CONF_DIRECT_VIDEO |
		CONF_DIRECT_VIDEO2;
	const uint16_t without_fb = user_io_apply_framework_config(BUTTON1, framework);
	assert(without_fb == expected_without_fb);

	framework.vga_fb = 1;
	const uint16_t with_fb = user_io_apply_framework_config(BUTTON1, framework);
	assert(with_fb == (expected_without_fb | CONF_VGA_FB));
	assert((with_fb ^ without_fb) == CONF_VGA_FB);
	assert(with_fb & CONF_CSYNC);
	assert(with_fb & CONF_VGA_SOG);
	assert(with_fb & CONF_DIRECT_VIDEO);

	framework.vga_fb = 0;
	framework.csync = 0;
	const uint16_t without_csync = user_io_apply_framework_config(BUTTON1, framework);
	assert(!(without_csync & CONF_CSYNC));
	assert(without_csync & CONF_VGA_SOG);
	assert(without_csync & CONF_DIRECT_VIDEO);
	return 0;
}
