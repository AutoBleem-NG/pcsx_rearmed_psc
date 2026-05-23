/*
 * PSC eject button polling
 *
 * SDL2 under Wayland does not deliver KEY_EJECTCD (the PSC's physical
 * eject button) as a key event, so we read /dev/input/event* ourselves
 * and look for the gpio-keys device that exposes KEY_EJECTCD.
 *
 * On press we set emu_action = SACTION_SWAP_CD; the main loop's existing
 * do_emu_action() handles single- vs multi-disc cases.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "main.h"
#include "psc_eject.h"
#include "../libpcsxcore/r3000a.h"

extern enum sched_action emu_action;

#define KEYBIT_LONGS ((KEY_MAX + 8 * sizeof(long) - 1) / (8 * sizeof(long)))
#define KEYBIT_TEST(b, k) ((b)[(k) / (8 * sizeof(long))] & (1UL << ((k) % (8 * sizeof(long)))))

#ifndef PSC_EJECT_DEBUG
#define PSC_EJECT_DEBUG 0
#endif

static int eject_fd = -1;

static int try_open_eject_device(void)
{
	unsigned long keybits[KEYBIT_LONGS];
	char path[32];
	int i, fd;

	for (i = 0; i < 32; i++) {
		snprintf(path, sizeof(path), "/dev/input/event%d", i);
		fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0) {
			if (errno == ENOENT)
				continue;
			continue;
		}
		memset(keybits, 0, sizeof(keybits));
		if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) {
			close(fd);
			continue;
		}
		if (KEYBIT_TEST(keybits, KEY_EJECTCD)) {
			fprintf(stderr, "[eject] watching %s for KEY_EJECTCD\n", path);
			fflush(stderr);
			return fd;
		}
		close(fd);
	}
	return -1;
}

void psc_eject_init(void)
{
	if (eject_fd >= 0)
		return;
	eject_fd = try_open_eject_device();
	if (eject_fd < 0) {
		fprintf(stderr, "[eject] no /dev/input/event* exposes KEY_EJECTCD\n");
		fflush(stderr);
	}
}

void psc_eject_poll(void)
{
	struct input_event ev;
	ssize_t rd;

	if (eject_fd < 0)
		return;

	for (;;) {
		rd = read(eject_fd, &ev, sizeof(ev));
		if (rd != (ssize_t)sizeof(ev))
			return;
		if (ev.type != EV_KEY)
			continue;
#if PSC_EJECT_DEBUG
		fprintf(stderr, "[eject] key code=%d (0x%x) value=%d\n",
			ev.code, ev.code, ev.value);
		fflush(stderr);
#endif
		if (ev.code == KEY_EJECTCD && ev.value == 1) {
			emu_action = SACTION_SWAP_CD;
			/* Break the CPU out of psxCpu->Execute() so the main loop
			 * notices the action immediately instead of waiting for the
			 * next vsync / event poll. */
			psxRegs.stop++;
		}
	}
}

void psc_eject_finish(void)
{
	if (eject_fd >= 0) {
		close(eject_fd);
		eject_fd = -1;
	}
}
