/* power.h — leaving: reboot and power off. */
#ifndef SPROUT_POWER_H
#define SPROUT_POWER_H

__attribute__((noreturn)) void reboot(void);
__attribute__((noreturn)) void poweroff(void);

#endif
