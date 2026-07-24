#pragma once

constexpr unsigned long MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS = 20000;
constexpr unsigned long MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS = 30000;

struct MagikDisplayDeadlines
{
	unsigned long confirmation_ms;
	unsigned long fallback_ms;
};

constexpr MagikDisplayDeadlines magik_display_confirmation_deadlines(unsigned long now_ms)
{
	return {
		now_ms + MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS,
		now_ms + MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS,
	};
}
