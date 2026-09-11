#include <assert.h>
#include "support/mister_magik/writer_silence.h"

int main()
{
	MagikWriterSilence silence;
	assert(!silence.suppress_writes());
	assert(!silence.mark_drained());
	assert(!silence.enter_graphics(true));
	assert(silence.close_admission());
	assert(silence.suppress_writes());
	assert(!silence.enter_graphics(true));
	assert(silence.mark_drained());
	assert(!silence.enter_graphics(false));
	assert(!silence.ready_for_module());
	assert(silence.enter_graphics(true));
	assert(silence.ready_for_module());
	assert(!silence.close_admission());
	silence.release();
	assert(!silence.suppress_writes());
	assert(!silence.ready_for_module());
	return 0;
}
