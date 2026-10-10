#pragma once

#include <stdint.h>

enum class MagikFpgaOwner
{
	Main,
	Launcher,
};

enum class MagikFpgaAccess
{
	Spi,
	Gpo,
};

struct MagikFpgaOwnershipSnapshot
{
	MagikFpgaOwner owner;
	uint64_t epoch;
	uint64_t blocked_spi_writes;
	uint64_t blocked_gpo_writes;
	const char *last_blocked_site;
};

class MagikFpgaOwnership
{
public:
	MagikFpgaOwnership();

	bool transfer(MagikFpgaOwner expected, MagikFpgaOwner next, const char *site);
	bool allow_main(MagikFpgaAccess access, const char *site);
	MagikFpgaOwnershipSnapshot snapshot() const;

private:
	MagikFpgaOwner owner_;
	uint64_t epoch_;
	uint64_t blocked_spi_writes_;
	uint64_t blocked_gpo_writes_;
	char last_blocked_site_[96];
};

const char *magik_fpga_owner_name(MagikFpgaOwner owner);
