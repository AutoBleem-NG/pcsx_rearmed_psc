/*
 * Minimal .m3u playlist parser for the standalone PSC frontend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "psc_m3u.h"

#define PSC_M3U_MAX_DISCS 8

static char *disc_paths[PSC_M3U_MAX_DISCS];
static int disc_count;
static int current_idx;

static int has_m3u_ext(const char *path)
{
	size_t n = path ? strlen(path) : 0;
	return n >= 4 && strcasecmp(path + n - 4, ".m3u") == 0;
}

static void clear(void)
{
	int i;
	for (i = 0; i < disc_count; i++) {
		free(disc_paths[i]);
		disc_paths[i] = NULL;
	}
	disc_count = 0;
	current_idx = 0;
}

const char *psc_m3u_load(const char *path)
{
	char line[1024];
	char abs[PATH_MAX];
	char dir[PATH_MAX];
	FILE *fp;
	const char *slash;

	clear();

	if (!has_m3u_ext(path))
		return NULL;

	fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "[m3u] open failed: %s\n", path);
		return NULL;
	}

	/* Directory of the m3u, used to resolve relative entries. */
	slash = strrchr(path, '/');
	if (slash) {
		size_t dn = (size_t)(slash - path);
		if (dn >= sizeof(dir)) dn = sizeof(dir) - 1;
		memcpy(dir, path, dn);
		dir[dn] = '\0';
	} else {
		dir[0] = '.';
		dir[1] = '\0';
	}

	while (fgets(line, sizeof(line), fp)) {
		char *p = line;
		char *cr;
		cr = strpbrk(p, "\r\n");
		if (cr) *cr = '\0';
		while (*p == ' ' || *p == '\t') p++;
		if (*p == '\0' || *p == '#')
			continue;
		if (p[0] == '/') {
			snprintf(abs, sizeof(abs), "%s", p);
		} else {
			snprintf(abs, sizeof(abs), "%s/%s", dir, p);
		}
		if (disc_count >= PSC_M3U_MAX_DISCS) {
			fprintf(stderr, "[m3u] hit max %d discs, ignoring rest\n",
				PSC_M3U_MAX_DISCS);
			break;
		}
		disc_paths[disc_count++] = strdup(abs);
	}

	fclose(fp);

	if (disc_count == 0) {
		fprintf(stderr, "[m3u] no usable entries in %s\n", path);
		return NULL;
	}

	fprintf(stderr, "[m3u] loaded %d disc(s) from %s\n", disc_count, path);
	{
		int i;
		for (i = 0; i < disc_count; i++)
			fprintf(stderr, "[m3u]   %d: %s\n", i + 1, disc_paths[i]);
	}
	fflush(stderr);

	current_idx = 0;
	return disc_paths[0];
}

int psc_m3u_active(void)        { return disc_count > 1; }
int psc_m3u_count(void)         { return disc_count; }
int psc_m3u_current(void)       { return current_idx; }

const char *psc_m3u_disc_path(int idx)
{
	if (idx < 0 || idx >= disc_count) return NULL;
	return disc_paths[idx];
}

const char *psc_m3u_peek_next(void)
{
	if (disc_count == 0) return NULL;
	return disc_paths[(current_idx + 1) % disc_count];
}

void psc_m3u_advance(void)
{
	if (disc_count == 0) return;
	current_idx = (current_idx + 1) % disc_count;
}
