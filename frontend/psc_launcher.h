/*
 * PlayStation Classic (PSC) Launcher Support
 * (C) 2025 AutoBleem-NG Team
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 *
 * Provides compatibility with PSC launcher command-line arguments:
 *   -region N   : 0=auto, 1=NTSC, 2=PAL
 *   -filter N   : 0=bilinear ON, 1=bilinear OFF (sharp pixels)
 *   -ratio N    : 0=4:3, 1=16:9 (fullscreen)
 *   -enter N    : 0=O confirm (JP), 1=X confirm (Western)
 *   -pad1 PATH  : USB controller 1 device path (unused, SDL handles)
 *   -pad2 PATH  : USB controller 2 device path (unused, SDL handles)
 */

#ifndef PSC_LAUNCHER_H
#define PSC_LAUNCHER_H

/* PSC launcher settings parsed from command line */
struct psc_settings {
	int region;    /* 0=auto, 1=NTSC, 2=PAL */
	int filter;    /* 0=bilinear, 1=nearest */
	int ratio;     /* 0=4:3, 1=16:9 */
	int enter;     /* 0=O confirm, 1=X confirm */
	int has_region;
	int has_filter;
	int has_ratio;
	int has_enter;
};

/* Initialize PSC settings to defaults */
void psc_settings_init(struct psc_settings *psc);

/*
 * Parse PSC-specific command line arguments.
 * Returns 1 if argument was handled, 0 if not a PSC argument.
 * Updates *i to skip consumed arguments.
 */
int psc_parse_arg(struct psc_settings *psc, int argc, char **argv, int *i);

/*
 * Apply PSC settings to emulator configuration.
 * Should be called after loading config file but before starting emulation.
 */
void psc_apply_settings(const struct psc_settings *psc);

#endif /* PSC_LAUNCHER_H */
