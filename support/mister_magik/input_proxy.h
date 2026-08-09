#pragma once

#include <stddef.h>
#include <stdint.h>

#define MAGIK_INPUT_PROXY_MAX_CONTRIBUTORS 128
#define MAGIK_INPUT_PROXY_JOURNAL_CAPACITY 64

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

struct MagikInputProxyContributor
{
	bool active;
	int jnum;
	uint32_t code;
	uint32_t mask;
	int bnum;
	int key;
};

struct MagikInputProxyState
{
	MagikInputProxyContributor contributors[MAGIK_INPUT_PROXY_MAX_CONTRIBUTORS];
	uint16_t key_holds[256];
};

struct MagikInputProxyEvent
{
	int key;
	int press;
};

enum MagikInputProxyUpdate
{
	MagikInputProxyNoChange,
	MagikInputProxyEmit,
	MagikInputProxyOverflow,
	MagikInputProxyUnmatchedRelease,
};

void magik_input_proxy_init(MagikInputProxyState *state);
MagikInputProxyUpdate magik_input_proxy_update(
	MagikInputProxyState *state,
	int jnum,
	uint32_t code,
	uint32_t mask,
	int bnum,
	int key,
	bool press,
	MagikInputProxyEvent *event);
size_t magik_input_proxy_reset(MagikInputProxyState *state, int *released_keys, size_t capacity);

struct MagikInputProxyJournal
{
	MagikInputProxyEvent events[MAGIK_INPUT_PROXY_JOURNAL_CAPACITY];
	size_t head;
	size_t size;
};

void magik_input_proxy_journal_init(MagikInputProxyJournal *journal);
bool magik_input_proxy_journal_push(MagikInputProxyJournal *journal, MagikInputProxyEvent event);
bool magik_input_proxy_journal_front(const MagikInputProxyJournal *journal, MagikInputProxyEvent *event);
void magik_input_proxy_journal_pop(MagikInputProxyJournal *journal);
