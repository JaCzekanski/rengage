#include <cstdio>
#include <stdint.h>
#include "utils/file.h"
#include <unicorn/unicorn.h>

const std::string ROM_FILE = "data/rom.dmp";

const uint32_t ROM_ADDR = 0x50000000;
const size_t ROM_SIZE = 0x1400000;

int main(int argc, char **argv) {
	printf("reengage\n");

	uc_engine *uc;
	auto err = uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc);
	if (err) {
		printf("uc_open failed: %u (%s)\n", err, uc_strerror(err));
		return 1;
	}

	uc_mem_map(uc, ROM_ADDR, ROM_SIZE, UC_PROT_READ | UC_PROT_EXEC);

	{
		auto rom = getFileContents(ROM_FILE);
		if (rom.empty()) {
			printf("cannot read rom");
			return 1;
		}
		uc_mem_write(uc, ROM_ADDR, rom.data(), rom.size());
	}

	uc_emu_start(uc, 0x503220D4, 0, 0, 1000);
	
	return 0;
}