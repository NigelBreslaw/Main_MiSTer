#pragma once

#include <stdint.h>

#define BUTTON1                 0b0000000000000001
#define BUTTON2                 0b0000000000000010
#define CONF_VGA_SCALER         0b0000000000000100
#define CONF_CSYNC              0b0000000000001000
#define CONF_FORCED_SCANDOUBLER 0b0000000000010000
#define CONF_YPBPR              0b0000000000100000
#define CONF_AUDIO_96K          0b0000000001000000
#define CONF_DVI                0b0000000010000000
#define CONF_HDMI_LIMITED1      0b0000000100000000
#define CONF_VGA_SOG            0b0000001000000000
#define CONF_DIRECT_VIDEO       0b0000010000000000
#define CONF_HDMI_LIMITED2      0b0000100000000000
#define CONF_VGA_FB             0b0001000000000000
#define CONF_DIRECT_VIDEO2      0b0010000000000000

struct UserIoFrameworkConfig
{
	int vga_scaler;
	int vga_sog;
	int csync;
	int vga_mode_int;
	int forced_scandoubler;
	int hdmi_audio_96k;
	int dvi_mode;
	int hdmi_limited;
	int direct_video;
	int vga_fb;
};

static inline uint16_t user_io_apply_framework_config(
	uint16_t map,
	const UserIoFrameworkConfig &framework)
{
	if (framework.vga_scaler) map |= CONF_VGA_SCALER;
	if (framework.vga_sog) map |= CONF_VGA_SOG;
	if (framework.csync) map |= CONF_CSYNC;
	if (framework.vga_mode_int == 1) map |= CONF_YPBPR;
	if (framework.forced_scandoubler) map |= CONF_FORCED_SCANDOUBLER;
	if (framework.hdmi_audio_96k) map |= CONF_AUDIO_96K;
	if (framework.dvi_mode == 1) map |= CONF_DVI;
	if (framework.hdmi_limited & 1) map |= CONF_HDMI_LIMITED1;
	if (framework.hdmi_limited & 2) map |= CONF_HDMI_LIMITED2;
	if (framework.direct_video) map |= CONF_DIRECT_VIDEO;
	if (framework.direct_video == 2) map |= CONF_DIRECT_VIDEO2;
	if (framework.vga_fb) map |= CONF_VGA_FB;
	return map;
}
