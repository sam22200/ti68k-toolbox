/* tiemu-keyfix: LD_PRELOAD shim for TiEmu 3.04 (GTK 2) that stops keys from sticking.
 *
 * TiEmu keeps one pressed/released flag per TI key and only clears it on the matching GTK
 * event, so a lost release leaves the key held until it is pressed again:
 * - keyboard: a release that never reaches the window (focus moved away, ...). Every 50 ms we
 *   compare the PC keys TiEmu saw go down with the X server's real keyboard (XQueryKeymap) and
 *   replay the missing releases through TiEmu's own handler;
 * - mouse: TiEmu releases the skin key under the pointer at button release, none if the pointer
 *   slid off it. The release is moved back to where the button went down.
 * - keymap: TiEmu 3.04 matches hardware keycodes against the old XFree86 table (Up 98, Down 104,
 *   Left 100, Right 102...), while today's X servers send evdev codes (Up 111, Down 116...): the
 *   keys outside the main block are translated, and AltGr = 2nd, Right Ctrl = alpha (sent as the
 *   codes ti89.map binds to them: 64 = PCKEY_MENU -> 2ND, 66 = PCKEY_CAPITAL -> ALPHA).
 * Hook: gtk_init installs its dispatcher with gdk_event_handler_set (a call from libgtk into
 * libgdk, so it goes through the PLT): we wrap the dispatcher it installs.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

static GdkEventFunc real_func;
static gpointer real_data;
static GdkEvent *held[256];                  /* the key-press event of each PC key still down */
static gdouble press_x, press_y;

static const unsigned char evdev_to_xfree86[][2] = {
    { 111, 98 }, { 116, 104 }, { 113, 100 }, { 114, 102 },            /* arrows */
    { 110, 97 }, { 115, 103 }, { 112, 99 }, { 117, 105 }, { 118, 106 }, { 119, 107 },   /* Home End PgUp PgDn Ins Del */
    { 104, 108 }, { 106, 112 }, { 127, 110 }, { 107, 111 },           /* KP_Enter KP_Divide Pause Print */
    { 108, 64 }, { 105, 66 },                                         /* AltGr = 2nd, Right Ctrl = alpha */
};

static void on_event(GdkEvent *e, gpointer data)
{
    if (e->type == GDK_KEY_PRESS || e->type == GDK_KEY_RELEASE) {
        guint8 k = e->key.hardware_keycode & 255;    /* the real key, as XQueryKeymap sees it */
        unsigned i;
        for (i = 0; i < sizeof evdev_to_xfree86 / sizeof evdev_to_xfree86[0]; i++)
            if (evdev_to_xfree86[i][0] == k) { e->key.hardware_keycode = evdev_to_xfree86[i][1]; break; }
        if (e->type == GDK_KEY_PRESS && !held[k])
            held[k] = gdk_event_copy(e);
        else if (e->type == GDK_KEY_RELEASE && held[k]) {
            gdk_event_free(held[k]);
            held[k] = NULL;
        }
    } else if (e->type == GDK_BUTTON_PRESS && e->button.button == 1) {
        press_x = e->button.x; press_y = e->button.y;
    } else if (e->type == GDK_BUTTON_RELEASE && e->button.button == 1) {
        e->button.x = press_x; e->button.y = press_y;
    }
    real_func(e, data);
}

static gboolean check_keys(gpointer unused)
{
    char down[32];
    int k;
    GdkDisplay *d = gdk_display_get_default();
    if (!d || !real_func) return TRUE;
    XQueryKeymap(GDK_DISPLAY_XDISPLAY(d), down);
    for (k = 8; k < 256; k++)
        if (held[k] && !(down[k >> 3] & (1 << (k & 7)))) {
            GdkEvent *e = held[k];
            held[k] = NULL;
            e->type = GDK_KEY_RELEASE;
            real_func(e, real_data);
            gdk_event_free(e);
        }
    return TRUE;
}

void gdk_event_handler_set(GdkEventFunc func, gpointer data, GDestroyNotify notify)
{
    static void (*real_set)(GdkEventFunc, gpointer, GDestroyNotify);
    if (!real_set) real_set = dlsym(RTLD_NEXT, "gdk_event_handler_set");
    real_func = func;
    real_data = data;
    real_set(on_event, data, notify);
    g_timeout_add(50, check_keys, NULL);
}
