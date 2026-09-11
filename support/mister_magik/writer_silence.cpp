#include "writer_silence.h"

bool MagikWriterSilence::close_admission()
{
	if (stage_ != MagikWriterSilenceStage::Open) return false;
	stage_ = MagikWriterSilenceStage::AdmissionClosed;
	return true;
}

bool MagikWriterSilence::mark_drained()
{
	if (stage_ != MagikWriterSilenceStage::AdmissionClosed) return false;
	stage_ = MagikWriterSilenceStage::Drained;
	return true;
}

bool MagikWriterSilence::enter_graphics(bool succeeded)
{
	if (stage_ != MagikWriterSilenceStage::Drained || !succeeded) return false;
	stage_ = MagikWriterSilenceStage::Graphics;
	return true;
}

void MagikWriterSilence::release()
{
	stage_ = MagikWriterSilenceStage::Open;
}
