/*
 * PlayStation Classic (PSC) Launcher Support
 * (C) 2025 AutoBleem-NG Team
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psc_launcher.h"
#include "../libpcsxcore/misc.h"  /* Config */
#include "libpicofe/plat.h"       /* plat_target */
#include "menu.h"                 /* g_scaler, SCALE_* */

void psc_settings_init(struct psc_settings *psc)
{
	psc->region = 0;  /* auto-detect */
	psc->filter = 0;  /* bilinear (smooth) */
	psc->ratio = 0;   /* 4:3 */
	psc->enter = 0;   /* O confirm (Japanese) */
	psc->has_region = 0;
	psc->has_filter = 0;
	psc->has_ratio = 0;
	psc->has_enter = 0;
}

int psc_parse_arg(struct psc_settings *psc, int argc, char **argv, int *i)
{
	int idx = *i;

	/* -pad1/-pad2: Controller device paths (ignored - SDL handles USB HID) */
	if (!strcmp(argv[idx], "-pad1") || !strcmp(argv[idx], "-pad2")) {
		if (idx + 1 < argc) {
			(*i)++;  /* skip the path argument */
		}
		return 1;
	}

	/* -region: 0=auto, 1=NTSC, 2=PAL */
	if (!strcmp(argv[idx], "-region")) {
		if (idx + 1 < argc) {
			psc->region = atoi(argv[++(*i)]);
			psc->has_region = 1;
		}
		return 1;
	}

	/* -filter: 0=bilinear ON, 1=bilinear OFF */
	if (!strcmp(argv[idx], "-filter")) {
		if (idx + 1 < argc) {
			psc->filter = atoi(argv[++(*i)]);
			psc->has_filter = 1;
		}
		return 1;
	}

	/* -ratio: 0=4:3, 1=16:9 */
	if (!strcmp(argv[idx], "-ratio")) {
		if (idx + 1 < argc) {
			psc->ratio = atoi(argv[++(*i)]);
			psc->has_ratio = 1;
		}
		return 1;
	}

	/* -enter: 0=O confirm, 1=X confirm */
	if (!strcmp(argv[idx], "-enter")) {
		if (idx + 1 < argc) {
			psc->enter = atoi(argv[++(*i)]);
			psc->has_enter = 1;
		}
		return 1;
	}

	return 0;  /* not a PSC argument */
}

void psc_apply_settings(const struct psc_settings *psc)
{
	/* Apply region setting */
	if (psc->has_region) {
		switch (psc->region) {
		case 1:  /* NTSC */
			menu_set_region(1);
			Config.PsxAuto = 0;
			Config.PsxType = 0;
			printf("PSC: Region set to NTSC\n");
			break;
		case 2:  /* PAL */
			menu_set_region(2);
			Config.PsxAuto = 0;
			Config.PsxType = 1;
			printf("PSC: Region set to PAL\n");
			break;
		default: /* Auto-detect */
			menu_set_region(0);
			Config.PsxAuto = 1;
			printf("PSC: Region set to Auto\n");
			break;
		}
	}

	/*
	 * Apply filter setting
	 * hwfilter: 0="linear" (bilinear), 1="nearest" (sharp pixels)
	 * PSC -filter: 0=bilinear ON, 1=bilinear OFF
	 * Maps directly: psc_filter -> hwfilter
	 */
	if (psc->has_filter) {
		plat_target.hwfilter = psc->filter ? 1 : 0;
		printf("PSC: Hardware filter set to %s\n",
			psc->filter ? "nearest (sharp)" : "linear (bilinear)");
	}

	/*
	 * Apply aspect ratio setting
	 * g_scaler: SCALE_4_3=2, SCALE_FULLSCREEN=4
	 * PSC -ratio: 0=4:3, 1=16:9 (fullscreen on 720p display)
	 */
	if (psc->has_ratio) {
		if (psc->ratio == 1) {
			g_scaler = SCALE_FULLSCREEN;
			printf("PSC: Aspect ratio set to 16:9 (fullscreen)\n");
		} else {
			g_scaler = SCALE_4_3;
			printf("PSC: Aspect ratio set to 4:3\n");
		}
	}

	/*
	 * Enter button mode (X/O swap) - not currently implemented
	 * Would need to swap PBTN_MOK bindings in input system
	 * PSC -enter: 0=O confirm (Japanese), 1=X confirm (Western)
	 */
	(void)psc->has_enter;
	(void)psc->enter;
}
