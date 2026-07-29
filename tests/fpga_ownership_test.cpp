#include <assert.h>
#include <string.h>

#include "support/mister_magik/fpga_ownership.h"

int main()
{
	MagikFpgaOwnership ownership;
	MagikFpgaOwnershipSnapshot initial = ownership.snapshot();
	assert(initial.owner == MagikFpgaOwner::Main);
	assert(initial.epoch == 0);
	assert(ownership.allow_main(MagikFpgaAccess::Spi, "initial-spi"));

	assert(ownership.transfer(
		MagikFpgaOwner::Main,
		MagikFpgaOwner::Launcher,
		"spawn"));
	assert(!ownership.allow_main(MagikFpgaAccess::Spi, "blocked-spi"));
	assert(!ownership.allow_main(MagikFpgaAccess::Gpo, "blocked-gpo"));

	MagikFpgaOwnershipSnapshot launcher = ownership.snapshot();
	assert(launcher.owner == MagikFpgaOwner::Launcher);
	assert(launcher.epoch == 1);
	assert(launcher.blocked_spi_writes == 1);
	assert(launcher.blocked_gpo_writes == 1);
	assert(!strcmp(launcher.last_blocked_site, "blocked-gpo"));

	assert(!ownership.transfer(
		MagikFpgaOwner::Main,
		MagikFpgaOwner::Launcher,
		"stale-transfer"));
	assert(ownership.transfer(
		MagikFpgaOwner::Launcher,
		MagikFpgaOwner::Main,
		"child-reaped"));
	assert(ownership.snapshot().epoch == 2);
	assert(ownership.allow_main(MagikFpgaAccess::Spi, "restored-spi"));
	return 0;
}
