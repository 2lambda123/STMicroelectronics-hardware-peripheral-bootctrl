/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bootctrl"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <linux/fs.h>
#include <hardware/hardware.h>
#include <hardware/boot_control.h>
#include <log/log.h>
#include <cutils/properties.h>
#include <zlib.h>

#include <bootloader.h>

#include "bootctrl.h"

#define BOOTCTRL_METADATA_FILE  "/dev/block/by-name/misc"

static int bootctrl_open_misc(int flags)
{
	int fd;

	fd = open(BOOTCTRL_METADATA_FILE, flags);
	if (fd < 0) {
		ALOGE("Error opening metadata file: %s\n", strerror(errno));
		return -1;
	}
	ALOGV("%s partition opened\n", BOOTCTRL_METADATA_FILE);

	return fd;
}

static bool bootctrl_check_magic(struct bootloader_control *bctrl)
{
	unsigned char buffer[] = {'\0', 'S', 'T', 0x00};
	uint32_t num =  (uint32_t)buffer[0] << 24 |
					(uint32_t)buffer[1] << 16 |
					(uint32_t)buffer[2] << 8  |
					(uint32_t)buffer[3];

	if (bctrl->magic != num) {
		ALOGE("Wrong magic number: found 0x%x instead of 0x%x\n",bctrl->magic, num);
		return false;
	}

	return true;
}

static bool bootctrl_check_version(struct bootloader_control *bctrl)
{
	if (bctrl->version != BOOTCTRL_VERSION)
		return false;

	return true;
}

static bool bootctrl_check_crc(struct bootloader_control *bctrl)
{
	uint32_t crc;
	uint8_t *crc_data = NULL;
	uint32_t crc_len = 0;

	crc_data = (uint8_t *)bctrl;
	crc_len = sizeof(struct bootloader_control) - sizeof(uint32_t);

	crc = crc32(0, crc_data, crc_len);

	if (bctrl->crc32_le != crc) {
		ALOGE("CRC ERROR: caculated CRC = %d vs stored CRC = %d\n",crc, bctrl->crc32_le);
		return false;
	}

	ALOGV("CRC OK: %d\n", crc);

	return true;
}

static bool bootctrl_check_metadata(struct bootloader_control *bctrl)
{
	if (!bootctrl_check_magic(bctrl)) {
		ALOGW("magic number check failed\n");
		return false;
	}

	if (!bootctrl_check_version(bctrl)) {
		ALOGW("bootctrl structure version check failed\n");
		return false;
	}

	if (!bootctrl_check_crc(bctrl)) {
		ALOGW("bootctrl structure CRC check failed\n");
		return false;
	}

	return true;
}

static bool bootctrl_read_metadata(struct bootloader_control *bctrl)
{
	int fd;

	fd = bootctrl_open_misc(O_RDONLY);
	if (fd == -1)
		return false;

	if (lseek(fd, BOOTCTRL_OFFSET_SUFFIX, SEEK_SET) != BOOTCTRL_OFFSET_SUFFIX) {
		close(fd);
		return false;
	}

	ssize_t num_read;
	do {
		num_read = read(fd, (void*) bctrl, sizeof(struct bootloader_control));
	} while (num_read == -1 && errno == EINTR);

	close(fd);

	ALOGV("bootcontrol read data: magic =  0x%x\n", bctrl->magic);
	ALOGV("bootcontrol read data: crc =  %u\n",(uint32_t)bctrl->crc32_le);
	ALOGV("bootcontrol read data: version =  %u\n",(uint8_t)bctrl->version);
	ALOGV("bootcontrol read data: slot_info priority a =  %u\n",(uint8_t)bctrl->slot_info[0].priority);
	ALOGV("bootcontrol read data: slot_info tries_remaining a =  %u\n",(uint8_t)bctrl->slot_info[0].tries_remaining);
	ALOGV("bootcontrol read data: slot_info successful_boot a =  %u\n",(uint8_t)bctrl->slot_info[0].successful_boot);
	ALOGV("bootcontrol read data: slot_info priority b =  %u\n",(uint8_t)bctrl->slot_info[1].priority);
	ALOGV("bootcontrol read data: slot_info tries_remaining b =  %u\n",(uint8_t)bctrl->slot_info[1].tries_remaining);
	ALOGV("bootcontrol read data: slot_info successful_boot b =  %u\n",(uint8_t)bctrl->slot_info[1].successful_boot);
	ALOGV("bootcontrol read data: slot_info recovery_tries_remaining =  %u\n",(uint8_t)bctrl->recovery_tries_remaining);

	if (!bootctrl_check_metadata(bctrl))
		return false;

	return true;
}

