#include "input_proxy.h"

#include <string.h>

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

int magik_input_proxy_keyboard_key(uint16_t code)
{
	switch (code)
	{
	case MagikProxyKeyEsc:
	case MagikProxyKeyTab:
	case MagikProxyKeyEnter:
	case MagikProxyKeySpace:
	case MagikProxyKeyF9:
	case MagikProxyKeyF10:
	case MagikProxyKeyUp:
	case MagikProxyKeyPageUp:
	case MagikProxyKeyLeft:
	case MagikProxyKeyRight:
	case MagikProxyKeyDown:
	case MagikProxyKeyPageDown:
	case MagikProxyKeyMenu:
		return code;
	default:
		return 0;
	}
}

bool magik_input_proxy_allows_fpga_output(bool launcher_mode)
{
	return !launcher_mode;
}

void magik_input_proxy_init(MagikInputProxyState *state)
{
	if (state) memset(state, 0, sizeof(*state));
}

static bool contributor_matches(
	const MagikInputProxyContributor &contributor,
	int jnum,
	uint32_t code,
	uint32_t mask,
	int bnum,
	int key)
{
	return contributor.active &&
	       contributor.jnum == jnum &&
	       contributor.code == code &&
	       contributor.mask == mask &&
	       contributor.bnum == bnum &&
	       contributor.key == key;
}

MagikInputProxyUpdate magik_input_proxy_update(
	MagikInputProxyState *state,
	int jnum,
	uint32_t code,
	uint32_t mask,
	int bnum,
	int key,
	bool press,
	MagikInputProxyEvent *event)
{
	if (!state || !event || key <= 0 || key >= 256) return MagikInputProxyNoChange;
	MagikInputProxyContributor *free_slot = NULL;
	MagikInputProxyContributor *match = NULL;
	for (size_t i = 0; i < MAGIK_INPUT_PROXY_MAX_CONTRIBUTORS; i++)
	{
		MagikInputProxyContributor *candidate = &state->contributors[i];
		if (!candidate->active && !free_slot) free_slot = candidate;
		if (contributor_matches(*candidate, jnum, code, mask, bnum, key))
		{
			match = candidate;
			break;
		}
	}

	if (press)
	{
		if (match) return MagikInputProxyNoChange;
		if (!free_slot) return MagikInputProxyOverflow;
		free_slot->active = true;
		free_slot->jnum = jnum;
		free_slot->code = code;
		free_slot->mask = mask;
		free_slot->bnum = bnum;
		free_slot->key = key;
		uint16_t previous = state->key_holds[key]++;
		if (previous) return MagikInputProxyNoChange;
		event->key = key;
		event->press = 1;
		return MagikInputProxyEmit;
	}

	if (!match) return MagikInputProxyUnmatchedRelease;
	match->active = false;
	if (!state->key_holds[key]) return MagikInputProxyUnmatchedRelease;
	state->key_holds[key]--;
	if (state->key_holds[key]) return MagikInputProxyNoChange;
	event->key = key;
	event->press = 0;
	return MagikInputProxyEmit;
}

size_t magik_input_proxy_reset(MagikInputProxyState *state, int *released_keys, size_t capacity)
{
	if (!state) return 0;
	size_t released = 0;
	for (size_t key = 0; key < 256; key++)
	{
		if (!state->key_holds[key]) continue;
		if (released_keys && released < capacity) released_keys[released] = (int)key;
		released++;
	}
	magik_input_proxy_init(state);
	return released;
}

void magik_input_proxy_journal_init(MagikInputProxyJournal *journal)
{
	if (journal) memset(journal, 0, sizeof(*journal));
}

bool magik_input_proxy_journal_push(MagikInputProxyJournal *journal, MagikInputProxyEvent event)
{
	if (!journal || journal->size >= MAGIK_INPUT_PROXY_JOURNAL_CAPACITY) return false;
	size_t tail = (journal->head + journal->size) % MAGIK_INPUT_PROXY_JOURNAL_CAPACITY;
	journal->events[tail] = event;
	journal->size++;
	return true;
}

bool magik_input_proxy_journal_front(const MagikInputProxyJournal *journal, MagikInputProxyEvent *event)
{
	if (!journal || !event || !journal->size) return false;
	*event = journal->events[journal->head];
	return true;
}

void magik_input_proxy_journal_pop(MagikInputProxyJournal *journal)
{
	if (!journal || !journal->size) return;
	journal->head = (journal->head + 1) % MAGIK_INPUT_PROXY_JOURNAL_CAPACITY;
	journal->size--;
}
