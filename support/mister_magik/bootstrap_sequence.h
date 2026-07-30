#pragma once

enum class MagikBootstrapStage
{
	Idle,
	ReadyForBlack,
	BlackEntered,
	PreflightComplete,
	OwnershipTransferred,
	Spawned,
	Failed,
	RecoveredStockOsd,
};

enum class MagikBootstrapFailure
{
	None,
	Guard,
	BlackCommand,
	Preflight,
	Ownership,
	Fork,
	Ordering,
};

class MagikBootstrapSequence
{
public:
	MagikBootstrapSequence();

	bool begin(bool main_owns_fpga, bool child_exists);
	bool black_completed(bool command_acknowledged);
	bool preflight_completed(bool passed);
	bool ownership_transferred(bool transferred);
	bool child_spawned(bool fork_succeeded);
	bool recover_stock_osd();

	MagikBootstrapStage stage() const;
	MagikBootstrapFailure failure() const;
	bool spawn_allowed() const;
	bool stock_osd_allowed() const;

private:
	bool fail(MagikBootstrapFailure failure);

	MagikBootstrapStage stage_;
	MagikBootstrapFailure failure_;
};

const char *magik_bootstrap_stage_name(MagikBootstrapStage stage);
const char *magik_bootstrap_failure_name(MagikBootstrapFailure failure);
