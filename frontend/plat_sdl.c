/*
 * (C) Gražvydas "notaz" Ignotas, 2011-2013
 *
 * PlayStation Classic (PSC) SDL2 port for AutoBleem-NG
 * (C) 2025 AutoBleem-NG Team
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 *
 * PSC-specific changes from upstream pcsx_rearmed:
 * - SDL2 API: Replaces SDL1 with SDL2 (SDL_Window, SDL_Renderer, SDL_Texture)
 * - Wayland: Native Wayland/EGL/GLES rendering via libpicofe-psc
 * - Input: PSC controllers are USB HID joysticks (buttons 0-9 -> 0xF0-0xF9)
 * - Video: Fullscreen via SDL_WINDOW_FULLSCREEN_DESKTOP on Wayland
 */

#include <stdio.h>
#include <assert.h>
#include <SDL.h>

#include "../libpcsxcore/plugins.h"
#include "libpicofe/input.h"
#include "libpicofe/in_sdl.h"
#include "libpicofe/menu.h"
#include "libpicofe/fonts.h"
#include "libpicofe/plat_sdl.h"
#include "libpicofe/plat.h"
#include "libpicofe/gl.h"
#include "cspace.h"
#include "plugin_lib.h"
#include "plugin.h"
#include "menu.h"
#include "main.h"
#include "plat.h"
#include "psc_input.h"
#include "revision.h"

// Include the SDL2-ported plat_sdl.c implementation
#include "libpicofe/plat_sdl.c"

// SDL2 compatibility - define SDLK_JOY_BASE for joystick button keycodes
#define SDLK_JOY_BASE 400
#define SDLK_JOY_BUTTON(n) (SDLK_JOY_BASE + (n))

#ifndef PSC_EJECT_DEBUG
#define PSC_EJECT_DEBUG 0
#endif

