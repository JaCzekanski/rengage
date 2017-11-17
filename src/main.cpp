#include <cstdio>
#include <stdint.h>
#include "utils/file.h"
#include <unicorn/unicorn.h>

const std::string BOOT_FILE = "data/boot.dmp";
const uint32_t BOOT_ADDR = 0x00000000;
const size_t BOOT_SIZE = 0x2000;

const std::string ROM_FILE = "data/rom.dmp";
const uint32_t ROM_ADDR = 0x50000000;
const size_t ROM_SIZE = 0x1400000;

static bool hook_mem_invalid(uc_engine *uc, uc_mem_type type,
	uint64_t addr, int size, int64_t value, void *user_data)
{
	printf("MEM 0x%llx, size = %d, data = 0x%llx\n", addr, size, value);
	return false;
}

int main(int argc, char **argv) {
	printf("reengage\n");

	uc_engine *uc;
	auto err = uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc);
	if (err) {
		printf("uc_open failed: %u (%s)\n", err, uc_strerror(err));
		return 1;
	}

	// BOOT 0x0000 0000 - 8kB
	err = uc_mem_map(uc, BOOT_ADDR, BOOT_SIZE, UC_PROT_READ | UC_PROT_EXEC);
	{
		auto boot = getFileContents(BOOT_FILE);
		if (boot.empty()) {
			printf("cannot read boot");
			return 1;
		}
		uc_mem_write(uc, BOOT_ADDR, boot.data(), boot.size());
	}

	// ROM 0x5000 0000 - 0x140 0000 B
	err = uc_mem_map(uc, ROM_ADDR, ROM_SIZE, UC_PROT_READ | UC_PROT_EXEC);
	{
		auto rom = getFileContents(ROM_FILE);
		if (rom.empty()) {
			printf("cannot read rom");
			return 1;
		}
		uc_mem_write(uc, ROM_ADDR, rom.data(), rom.size());
	}

	// Super page 0x4000 0000 - 0x4000 1FFF
	std::vector<uint8_t> mem_superpage;
	mem_superpage.resize(0x2000);

	err = uc_mem_map_ptr(uc, 0x40000000, 0x2000, UC_PROT_WRITE, mem_superpage.data());
	if (err) printf("uc_mem_map failed: %u (%s)\n", err, uc_strerror(err));

	// MMIO 0x5800 0000 - 0x5EFF FFFF
	std::vector<uint8_t> mem_mmio;
	mem_mmio.resize(0x7000000);

	err = uc_mem_map_ptr(uc, 0x58000000, 0x7000000, UC_PROT_WRITE, mem_mmio.data());
	if (err) printf("uc_mem_map failed: %u (%s)\n", err, uc_strerror(err));

	// Vectors 0xFFFF 0000 - 0xFFFF FFFF
	std::vector<uint8_t> mem_vectors;
	mem_vectors.resize(0x10000);

	err = uc_mem_map_ptr(uc, 0xFFFF0000, 0x10000, UC_PROT_WRITE, mem_vectors.data());
	if (err) printf("uc_mem_map failed: %u (%s)\n", err, uc_strerror(err));

	uc_hook writeHook;

	err = uc_hook_add(uc, &writeHook, UC_HOOK_MEM_INVALID, hook_mem_invalid, nullptr, 1, 0);
	if (err) {
		printf("uc_hook_add: %u (%s)\n", err, uc_strerror(err));
	}

	uint32_t pc = 0;
	for (int i = 0; i < 200; i++) {
		printf("PC: 0x%08x\n", pc);
		auto err = uc_emu_start(uc, pc, 0xffffffff, 0, 1);
		if (err) {
			printf("uc_emu_error: %u (%s)\n", err, uc_strerror(err));
		}
		uc_reg_read(uc, UC_ARM_REG_PC, &pc);
	}
	
	putFileContents("mem_superpage.bin", mem_superpage);
	putFileContents("mem_mmio.bin", mem_mmio);
	putFileContents("mem_vectors.bin", mem_vectors);
	return 0;
}