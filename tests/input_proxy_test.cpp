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

	MagikInputProxyState state = {};
	MagikInputProxyEvent event = {};
	magik_input_proxy_init(&state);
	assert(magik_input_proxy_update(&state, 1, 10, 0x0004, 2, MagikProxyKeyDown, true, &event) == MagikInputProxyEmit);
	assert(event.key == MagikProxyKeyDown && event.press == 1);
	assert(magik_input_proxy_update(&state, 2, 20, 0x0004, 2, MagikProxyKeyDown, true, &event) == MagikInputProxyNoChange);
	assert(magik_input_proxy_update(&state, 1, 10, 0x0004, 2, MagikProxyKeyDown, false, &event) == MagikInputProxyNoChange);
	assert(magik_input_proxy_update(&state, 2, 20, 0x0004, 2, MagikProxyKeyDown, false, &event) == MagikInputProxyEmit);
	assert(event.key == MagikProxyKeyDown && event.press == 0);
	assert(magik_input_proxy_update(&state, 2, 20, 0x0004, 2, MagikProxyKeyDown, false, &event) == MagikInputProxyUnmatchedRelease);

	assert(magik_input_proxy_update(&state, 1, 30, 0x0010, 4, MagikProxyKeyEnter, true, &event) == MagikInputProxyEmit);
	assert(magik_input_proxy_update(&state, 1, 31, 0x0020, 5, MagikProxyKeyEsc, true, &event) == MagikInputProxyEmit);
	int released[4] = {};
	assert(magik_input_proxy_reset(&state, released, 4) == 2);
	assert((released[0] == MagikProxyKeyEsc && released[1] == MagikProxyKeyEnter) ||
	       (released[0] == MagikProxyKeyEnter && released[1] == MagikProxyKeyEsc));

	MagikInputProxyJournal journal = {};
	magik_input_proxy_journal_init(&journal);
	for (size_t i = 0; i < MAGIK_INPUT_PROXY_JOURNAL_CAPACITY; i++)
	{
		assert(magik_input_proxy_journal_push(&journal, {(int)i, (int)(i & 1)}));
	}
	assert(!magik_input_proxy_journal_push(&journal, {999, 1}));
	for (size_t i = 0; i < MAGIK_INPUT_PROXY_JOURNAL_CAPACITY; i++)
	{
		assert(magik_input_proxy_journal_front(&journal, &event));
		assert(event.key == (int)i);
		magik_input_proxy_journal_pop(&journal);
	}
	assert(!magik_input_proxy_journal_front(&journal, &event));
	return 0;
}
