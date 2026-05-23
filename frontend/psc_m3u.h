#ifndef __PSC_M3U_H__
#define __PSC_M3U_H__

/*
 * Minimal .m3u playlist support for the standalone PSC frontend.
 *
 * libretro has its own m3u parser; for our SDL build we keep a small
 * in-memory list of disc paths and use it to drive the eject->next-disc
 * swap that the menu's PBP-multidisk path can't handle for separate CHD
 * files.
 */

/* Parse `path` (must end in .m3u). Resolves entries relative to the m3u's
 * directory. Returns the path to disc 1 (caller passes that to
 * set_cd_image), or NULL on failure / non-m3u input. */
const char *psc_m3u_load(const char *path);

/* True if an m3u was loaded and has > 1 disc. */
int psc_m3u_active(void);

/* Number of discs (0 if no m3u). */
int psc_m3u_count(void);

/* Currently selected disc index, 0-based. */
int psc_m3u_current(void);

/* Returns the absolute path of disc `idx`, or NULL. */
const char *psc_m3u_disc_path(int idx);

/* Returns the absolute path of the next disc (wrapping), or NULL if no
 * m3u. Does NOT update the current index. */
const char *psc_m3u_peek_next(void);

/* Advances the current index (wrapping). */
void psc_m3u_advance(void);

#endif
