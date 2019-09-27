/*
 * Copyright 2015 The Android Open Source Project
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>

#include <bootable/recovery/bootloader.h>
#include "bootctrl.h"

static struct bootloader_control bctrl;
static char *misc_file = "misc.img";

static void usage() {
	printf("Usage: miscgen <aPrio> <aTry> <aSuccess> <bPrio> <bTry> <bSuccess> [file]\n");
	printf("   xPrio = value between 0 and 15\n");
	printf("   xTry = value between 0 and 7\n");
	printf("   xSuccess = value 0 (false) or 1 (true)\n");
	printf("   [file] = name of the output file (opt), misc.img by default\n");
}

static void write_misc_suffix() {
	int ret = 0;
	int fd;

	fd = open(misc_file, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	if(fd < 0) {
		printf("%s, open %s failed, fd %d, errno %d\n", __FUNCTION__, misc_file, fd, errno);
		return;
	}

	/* struct boot_ctrl occupies the slot_suffix field of struct bootloader_message */
	lseek(fd, BOOTCTRL_OFFSET_SUFFIX, SEEK_SET);

	/* write boot_ctrl data in the slot_suffix field */
	ret = write(fd, &bctrl, sizeof(struct bootloader_control));

	if(ret != sizeof(struct bootloader_control)) {
		close(fd);
		printf("%s, write failed, expect %u, actual %d\n", __FUNCTION__, (unsigned int)sizeof(struct bootloader_control), ret);
		return;
	}

	fsync(fd);
	close(fd);

	return;
}

int main(int argc, char *argv[])
{
	int i;

	if(argc < 7) {
		usage();
		return 1;
	}

	if ((atoi(argv[1]) > 15) || (atoi(argv[4]) > 15)
		|| (atoi(argv[2]) > 7) || (atoi(argv[5]) > 7)
		|| (atoi(argv[3]) > 1) || (atoi(argv[6]) > 1)) {
		usage();
		return 1;
	}

	/* update bootctrl based on input parameters */
	bctrl.recovery_tries_remaining = 7;
	bctrl.slot_info[0].priority = atoi(argv[1]);
	bctrl.slot_info[0].tries_remaining = atoi(argv[2]);
	bctrl.slot_info[0].successful_boot = atoi(argv[3]);
	bctrl.slot_info[1].priority = atoi(argv[4]);
	bctrl.slot_info[1].tries_remaining = atoi(argv[5]);
	bctrl.slot_info[1].successful_boot = atoi(argv[6]);

	if(argc >= 8)
		misc_file = argv[7];

	printf("output file: %s\n", misc_file);

	unsigned char buffer[] = {'\0', 'S', 'T', 0x00};
	bctrl.magic  =  (uint32_t)buffer[0] << 24 |
					(uint32_t)buffer[1] << 16 |
					(uint32_t)buffer[2] << 8  |
					(uint32_t)buffer[3];

	bctrl.version = BOOTCTRL_VERSION;

	bctrl.crc32_le = crc32(0, (uint8_t *)&bctrl, sizeof(struct bootloader_control) - sizeof(uint32_t));

	printf("crc: %d\n", bctrl.crc32_le);

	for(i = 0; i < BOOTCTRL_NUM_SLOT; i++) {
		printf("slot %d: prio %d, try %d, success %d\n", i,
			bctrl.slot_info[i].priority,
			bctrl.slot_info[i].tries_remaining,
			bctrl.slot_info[i].successful_boot);
	}

	write_misc_suffix();

	return 0;
}
