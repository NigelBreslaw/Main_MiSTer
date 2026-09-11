#pragma once

enum class MagikWriterSilenceStage
{
	Open,
	AdmissionClosed,
	Drained,
	Graphics,
};

class MagikWriterSilence
{
public:
	bool close_admission();
	bool mark_drained();
	bool enter_graphics(bool succeeded);
	void release();

	bool suppress_writes() const { return stage_ != MagikWriterSilenceStage::Open; }
	bool ready_for_module() const { return stage_ == MagikWriterSilenceStage::Graphics; }
	MagikWriterSilenceStage stage() const { return stage_; }

private:
	MagikWriterSilenceStage stage_ = MagikWriterSilenceStage::Open;
};
