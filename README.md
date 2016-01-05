# orangepi_wily_firmware_loader
firmware loader in udev for kernel 3.4.39, which is used by orangepi

## udev change in kernel
since 3.8, firmware can be loaded in kernel mode, a new function is provided by kernel in drivers/base/firmware_class.c, named "fw_get_filesystem_firmware".
And thus, systemd removed the support of loading firmware in usermode since v217(udev-builtin-firmware.c is removed from udev)

## what happed to orangepi
orangepi ubuntu use kernel 3.4.39, in which version, usermode firmware loading is essential, but missed. For this reason, the orangepi wily will prevent pass the firmware request by setting stub response in 50-firmware.rules
it says:
- #stub for immediately telling the kernel that userspace firmware loading
- #failed; necessary to avoid long timeouts with CONFIG_FW_LOADER_USER_HELPER=y
- SUBSYSTEM=="firmware", ACTION=="add", ATTR{loading}="-1"

## what you need to do
if you want to use hardware with firmware orangepi wily, 
- 1. you should replace the "/lib/udev/rules.d/50-firmware.rules" in the wily with the one I provided
- 2. you have to build the firmware.c in the orangepi ubuntu or with a cross compiling tool, I compiled it with arm-linux-gnueabi-gcc, you can use apt-get to install it. There is a prebuilt binary file in the prebuilt directory. copy it to "/lib/udev"
- 3. orangepi wily doesn't provide firmware, you have to copy the firmware from other source. I copied them from PC ubuntu.

## about the firmware.c
I copied function set_loading, copy_firmware, and builtin_firmware from udev-builtin-firmware.c, and remove the dependence to udev. now, it only use std features and little linux feature.

## issues
these is still some issue now:
the rtl8188cu will work well after reboot, but if you plug it out, rtlwifi will print some error message, and then if you plug it in again, the wifi will not be light up. This will not be a trouble if you don't plug out it, but the root cause is still not clear.