#ifdef MIYOO
static const struct in_default_bind in_sdl_defbinds[] = {
  { SDL_SCANCODE_UP,        IN_BINDTYPE_PLAYER12, DKEY_UP },
  { SDL_SCANCODE_DOWN,      IN_BINDTYPE_PLAYER12, DKEY_DOWN },
  { SDL_SCANCODE_LEFT,      IN_BINDTYPE_PLAYER12, DKEY_LEFT },
  { SDL_SCANCODE_RIGHT,     IN_BINDTYPE_PLAYER12, DKEY_RIGHT },
  { SDL_SCANCODE_LSHIFT,    IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
  { SDL_SCANCODE_LCTRL,     IN_BINDTYPE_PLAYER12, DKEY_CROSS },
  { SDL_SCANCODE_LALT,      IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
  { SDL_SCANCODE_SPACE,     IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
  { SDL_SCANCODE_RETURN,    IN_BINDTYPE_PLAYER12, DKEY_START },
  { SDL_SCANCODE_ESCAPE,    IN_BINDTYPE_PLAYER12, DKEY_SELECT },
  { SDL_SCANCODE_TAB,       IN_BINDTYPE_PLAYER12, DKEY_L1 },
  { SDL_SCANCODE_BACKSPACE, IN_BINDTYPE_PLAYER12, DKEY_R1 },
  { SDL_SCANCODE_PAGEUP,    IN_BINDTYPE_PLAYER12, DKEY_L2 },
  { SDL_SCANCODE_PAGEDOWN,  IN_BINDTYPE_PLAYER12, DKEY_R2 },
  { SDL_SCANCODE_RALT,      IN_BINDTYPE_PLAYER12, DKEY_L3 },
  { SDL_SCANCODE_RSHIFT,    IN_BINDTYPE_PLAYER12, DKEY_R3 },
  { SDL_SCANCODE_RCTRL,     IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
  { 0, 0, 0 }
};

const struct menu_keymap in_sdl_key_map[] =
{
  { SDL_SCANCODE_UP,        PBTN_UP },
  { SDL_SCANCODE_DOWN,      PBTN_DOWN },
  { SDL_SCANCODE_LEFT,      PBTN_LEFT },
  { SDL_SCANCODE_RIGHT,     PBTN_RIGHT },
  { SDL_SCANCODE_LALT,      PBTN_MOK },
  { SDL_SCANCODE_LCTRL,     PBTN_MBACK },
  { SDL_SCANCODE_SPACE,     PBTN_MA2 },
  { SDL_SCANCODE_LSHIFT,    PBTN_MA3 },
  { SDL_SCANCODE_TAB,       PBTN_L },
  { SDL_SCANCODE_BACKSPACE, PBTN_R },
};
#else
static const struct in_default_bind in_sdl_defbinds[] = {
  /* Keyboard bindings */
  { SDL_SCANCODE_UP,     IN_BINDTYPE_PLAYER12, DKEY_UP },
  { SDL_SCANCODE_DOWN,   IN_BINDTYPE_PLAYER12, DKEY_DOWN },
  { SDL_SCANCODE_LEFT,   IN_BINDTYPE_PLAYER12, DKEY_LEFT },
  { SDL_SCANCODE_RIGHT,  IN_BINDTYPE_PLAYER12, DKEY_RIGHT },
  { SDL_SCANCODE_D,      IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
  { SDL_SCANCODE_Z,      IN_BINDTYPE_PLAYER12, DKEY_CROSS },
  { SDL_SCANCODE_X,      IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
  { SDL_SCANCODE_S,      IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
  { SDL_SCANCODE_V,      IN_BINDTYPE_PLAYER12, DKEY_START },
  { SDL_SCANCODE_C,      IN_BINDTYPE_PLAYER12, DKEY_SELECT },
  { SDL_SCANCODE_W,      IN_BINDTYPE_PLAYER12, DKEY_L1 },
  { SDL_SCANCODE_R,      IN_BINDTYPE_PLAYER12, DKEY_R1 },
  { SDL_SCANCODE_E,      IN_BINDTYPE_PLAYER12, DKEY_L2 },
  { SDL_SCANCODE_T,      IN_BINDTYPE_PLAYER12, DKEY_R2 },
  { SDL_SCANCODE_ESCAPE, IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
  { SDL_SCANCODE_F1,     IN_BINDTYPE_EMU, SACTION_SAVE_STATE },
  { SDL_SCANCODE_F2,     IN_BINDTYPE_EMU, SACTION_LOAD_STATE },
  { SDL_SCANCODE_F3,     IN_BINDTYPE_EMU, SACTION_PREV_SSLOT },
  { SDL_SCANCODE_F4,     IN_BINDTYPE_EMU, SACTION_NEXT_SSLOT },
  { SDL_SCANCODE_F5,     IN_BINDTYPE_EMU, SACTION_TOGGLE_FSKIP },
  { SDL_SCANCODE_F6,     IN_BINDTYPE_EMU, SACTION_SCREENSHOT },
  { SDL_SCANCODE_F7,     IN_BINDTYPE_EMU, SACTION_TOGGLE_FPS },
  { SDL_SCANCODE_F8,     IN_BINDTYPE_EMU, SACTION_SWITCH_DISPMODE },
  { SDL_SCANCODE_F11,    IN_BINDTYPE_EMU, SACTION_TOGGLE_FULLSCREEN },
  { SDL_SCANCODE_BACKSPACE, IN_BINDTYPE_EMU, SACTION_FAST_FORWARD },
  /* PSC physical eject button (KEY_EJECTCD -> SDL_SCANCODE_EJECT) */
  { SDL_SCANCODE_EJECT,  IN_BINDTYPE_EMU, SACTION_SWAP_CD },
  /* PlayStation Classic controller button bindings (see psc_input.h) */
  { PSC_KEY_TRIANGLE, IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
  { PSC_KEY_CIRCLE,   IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
  { PSC_KEY_CROSS,    IN_BINDTYPE_PLAYER12, DKEY_CROSS },
  { PSC_KEY_SQUARE,   IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
  { PSC_KEY_L2,       IN_BINDTYPE_PLAYER12, DKEY_L2 },
  { PSC_KEY_R2,       IN_BINDTYPE_PLAYER12, DKEY_R2 },
  { PSC_KEY_L1,       IN_BINDTYPE_PLAYER12, DKEY_L1 },
  { PSC_KEY_R1,       IN_BINDTYPE_PLAYER12, DKEY_R1 },
  { PSC_KEY_SELECT,   IN_BINDTYPE_PLAYER12, DKEY_SELECT },
  { PSC_KEY_START,    IN_BINDTYPE_PLAYER12, DKEY_START },
  /* Menu access via Select */
  { PSC_KEY_SELECT,   IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
  { 0, 0, 0 }
};

const struct menu_keymap in_sdl_key_map[] =
{
  { SDL_SCANCODE_UP,     PBTN_UP },
  { SDL_SCANCODE_DOWN,   PBTN_DOWN },
  { SDL_SCANCODE_LEFT,   PBTN_LEFT },
  { SDL_SCANCODE_RIGHT,  PBTN_RIGHT },
  { SDL_SCANCODE_RETURN, PBTN_MOK },
  { SDL_SCANCODE_ESCAPE, PBTN_MBACK },
  { SDL_SCANCODE_SEMICOLON,    PBTN_MA2 },
  { SDL_SCANCODE_APOSTROPHE,   PBTN_MA3 },
  { SDL_SCANCODE_LEFTBRACKET,  PBTN_L },
  { SDL_SCANCODE_RIGHTBRACKET, PBTN_R },
};
#endif

const struct menu_keymap in_sdl_joy_map[] =
{
  { SDL_SCANCODE_UP,    PBTN_UP },
  { SDL_SCANCODE_DOWN,  PBTN_DOWN },
  { SDL_SCANCODE_LEFT,  PBTN_LEFT },
  { SDL_SCANCODE_RIGHT, PBTN_RIGHT },
  /* PSC controller menu navigation (see psc_input.h) */
  { PSC_MENU_OK,      PBTN_MOK },   /* Cross = OK */
  { PSC_MENU_BACK,    PBTN_MBACK }, /* Circle = Back */
  { PSC_MENU_ACTION2, PBTN_MA2 },   /* Square */
  { PSC_MENU_ACTION3, PBTN_MA3 },   /* Triangle */
};

static const struct in_pdata in_sdl_platform_data = {
  .defbinds  = in_sdl_defbinds,
  .key_map   = in_sdl_key_map,
  .kmap_size = sizeof(in_sdl_key_map) / sizeof(in_sdl_key_map[0]),
  .joy_map   = in_sdl_joy_map,
  .jmap_size = sizeof(in_sdl_joy_map) / sizeof(in_sdl_joy_map[0]),
};

static int psx_w = 256, psx_h = 240;
static void *shadow_fb, *menubg_img;
static int vout_fullscreen_old;
static int forced_clears;
static int forced_flips;
static int sdl12_compat;
static int resized;
static int in_menu;

static int gl_w_prev, gl_h_prev, gl_quirks_prev;
static float gl_vertices[] = {
	-1.0f,  1.0f,  0.0f, // 0    0  1
	 1.0f,  1.0f,  0.0f, // 1  ^
	-1.0f, -1.0f,  0.0f, // 2  | 2  3
	 1.0f, -1.0f,  0.0f, // 3  +-->
};

static void handle_window_resize(void);
static void handle_scaler_resize(int w, int h);
static void centered_clear(void);

static int plugin_owns_display(void)
{
  // if true, a plugin is drawing and flipping
  return (pl_rearmed_cbs.gpu_caps & GPU_CAP_OWNS_DISPLAY);
}

static void plugin_update(void)
{
  // used by some plugins...
  pl_rearmed_cbs.screen_w = plat_sdl_screen->w;
  pl_rearmed_cbs.screen_h = plat_sdl_screen->h;
  pl_rearmed_cbs.gles_display = gl_es_display;
  pl_rearmed_cbs.gles_surface = gl_es_surface;
  plugin_call_rearmed_cbs();
}

static void sdl_event_handler(void *event_)
{
  SDL_Event *event = event_;

#if PSC_EJECT_DEBUG
  if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
    fprintf(stderr, "[eject] SDL_KEY%s sym=0x%x scan=0x%x (EJECT=0x%x)\n",
        event->type == SDL_KEYDOWN ? "DOWN" : "UP",
        event->key.keysym.sym, event->key.keysym.scancode,
        SDL_SCANCODE_EJECT);
    fflush(stderr);
  }
#endif

  switch (event->type) {
  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_RESIZED ||
        event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      int new_w = event->window.data1 & ~3;
      int new_h = event->window.data2 & ~1;
      if (window_w != new_w || window_h != new_h) {
        window_w = new_w;
        window_h = new_h;
        resized = 1;
        if (!in_menu && plat_sdl_gl_active && plugin_owns_display()) {
          // the plugin flips by itself so resize has to be handled here
          handle_window_resize();
          if (GPU_open != NULL) {
            int ret = GPU_open(&gpuDisp, "PCSX", NULL);
            if (ret)
              fprintf(stderr, "GPU_open: %d\n", ret);
          }
        }
      }
    }
    // SDL_WINDOWEVENT_FOCUS_GAINED/LOST handled in plat_sdl_event_handler
    break;
  default:
    break;
  }
  plat_sdl_event_handler(event_);
}

static void quit_cb(void)
{
  emu_core_ask_exit();
}

static void get_layer_pos(int *x, int *y, int *w, int *h)
{
  // always fill entire SDL window
  *x = *y = 0;
  *w = pl_rearmed_cbs.screen_w;
  *h = pl_rearmed_cbs.screen_h;
}

void plat_init(void)
{
  static const char *hwfilters[] = { "linear", "nearest", NULL };
  SDL_version ver;
  int shadow_size;
  int ret;

  plat_sdl_quit_cb = quit_cb;

  old_fullscreen = -1; // hack

  ret = plat_sdl_init();
  if (ret != 0)
    exit(1);

  SDL_GetVersion(&ver);
  sdl12_compat = ver.patch >= 50;
  printf("SDL %u.%u.%u compat=%d\n", ver.major, ver.minor, ver.patch, sdl12_compat);

  in_menu = 1;
  SDL_SetWindowTitle(plat_sdl_window, "PCSX-ReARMed " REV);

  shadow_size = g_menuscreen_w * g_menuscreen_h * 2;
  // alloc enough for double res. rendering
  if (shadow_size < 1024 * 512 * 2)
    shadow_size = 1024 * 512 * 2;

  shadow_fb = malloc(shadow_size);
  menubg_img = malloc(shadow_size);
  if (shadow_fb == NULL || menubg_img == NULL) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }

  in_sdl_init(&in_sdl_platform_data, sdl_event_handler);
  in_probe();
  pl_rearmed_cbs.only_16bpp = 1;
  pl_rearmed_cbs.pl_get_layer_pos = get_layer_pos;

  bgr_to_uyvy_init();

  assert(plat_sdl_screen);
  plugin_update();
  if (plat_target.vout_method == vout_mode_gl)
    gl_w_prev = plat_sdl_screen->w, gl_h_prev = plat_sdl_screen->h;
  if (vout_mode_gl != -1)
    plat_target.hwfilters = hwfilters;
}

void plat_finish(void)
{
  free(shadow_fb);
  shadow_fb = NULL;
  free(menubg_img);
  menubg_img = NULL;
  plat_sdl_finish();
}

void plat_gvideo_open(int is_pal)
{
}

static void uyvy_to_rgb565(void *d, int pixels)
{
  const unsigned int *src = (const void *)plat_sdl_overlay->pixels[0];
  int x2 = plat_sdl_overlay->w >= psx_w * 2;
  unsigned short *dst = d;
  int v;

  // no colors, for now
  if (x2) {
    for (; pixels > 0; src++, dst++, pixels--) {
      v = (*src >> 8) & 0xff;
      v = (v - 16) * 255 / 219 / 8;
      *dst = (v << 11) | (v << 6) | v;
    }
  }
  else {
    for (; pixels > 0; src++, dst += 2, pixels -= 2) {
      v = (*src >> 8) & 0xff;
      v = (v - 16) * 255 / 219 / 8;
      dst[0] = (v << 11) | (v << 6) | v;

      v = (*src >> 24) & 0xff;
      v = (v - 16) * 255 / 219 / 8;
      dst[1] = (v << 11) | (v << 6) | v;
    }
  }
}

static void overlay_resize(int force)
{
  int x2_mul = !in_menu && plat_target.vout_method > 1 ? 2 : 1; // lame
  int w = in_menu ? g_menuscreen_w : psx_w;
  int h = in_menu ? g_menuscreen_h : psx_h;

  if (!force && plat_sdl_overlay && w * x2_mul == plat_sdl_overlay->w
      && h == plat_sdl_overlay->h)
    return;

  // Free old overlay
  if (plat_sdl_overlay) {
    free(plat_sdl_overlay->pixels[0]);
    free(plat_sdl_overlay);
    plat_sdl_overlay = NULL;
  }

  // Create new overlay
  plat_sdl_overlay = calloc(1, sizeof(*plat_sdl_overlay));
  if (plat_sdl_overlay) {
    plat_sdl_overlay->w = w * x2_mul;
    plat_sdl_overlay->h = h;
    plat_sdl_overlay->format = SDL_PIXELFORMAT_UYVY;
    plat_sdl_overlay->pitches[0] = w * x2_mul * 2;
    plat_sdl_overlay->pixels[0] = malloc(w * x2_mul * h * 2);
    plat_sdl_overlay->hw_overlay = 1;

    if (plat_sdl_overlay->pixels[0]) {
      // Create YUV texture
      if (plat_sdl_texture)
        SDL_DestroyTexture(plat_sdl_texture);
      plat_sdl_texture = SDL_CreateTexture(plat_sdl_renderer,
        SDL_PIXELFORMAT_UYVY, SDL_TEXTUREACCESS_STREAMING, w * x2_mul, h);

      if ((uintptr_t)plat_sdl_overlay->pixels[0] & 3)
        fprintf(stderr, "warning: overlay pointer is unaligned\n");

      plat_sdl_overlay_clear();
    }
    else {
      free(plat_sdl_overlay);
      plat_sdl_overlay = NULL;
    }
  }

  if (!plat_sdl_overlay) {
    fprintf(stderr, "overlay resize to %dx%d failed\n", w, h);
    plat_target.vout_method = 0;
  }
  handle_scaler_resize(w, h);
}

static void overlay_blit(int doffs, const void *src_, int w, int h,
                         int sstride, int bgr24)
{
  const unsigned short *src = src_;
  unsigned short *dst;
  int dstride = plat_sdl_overlay->w;
  int x2 = dstride >= 2 * w;

  dst = (void *)plat_sdl_overlay->pixels[0];

  dst += doffs;
  if (bgr24) {
    for (; h > 0; dst += dstride, src += sstride, h--)
      bgr888_to_uyvy(dst, src, w, x2);
  }
  else {
    for (; h > 0; dst += dstride, src += sstride, h--)
      bgr555_to_uyvy(dst, src, w, x2);
  }
}

static void overlay_hud_print(int x, int y, const char *str, int bpp)
{
  int x2;
  x2 = plat_sdl_overlay->w >= psx_w * 2;
  if (x2)
    x *= 2;
  basic_text_out_uyvy_nf(plat_sdl_overlay->pixels[0], plat_sdl_overlay->w, x, y, str);
}

static void gl_finish_pl(void)
{
  if (plugin_owns_display() && GPU_close != NULL)
    GPU_close();
  gl_destroy();
}

static void gl_resize(void)
{
  int w = in_menu ? g_menuscreen_w : psx_w;
  int h = in_menu ? g_menuscreen_h : psx_h;
  int ret;

  gl_quirks &= ~(GL_QUIRK_SCALING_NEAREST | GL_QUIRK_VSYNC_ON);
  if (plat_target.hwfilter) // inverted from plat_sdl_gl_scaling()
    gl_quirks |= GL_QUIRK_SCALING_NEAREST;
  if (g_opts & OPT_VSYNC)
    gl_quirks |= GL_QUIRK_VSYNC_ON;

  if (plugin_owns_display()) {
    w = plat_sdl_screen->w, h = plat_sdl_screen->h;
  }
  if (plat_sdl_gl_active) {
    if (w == gl_w_prev && h == gl_h_prev && gl_quirks == gl_quirks_prev) {
      return;
    }
    gl_finish_pl();
  }
  ret = gl_create(plat_sdl_window, &gl_quirks, w, h);
  plat_sdl_gl_active = (ret == 0);
  if (plat_sdl_gl_active)
    gl_w_prev = w, gl_h_prev = h, gl_quirks_prev = gl_quirks;
  else {
    fprintf(stderr, "warning: could not init GL.\n");
    plat_target.vout_method = 0;
  }
  handle_scaler_resize(w, h);
  plugin_update();
  forced_flips = 0; // interferes with gl
}

static void overlay_or_gl_check_enable(void)
{
  int ovl_on = plat_target.vout_method == vout_mode_overlay ||
    plat_target.vout_method == vout_mode_overlay2x;
  int gl_on = plat_target.vout_method == vout_mode_gl;
  if (!gl_on && plat_sdl_gl_active) {
    gl_finish_pl();
    pl_rearmed_cbs.gles_display = gl_es_display;
    pl_rearmed_cbs.gles_surface = gl_es_surface;
    plat_sdl_gl_active = 0;
  }
  if (!ovl_on && plat_sdl_overlay) {
    free(plat_sdl_overlay->pixels[0]);
    free(plat_sdl_overlay);
    plat_sdl_overlay = NULL;
  }
  if (ovl_on)
    overlay_resize(0);
  else if (gl_on)
    gl_resize();
}

static void centered_clear(void)
{
  int dstride = plat_sdl_screen->pitch / 2;
  int w = plat_sdl_screen->w;
  int h = plat_sdl_screen->h;
  unsigned short *dst;

  if (plat_sdl_gl_active) {
    gl_clear();
    return;
  }

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_LockSurface(plat_sdl_screen);
  dst = plat_sdl_screen->pixels;

  for (; h > 0; dst += dstride, h--)
    memset(dst, 0, w * 2);

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_UnlockSurface(plat_sdl_screen);

  if (plat_sdl_overlay != NULL) {
    // apply the parts not covered by the overlay
    forced_flips = 3;
  }
}

static int adj_src_dst(const SDL_Surface *sfc, int w, int pp, int *h,
    unsigned short **dst, const unsigned short **src)
{
  int line_w = w;
  if (sfc->w > w)
    *dst += (sfc->w - w) / 2;
  else {
    *src += (w - sfc->w) / 2;
    line_w = sfc->w;
  }
  if (sfc->h > *h)
    *dst += sfc->pitch * (sfc->h - *h) / 2 / 2;
  else {
    *src += pp * (*h - sfc->h) / 2;
    *h = sfc->h;
  }
  return line_w;
}

static void centered_blit(int doffs, const void *src_, int w, int h,
                          int sstride, int bgr24)
{
  const unsigned short *src = src_;
  unsigned short *dst;
  int dstride;

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_LockSurface(plat_sdl_screen);
  dst = plat_sdl_screen->pixels;
  dstride = plat_sdl_screen->pitch / 2;
  w = adj_src_dst(plat_sdl_screen, w, sstride, &h, &dst, &src);

  if (bgr24) {
    for (; h > 0; dst += dstride, src += sstride, h--)
      bgr888_to_rgb565(dst, src, w);
  }
  else {
    for (; h > 0; dst += dstride, src += sstride, h--)
      bgr555_to_rgb565(dst, src, w);
  }

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_UnlockSurface(plat_sdl_screen);
}

static void centered_blit_menu(void)
{
  const unsigned short *src = g_menuscreen_ptr;
  int w = g_menuscreen_w;
  int h = g_menuscreen_h;
  unsigned short *dst;
  int dstride, len;

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_LockSurface(plat_sdl_screen);

  dst = plat_sdl_screen->pixels;
  dstride = plat_sdl_screen->pitch / 2;
  len = adj_src_dst(plat_sdl_screen, w, g_menuscreen_pp, &h, &dst, &src);

  for (; h > 0; dst += dstride, src += g_menuscreen_pp, h--)
    memcpy(dst, src, len * 2);

  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_UnlockSurface(plat_sdl_screen);
}

static void centered_hud_print(int x, int y, const char *str, int bpp)
{
  int w_diff, h_diff;
  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_LockSurface(plat_sdl_screen);
  w_diff = plat_sdl_screen->w - psx_w;
  h_diff = plat_sdl_screen->h - psx_h;
  if (w_diff > 0) x += w_diff / 2;
  if (h_diff > 0) y += h_diff / 2;
  if (h_diff < 0) y += h_diff;
  if (w_diff < 0 && x > 32) x += w_diff;
  basic_text_out16_nf(plat_sdl_screen->pixels, plat_sdl_screen->pitch / 2, x, y, str);
  if (SDL_MUSTLOCK(plat_sdl_screen))
    SDL_UnlockSurface(plat_sdl_screen);
}

static void *setup_blit_callbacks(int w, int h)
{
  void *ret = NULL;
  const char *mode = "unknown";

  pl_plat_clear = NULL;
  pl_plat_blit = NULL;
  pl_plat_hud_print = NULL;
  if (plat_sdl_overlay != NULL) {
    pl_plat_clear = plat_sdl_overlay_clear;
    pl_plat_blit = overlay_blit;
    pl_plat_hud_print = overlay_hud_print;
    mode = "overlay";
    ret = NULL;
  }
  else if (plat_sdl_gl_active) {
    mode = "GL";
    ret = shadow_fb;
  }
  else {
    pl_plat_clear = centered_clear;

    if (!SDL_MUSTLOCK(plat_sdl_screen) && w == plat_sdl_screen->w &&
        h == plat_sdl_screen->h) {
      mode = "direct";
      ret = plat_sdl_screen->pixels;
    } else {
      pl_plat_blit = centered_blit;
      pl_plat_hud_print = centered_hud_print;
      mode = "centered";
      ret = NULL;
    }
  }
  printf("setup_blit_callbacks: %dx%d mode=%s ret=%p gl_active=%d overlay=%p\n",
         w, h, mode, ret, plat_sdl_gl_active, (void *)plat_sdl_overlay);
  return ret;
}

// not using plat_sdl_change_video_mode() since we need
// different size overlay vs plat_sdl_screen layer
static void change_mode(int w, int h)
{
  int set_w = w, set_h = h, had_overlay = 0, had_gl = 0;
  if (plat_target.vout_fullscreen && (plat_target.vout_method != 0 || !sdl12_compat))
    set_w = fs_w, set_h = fs_h;
  if (plat_sdl_screen->w != set_w || plat_sdl_screen->h != set_h ||
      plat_target.vout_fullscreen != vout_fullscreen_old)
  {
    if (plat_sdl_overlay) {
      free(plat_sdl_overlay->pixels[0]);
      free(plat_sdl_overlay);
      plat_sdl_overlay = NULL;
      had_overlay = 1;
    }
    if (plat_sdl_gl_active) {
      gl_finish_pl();
      plat_sdl_gl_active = 0;
      had_gl = 1;
    }
    SDL_PumpEvents();

    // SDL2: resize window instead of SDL_SetVideoMode
    SDL_SetWindowSize(plat_sdl_window, set_w, set_h);
    if (plat_target.vout_fullscreen)
      SDL_SetWindowFullscreen(plat_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else {
      SDL_SetWindowFullscreen(plat_sdl_window, 0);
    }

    // Recreate screen surface
    if (plat_sdl_screen)
      SDL_FreeSurface(plat_sdl_screen);
    plat_sdl_screen = SDL_CreateRGBSurface(0, set_w, set_h, 16,
      0xf800, 0x07e0, 0x001f, 0);

    // Recreate texture
    if (plat_sdl_texture)
      SDL_DestroyTexture(plat_sdl_texture);
    plat_sdl_texture = SDL_CreateTexture(plat_sdl_renderer,
      SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, set_w, set_h);

    assert(plat_sdl_screen);
    if (vout_fullscreen_old && !plat_target.vout_fullscreen)
      SDL_SetWindowGrab(plat_sdl_window, SDL_FALSE);

    // overlay needs the latest plat_sdl_screen
    if (had_overlay)
      overlay_resize(1);
    if (had_gl)
      gl_resize();
    centered_clear();
    plugin_update();
    vout_fullscreen_old = plat_target.vout_fullscreen;
  }
}

static void handle_scaler_resize(int w, int h)
{
  int ww = plat_sdl_screen->w;
  int wh = plat_sdl_screen->h;
  int layer_w_old = g_layer_w;
  int layer_h_old = g_layer_h;
  float w_mul, h_mul;
  int x, y;
  pl_update_layer_size(w, h, ww, wh);
  if (layer_w_old != g_layer_w || layer_h_old != g_layer_h)
    forced_clears = 3;

  w_mul = 2.0f / ww;
  h_mul = 2.0f / wh;
  x = (ww - g_layer_w) / 2;
  y = (wh - g_layer_h) / 2;
  gl_vertices[3*0+0] = gl_vertices[3*2+0] = -1.0f + x * w_mul;
  gl_vertices[3*1+0] = gl_vertices[3*3+0] = -1.0f + (x + g_layer_w) * w_mul;
  gl_vertices[3*2+1] = gl_vertices[3*3+1] = -1.0f + y * h_mul;
  gl_vertices[3*0+1] = gl_vertices[3*1+1] = -1.0f + (y + g_layer_h) * h_mul;
}

static void handle_window_resize(void)
{
  // sdl12-compat: a hack to take advantage of sdl2 scaling
  if (resized && (plat_target.vout_method != 0 || !sdl12_compat)) {
    change_mode(window_w, window_h);
    setup_blit_callbacks(psx_w, psx_h);
    forced_clears = 3;
  }
  resized = 0;
}

void *plat_gvideo_set_mode(int *w, int *h, int *bpp)
{
  psx_w = *w;
  psx_h = *h;

  if (plat_sdl_gl_active && plugin_owns_display())
    return NULL;

  if (plat_sdl_overlay != NULL)
    overlay_resize(0);
  else if (plat_sdl_gl_active) {
    memset(shadow_fb, 0, (*w) * (*h) * 2);
    gl_resize();
  }
  else if (plat_target.vout_method == 0) // && sdl12_compat
    change_mode(*w, *h);

  handle_scaler_resize(*w, *h); // override the value from pl_vout_set_mode()
  return setup_blit_callbacks(*w, *h);
}

void *plat_gvideo_flip(void)
{
  void *ret = NULL;
  int do_flip = 0;
  if (plat_sdl_overlay != NULL) {
    SDL_Rect dstrect = {
      (plat_sdl_screen->w - g_layer_w) / 2,
      (plat_sdl_screen->h - g_layer_h) / 2,
      g_layer_w, g_layer_h
    };
    // SDL2: Update texture and render
    SDL_UpdateTexture(plat_sdl_texture, NULL,
      plat_sdl_overlay->pixels[0], plat_sdl_overlay->pitches[0]);
    SDL_RenderClear(plat_sdl_renderer);
    SDL_RenderCopy(plat_sdl_renderer, plat_sdl_texture, NULL, &dstrect);
    SDL_RenderPresent(plat_sdl_renderer);
  }
  else if (plat_sdl_gl_active) {
    gl_flip_v(shadow_fb, psx_w, psx_h, g_scaler != SCALE_FULLSCREEN ? gl_vertices : NULL);
    ret = shadow_fb;
  }
  else
    do_flip |= 2;

  if (forced_flips > 0) {
    forced_flips--;
    do_flip |= 1;
  }
  if (do_flip) {
    // SDL2: Update texture and present
    SDL_UpdateTexture(plat_sdl_texture, NULL,
      plat_sdl_screen->pixels, plat_sdl_screen->pitch);
    SDL_RenderClear(plat_sdl_renderer);
    SDL_RenderCopy(plat_sdl_renderer, plat_sdl_texture, NULL, NULL);
    SDL_RenderPresent(plat_sdl_renderer);
  }
  handle_window_resize();
  if (do_flip) {
    if (forced_clears > 0) {
      forced_clears--;
      centered_clear();
    }
    if (!SDL_MUSTLOCK(plat_sdl_screen) && plat_sdl_screen->w == psx_w &&
        plat_sdl_screen->h == psx_h && (do_flip & 2)) {
      ret = plat_sdl_screen->pixels;
    }
  }
  assert(ret || pl_plat_clear != NULL);
  return ret;
}

void plat_gvideo_close(void)
{
}

void plat_video_menu_enter(int is_rom_loaded)
{
  int d;

  in_menu = 1;

  /* surface will be lost, must adjust pl_vout_buf for menu bg */
  if (plat_sdl_overlay != NULL)
    uyvy_to_rgb565(menubg_img, psx_w * psx_h);
  else if (plat_sdl_gl_active)
    memcpy(menubg_img, shadow_fb, psx_w * psx_h * 2);
  else {
    unsigned short *dst = menubg_img;
    const unsigned short *src;
    int h;
    if (SDL_MUSTLOCK(plat_sdl_screen))
      SDL_LockSurface(plat_sdl_screen);
    src = plat_sdl_screen->pixels;
    src += (plat_sdl_screen->w - psx_w) / 2;
    src += plat_sdl_screen->pitch * (plat_sdl_screen->h - psx_h) / 2 / 2;
    for (h = psx_h; h > 0; dst += psx_w, src += plat_sdl_screen->pitch / 2, h--)
      memcpy(dst, src, psx_w * 2);
    if (SDL_MUSTLOCK(plat_sdl_screen))
      SDL_UnlockSurface(plat_sdl_screen);
  }
  pl_vout_buf = menubg_img;

  if (plat_target.vout_method == 0)
    change_mode(g_menuscreen_w, g_menuscreen_h);
  else
    overlay_or_gl_check_enable();
  centered_clear();

  for (d = 0; d < IN_MAX_DEVS; d++)
    in_set_config_int(d, IN_CFG_ANALOG_MAP_ULDR, 1);
}

void plat_video_menu_begin(void)
{
  struct sdl2_overlay *old_ovl = plat_sdl_overlay;
  static int g_scaler_old;
  int scaler_changed = g_scaler_old != g_scaler;
  g_scaler_old = g_scaler;
  if (plat_target.vout_fullscreen != vout_fullscreen_old ||
      (plat_target.vout_fullscreen && scaler_changed)) {
    change_mode(g_menuscreen_w, g_menuscreen_h);
  }
  overlay_or_gl_check_enable();
  handle_scaler_resize(g_menuscreen_w, g_menuscreen_h);

  if (old_ovl != plat_sdl_overlay || scaler_changed)
    centered_clear();
  g_menuscreen_ptr = shadow_fb;
}

void plat_video_menu_end(void)
{
  int do_flip = 0;

  if (plat_sdl_overlay != NULL) {
    SDL_Rect dstrect = {
      (plat_sdl_screen->w - g_layer_w) / 2,
      (plat_sdl_screen->h - g_layer_h) / 2,
      g_layer_w, g_layer_h
    };

    rgb565_to_uyvy(plat_sdl_overlay->pixels[0], shadow_fb,
      g_menuscreen_w * g_menuscreen_h);

    // SDL2: Update texture and render
    SDL_UpdateTexture(plat_sdl_texture, NULL,
      plat_sdl_overlay->pixels[0], plat_sdl_overlay->pitches[0]);
    SDL_RenderClear(plat_sdl_renderer);
    SDL_RenderCopy(plat_sdl_renderer, plat_sdl_texture, NULL, &dstrect);
    SDL_RenderPresent(plat_sdl_renderer);
  }
  else if (plat_sdl_gl_active) {
    gl_flip_v(g_menuscreen_ptr, g_menuscreen_w, g_menuscreen_h,
        g_scaler != SCALE_FULLSCREEN ? gl_vertices : NULL);
  }
  else {
    centered_blit_menu();
    do_flip |= 2;
  }

  if (forced_flips > 0) {
    forced_flips--;
    do_flip |= 1;
  }
  if (do_flip) {
    // SDL2: Update and present
    SDL_UpdateTexture(plat_sdl_texture, NULL,
      plat_sdl_screen->pixels, plat_sdl_screen->pitch);
    SDL_RenderClear(plat_sdl_renderer);
    SDL_RenderCopy(plat_sdl_renderer, plat_sdl_texture, NULL, NULL);
    SDL_RenderPresent(plat_sdl_renderer);
  }

  handle_window_resize();
  g_menuscreen_ptr = NULL;
}

void plat_video_menu_leave(void)
{
  int d;

  in_menu = 0;

  // PSC-style: stay in GL mode, don't switch video modes
  // Just clear the shadow buffer and set up callbacks
  if (plat_sdl_gl_active)
    memset(shadow_fb, 0, g_menuscreen_w * g_menuscreen_h * 2);

  // Ensure GL is resized for game resolution
  if (plat_sdl_gl_active)
    gl_resize();

  setup_blit_callbacks(psx_w, psx_h);

  /* Keep ULDR mapping enabled for PSC controllers that use d-pad as axes.
   * Original code disabled this for analog stick games, but PSC controllers
   * don't have real analog sticks - the d-pad IS reported as axes. */
  for (d = 0; d < IN_MAX_DEVS; d++)
    in_set_config_int(d, IN_CFG_ANALOG_MAP_ULDR, 1);

}

void *plat_prepare_screenshot(int *w, int *h, int *bpp)
{
  if (plat_sdl_screen && !SDL_MUSTLOCK(plat_sdl_screen) &&
      plat_sdl_overlay == NULL && !plat_sdl_gl_active)
  {
    *w = plat_sdl_screen->pitch / 2;
    *h = plat_sdl_screen->h;
    *bpp = 16;
    return plat_sdl_screen->pixels;
  }
  fprintf(stderr, "screenshot not implemented in current mode\n");
  return NULL;
}

void plat_trigger_vibrate(int pad, int low, int high)
{
}

void plat_minimize(void)
{
}

// vim:shiftwidth=2:expandtab