static bool bootctrl_write_metadata(struct bootloader_control *bctrl)
{
	int fd;
	uint8_t *crc_data = NULL;
	uint32_t crc_len = 0;

	fd = bootctrl_open_misc(O_RDWR);
	if (fd == -1)
		return false;

	if (lseek(fd, BOOTCTRL_OFFSET_SUFFIX, SEEK_SET) != BOOTCTRL_OFFSET_SUFFIX) {
		close(fd);
		return false;
	}

	/* update CRC before writing metadata */
	crc_data = (uint8_t *)bctrl;
	crc_len = sizeof(struct bootloader_control) - sizeof(uint32_t);

	bctrl->crc32_le = crc32(0, crc_data, crc_len);

	ssize_t num_written;
	do {
		num_written = write(fd, (void*) bctrl, sizeof(struct bootloader_control));
	} while (num_written == -1 && errno == EINTR);

	close(fd);

	if (num_written != sizeof(struct bootloader_control))
		return false;

	return true;
}

void bootctrl_init(boot_control_module_t *module __unused)
{
	/* Nothing to initialize */
}

unsigned bootctrl_get_number_slots(boot_control_module_t *module __unused)
{
	return BOOTCTRL_NUM_SLOT;
}

unsigned bootctrl_get_current_slot(boot_control_module_t *module __unused)
{
	char propbuf[PROPERTY_VALUE_MAX];
	static const char* suffix[BOOTCTRL_NUM_SLOT] = {BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B};
	int i;

	property_get("ro.boot.slot_suffix", propbuf, "");

	if (propbuf[0] != '\0') {
		for (i = 0; i < 2; i++) {
			if (strncmp(propbuf, suffix[i], 2) == 0)
				return i;
		}

		if (i == 2)
			ALOGW("WARNING: androidboot.slot_suffix is invalid\n");
	} else {
		ALOGW("WARNING: androidboot.slot_suffix is NULL\n");
	}

	return 0;
}

int bootctrl_mark_boot_successful(boot_control_module_t *module __unused)
{
	struct bootloader_control bootctrl;
	unsigned 	slot;

	if (!bootctrl_read_metadata(&bootctrl)) {
		ALOGW("WARNING: Error loading boot information\n");
		return -errno;
	}

	slot = bootctrl_get_current_slot(module);

	bootctrl.slot_info[slot].tries_remaining = 0;
	bootctrl.slot_info[slot].successful_boot = true;

	if (!bootctrl_write_metadata(&bootctrl)) {
		ALOGW("WARNING: Error writing boot information\n");
		return -errno;
	}

	return 0;
}

int bootctrl_set_active_boot_slot(boot_control_module_t *module __unused,
	unsigned slot)
{
	struct bootloader_control bootctrl;
	unsigned other_slot;

	if (slot >= 2)
		return -EINVAL;

	if (!bootctrl_read_metadata(&bootctrl)) {
		ALOGW("WARNING: Error loading boot information\n");
		return -errno;
	}

	other_slot = 1 - slot;
	if (bootctrl.slot_info[other_slot].priority == 15)
		bootctrl.slot_info[other_slot].priority = 14;

	bootctrl.slot_info[slot].priority = 15;
	bootctrl.slot_info[slot].tries_remaining = 7;
	bootctrl.slot_info[slot].successful_boot = false;

	if (!bootctrl_write_metadata(&bootctrl)) {
		ALOGW("WARNING: Error writing boot information\n");
		return -errno;
	}

	return 0;
}

