#include <assert.h>

#include "support/mister_magik/bootstrap_sequence.h"

static MagikBootstrapSequence ready_for_spawn()
{
	MagikBootstrapSequence sequence;
	assert(sequence.begin(true, false));
	assert(sequence.black_completed(true));
	assert(sequence.preflight_completed(true));
	assert(sequence.ownership_transferred(true));
	assert(sequence.spawn_allowed());
	return sequence;
}

static void assert_recovers_without_spawn(MagikBootstrapSequence &sequence)
{
	assert(!sequence.spawn_allowed());
	assert(sequence.stage() == MagikBootstrapStage::Failed);
	assert(sequence.recover_stock_osd());
	assert(sequence.stock_osd_allowed());
}

int main()
{
	MagikBootstrapSequence success = ready_for_spawn();
	assert(success.child_spawned(true));
	assert(success.stage() == MagikBootstrapStage::Spawned);

	MagikBootstrapSequence live_child;
	assert(!live_child.begin(true, true));
	assert(live_child.failure() == MagikBootstrapFailure::Guard);
	assert_recovers_without_spawn(live_child);

	MagikBootstrapSequence wrong_owner;
	assert(!wrong_owner.begin(false, false));
	assert(wrong_owner.failure() == MagikBootstrapFailure::Guard);
	assert_recovers_without_spawn(wrong_owner);

	MagikBootstrapSequence unsupported;
	assert(unsupported.begin(true, false));
	assert(!unsupported.black_completed(false));
	assert(unsupported.failure() == MagikBootstrapFailure::BlackCommand);
	assert_recovers_without_spawn(unsupported);

	MagikBootstrapSequence preflight;
	assert(preflight.begin(true, false));
	assert(preflight.black_completed(true));
	assert(!preflight.preflight_completed(false));
	assert(preflight.failure() == MagikBootstrapFailure::Preflight);
	assert_recovers_without_spawn(preflight);

	MagikBootstrapSequence ownership;
	assert(ownership.begin(true, false));
	assert(ownership.black_completed(true));
	assert(ownership.preflight_completed(true));
	assert(!ownership.ownership_transferred(false));
	assert(ownership.failure() == MagikBootstrapFailure::Ownership);
	assert_recovers_without_spawn(ownership);

	MagikBootstrapSequence fork = ready_for_spawn();
	assert(!fork.child_spawned(false));
	assert(fork.failure() == MagikBootstrapFailure::Fork);
	assert_recovers_without_spawn(fork);

	MagikBootstrapSequence ordering;
	assert(ordering.begin(true, false));
	assert(!ordering.preflight_completed(true));
	assert(ordering.failure() == MagikBootstrapFailure::Ordering);
	assert_recovers_without_spawn(ordering);

	return 0;
}
