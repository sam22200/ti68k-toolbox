/* tiemu-keyfix: LD_PRELOAD shim for TiEmu 3.04 (GTK 2) that stops keys from sticking.
 *
 * TiEmu keeps one pressed/released flag per TI key and only clears it on the matching GTK
 * event, so a lost release leaves the key held until it is pressed again:
 * - keyboard: a release that never reaches the window (focus moved away, ...). Every 50 ms we
 *   compare the PC keys TiEmu saw go down with the X server's real keyboard (XQueryKeymap) and
 *   replay the missing releases through TiEmu's own handler;
 * - mouse: TiEmu releases the skin key under the pointer at button release, none if the pointer
 *   slid off it. The release is moved back to where the button went down.
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

static void on_event(GdkEvent *e, gpointer data)
{
    if (e->type == GDK_KEY_PRESS && !held[e->key.hardware_keycode & 255])
        held[e->key.hardware_keycode & 255] = gdk_event_copy(e);
    else if (e->type == GDK_KEY_RELEASE && held[e->key.hardware_keycode & 255]) {
        gdk_event_free(held[e->key.hardware_keycode & 255]);
        held[e->key.hardware_keycode & 255] = NULL;
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
