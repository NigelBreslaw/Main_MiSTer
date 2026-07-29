#include "fpga_ownership.h"

#include <stdio.h>
#include <string.h>

MagikFpgaOwnership::MagikFpgaOwnership()
	: owner_(MagikFpgaOwner::Main),
	  epoch_(0),
	  blocked_spi_writes_(0),
	  blocked_gpo_writes_(0),
	  last_blocked_site_()
{
}

bool MagikFpgaOwnership::transfer(
	MagikFpgaOwner expected,
	MagikFpgaOwner next,
	const char *site)
{
	if (owner_ != expected) return false;
	if (owner_ == next) return true;
	owner_ = next;
	epoch_++;
	if (site && site[0])
		snprintf(last_blocked_site_, sizeof(last_blocked_site_), "transfer:%s", site);
	return true;
}

bool MagikFpgaOwnership::allow_main(MagikFpgaAccess access, const char *site)
{
	if (owner_ == MagikFpgaOwner::Main) return true;
	if (access == MagikFpgaAccess::Spi)
		blocked_spi_writes_++;
	else
		blocked_gpo_writes_++;
	snprintf(
		last_blocked_site_,
		sizeof(last_blocked_site_),
		"%s",
		site && site[0] ? site : "unknown");
	return false;
}

MagikFpgaOwnershipSnapshot MagikFpgaOwnership::snapshot() const
{
	MagikFpgaOwnershipSnapshot out = {
		owner_,
		epoch_,
		blocked_spi_writes_,
		blocked_gpo_writes_,
		last_blocked_site_,
	};
	return out;
}

const char *magik_fpga_owner_name(MagikFpgaOwner owner)
{
	switch (owner)
	{
	case MagikFpgaOwner::Main: return "main";
	case MagikFpgaOwner::Launcher: return "magik";
	}
	return "unknown";
}
