#include "../vm_core/vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

uint8_t firmware[64*1024];

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: %s firmware.bin\n", argv[0]);
		exit(0);
	}

	int i,f;
	f = open(argv[1],O_RDONLY);
	if ( f < 1)
	{
		perror("opening firmware file");
		exit(1);
	}
	i = read(f, firmware, 64*1024);
	if (i < 1)
	{
		perror("reading firmware file");
		exit(1);
	}
	printf("Firmware: loaded %i bytes.\n",i);
	vm_reset();
	while(vm_running() == 1)
	{
		vm_step();
	}
}

uint8_t vmext_getbyte(uint16_t addr)
{
	return firmware[addr];
}
