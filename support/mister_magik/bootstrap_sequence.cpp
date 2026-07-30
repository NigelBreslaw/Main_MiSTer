#include "bootstrap_sequence.h"

MagikBootstrapSequence::MagikBootstrapSequence()
	: stage_(MagikBootstrapStage::Idle),
	  failure_(MagikBootstrapFailure::None)
{
}

bool MagikBootstrapSequence::fail(MagikBootstrapFailure failure)
{
	stage_ = MagikBootstrapStage::Failed;
	failure_ = failure;
	return false;
}

bool MagikBootstrapSequence::begin(bool main_owns_fpga, bool child_exists)
{
	stage_ = MagikBootstrapStage::Idle;
	failure_ = MagikBootstrapFailure::None;
	if (!main_owns_fpga || child_exists)
		return fail(MagikBootstrapFailure::Guard);
	stage_ = MagikBootstrapStage::ReadyForBlack;
	return true;
}

bool MagikBootstrapSequence::black_completed(bool command_acknowledged)
{
	if (stage_ != MagikBootstrapStage::ReadyForBlack)
		return fail(MagikBootstrapFailure::Ordering);
	if (!command_acknowledged)
		return fail(MagikBootstrapFailure::BlackCommand);
	stage_ = MagikBootstrapStage::BlackEntered;
	return true;
}

bool MagikBootstrapSequence::preflight_completed(bool passed)
{
	if (stage_ != MagikBootstrapStage::BlackEntered)
		return fail(MagikBootstrapFailure::Ordering);
	if (!passed)
		return fail(MagikBootstrapFailure::Preflight);
	stage_ = MagikBootstrapStage::PreflightComplete;
	return true;
}

bool MagikBootstrapSequence::ownership_transferred(bool transferred)
{
	if (stage_ != MagikBootstrapStage::PreflightComplete)
		return fail(MagikBootstrapFailure::Ordering);
	if (!transferred)
		return fail(MagikBootstrapFailure::Ownership);
	stage_ = MagikBootstrapStage::OwnershipTransferred;
	return true;
}

bool MagikBootstrapSequence::child_spawned(bool fork_succeeded)
{
	if (stage_ != MagikBootstrapStage::OwnershipTransferred)
		return fail(MagikBootstrapFailure::Ordering);
	if (!fork_succeeded)
		return fail(MagikBootstrapFailure::Fork);
	stage_ = MagikBootstrapStage::Spawned;
	return true;
}

bool MagikBootstrapSequence::recover_stock_osd()
{
	if (stage_ != MagikBootstrapStage::Failed)
		return false;
	stage_ = MagikBootstrapStage::RecoveredStockOsd;
	return true;
}

MagikBootstrapStage MagikBootstrapSequence::stage() const
{
	return stage_;
}

MagikBootstrapFailure MagikBootstrapSequence::failure() const
{
	return failure_;
}

bool MagikBootstrapSequence::spawn_allowed() const
{
	return stage_ == MagikBootstrapStage::OwnershipTransferred;
}

bool MagikBootstrapSequence::stock_osd_allowed() const
{
	return stage_ == MagikBootstrapStage::RecoveredStockOsd;
}

const char *magik_bootstrap_stage_name(MagikBootstrapStage stage)
{
	switch (stage)
	{
	case MagikBootstrapStage::Idle: return "idle";
	case MagikBootstrapStage::ReadyForBlack: return "ready-for-black";
	case MagikBootstrapStage::BlackEntered: return "black-entered";
	case MagikBootstrapStage::PreflightComplete: return "preflight-complete";
	case MagikBootstrapStage::OwnershipTransferred: return "ownership-transferred";
	case MagikBootstrapStage::Spawned: return "spawned";
	case MagikBootstrapStage::Failed: return "failed";
	case MagikBootstrapStage::RecoveredStockOsd: return "recovered-stock-osd";
	}
	return "unknown";
}

const char *magik_bootstrap_failure_name(MagikBootstrapFailure failure)
{
	switch (failure)
	{
	case MagikBootstrapFailure::None: return "none";
	case MagikBootstrapFailure::Guard: return "guard";
	case MagikBootstrapFailure::BlackCommand: return "black-command";
	case MagikBootstrapFailure::Preflight: return "preflight";
	case MagikBootstrapFailure::Ownership: return "ownership";
	case MagikBootstrapFailure::Fork: return "fork";
	case MagikBootstrapFailure::Ordering: return "ordering";
	}
	return "unknown";
}
