# hardware-bootctrl #

This module contains the STMicroelectronics Boot Control HAL source code and the dedicated misc partition image generator.
It is part of the STMicroelectronics delivery for Android (see the [delivery][] for more information).

[delivery]: https://wiki.st.com/stm32mpu/wiki/STM32MP15_distribution_for_Android_release_note_-_v1.0.0

## Description ##

This module version includes the first version of the Boot Control Android abstraction layer.
It is based on the Module Boot Control API version 0.1.

Please see the Android delivery release notes for more details.

## Documentation ##

* The [release notes][] provide information on the release.
* The [distribution package][] provides detailed information on how to use this delivery.

[release notes]: https://wiki.st.com/stm32mpu/wiki/STM32MP15_distribution_for_Android_release_note_-_v1.0.0
[distribution package]: https://wiki.st.com/stm32mpu/wiki/STM32MP1_Distribution_Package_for_Android

## Dependencies ##

This module can't be used alone. It is part of the STMicroelectronics delivery for Android.
To be able to use it the device.mk must have the following packages:
```
PRODUCT_PACKAGES += \
    bootctrl.stm \
    android.hardware.boot@1.0-impl \
    android.hardware.boot@1.0-service
```

It's possible to integrate the static library with its dependencies (useful for update engine)
```
PRODUCT_STATIC_BOOT_CONTROL_HAL := \
    bootctrl_static.stm \
    libcutils \
    libz
```

## Containing ##

This directory contains the sources and associated Android makefile to generate the bootctrl.stm and bootctrl_static.stm library.
It contains also a generator (in ./tool directory) to create the initial misc partition image.

## License ##

This module is distributed under the Apache License, Version 2.0 found in the [LICENSE](./LICENSE) file.
