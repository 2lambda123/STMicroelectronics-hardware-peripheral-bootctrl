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

/* THE HAL BOOTCTRL HEADER MUST BE IN SYNC WITH THE UBOOT BOOTCTRL HEADER */

#ifndef _BOOTCTRL_H_
#define _BOOTCTRL_H_

#include <stdint.h>
#include <stddef.h>

/* boot_ctrl structure version - to be incremented if any change */
#define BOOTCTRL_VERSION			1

#define BOOTCTRL_NUM_SLOT			2
#define BOOTCTRL_SUFFIX_A			"_a"
#define BOOTCTRL_SUFFIX_B			"_b"


/* struct boot_ctrl occupies the slot_suffix field of struct bootloader_message */
#define BOOTCTRL_OFFSET_SUFFIX		offsetof(struct bootloader_message_ab, slot_suffix)

#endif /* _BOOTCTRL_H_ */