int bootctrl_set_slot_as_unbootable(boot_control_module_t *module __unused,
	unsigned slot)
{
	struct bootloader_control bootctrl;
	
	if (slot >= 2) {
		ALOGW("slot parameter invalid\n");
		return -EINVAL;
	}

	if (!bootctrl_read_metadata(&bootctrl)) {
		ALOGW("WARNING: Error loading boot information\n");
		return -errno;
	}

	bootctrl.slot_info[slot].priority = 0;
	bootctrl.slot_info[slot].tries_remaining = 0;
	bootctrl.slot_info[slot].successful_boot = false;

	if (!bootctrl_write_metadata(&bootctrl)) {
		ALOGW("WARNING: Error writing boot information\n");
		return -errno;
	}

	return 0;
}

int bootctrl_is_slot_bootable(boot_control_module_t *module __unused,
	unsigned slot)
{
	struct bootloader_control bootctrl;
	
	if (slot >= 2) {
		ALOGW("slot parameter invalid\n");
		return -EINVAL;
	}

	if (!bootctrl_read_metadata(&bootctrl)) {
		ALOGW("WARNING: Error loading boot information\n");
		return -errno;
	}

	if ((bootctrl.slot_info[slot].priority == 0)
	&& (bootctrl.slot_info[slot].tries_remaining == 0)
	&& (bootctrl.slot_info[slot].successful_boot == false))
		return false;

	return true;
}

static int bootctrl_isSlotMarkedSuccessful(struct boot_control_module* module __unused,
                                         unsigned int slot) {
	struct bootloader_control bootctrl;
	bool is_marked_successful;

	if (slot >= 2) {
		ALOGW("slot parameter invalid\n");
		return -EINVAL;
	}

	if (!bootctrl_read_metadata(&bootctrl)) {
		ALOGW("WARNING: Error loading boot information\n");
		return -errno;
	}

  is_marked_successful = bootctrl.slot_info[slot].successful_boot;

  return is_marked_successful ? 1 : 0;
}

const char *bootctrl_get_suffix(boot_control_module_t *module __unused,
	unsigned slot)
{
	static const char* suffix[BOOTCTRL_NUM_SLOT] = {BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B};
	if (slot >= 2)
		return NULL;
	return suffix[slot];
}

static int bootctrl_open(const hw_module_t *module __unused, const char *id __unused,
	hw_device_t **device __unused)
{
	/* Nothing to do currently */
	return 0;
}

static struct hw_module_methods_t bootctrl_methods = {
	.open  = bootctrl_open,
};

/* Boot Control Module implementation */
boot_control_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag                 = HARDWARE_MODULE_TAG,
		.module_api_version  = BOOT_CONTROL_MODULE_API_VERSION_0_1,
		.hal_api_version     = HARDWARE_HAL_API_VERSION,
		.id                  = BOOT_CONTROL_HARDWARE_MODULE_ID,
		.name                = "STM boot_control module",
		.author              = "STM",
		.methods             = &bootctrl_methods,
	},
	.init                 = bootctrl_init,
	.getNumberSlots       = bootctrl_get_number_slots,
	.getCurrentSlot       = bootctrl_get_current_slot,
	.markBootSuccessful   = bootctrl_mark_boot_successful,
	.setActiveBootSlot    = bootctrl_set_active_boot_slot,
	.setSlotAsUnbootable  = bootctrl_set_slot_as_unbootable,
	.isSlotBootable       = bootctrl_is_slot_bootable,
	.getSuffix            = bootctrl_get_suffix,
	.isSlotMarkedSuccessful = bootctrl_isSlotMarkedSuccessful,
};

