/* Event-order regressions for the preload shim. TiEmu reference-counts ALPHA, including
 * the implicit ALPHA attached to a PC letter: duplicate releases make it negative. */
#define _GNU_SOURCE
#include <stdio.h>
#include "tiemu-keyfix.c"

static int alpha, presses, releases, failures;
static gdouble mouse_x, mouse_y;

static void record(GdkEvent *e, gpointer unused)
{
    if (e->type == GDK_KEY_PRESS || e->type == GDK_KEY_RELEASE) {
        if (e->type == GDK_KEY_PRESS) { presses++; alpha++; }
        else { releases++; alpha--; }
    } else if (e->type == GDK_BUTTON_RELEASE) {
        mouse_x = e->button.x; mouse_y = e->button.y;
    }
}

static void key(GdkEventType type, guint16 code)
{
    GdkEvent e = { 0 };
    e.key.type = type; e.key.hardware_keycode = code;
    on_event(&e, NULL);
}

static void expect(const char *name, int actual, int wanted)
{
    if (actual != wanted) { printf("FAIL %s: %d, expected %d\n", name, actual, wanted); failures++; }
}

int main(int argc, char **argv)
{
    GdkEvent mouse = { 0 };
    int before;
    gdk_init(&argc, &argv);
    real_func = record; real_data = NULL;

    key(GDK_KEY_PRESS, 41); key(GDK_KEY_RELEASE, 41);
    expect("ordinary letter leaves ALPHA released", alpha, 0);
    key(GDK_KEY_PRESS, 41);
    check_keys(NULL); /* X server says up, GTK's actual release is still queued. */
    expect("watchdog releases implicit ALPHA", alpha, 0);
    key(GDK_KEY_RELEASE, 41);
    expect("late release cannot underflow ALPHA", alpha, 0);
    alpha = 0;

    key(GDK_KEY_PRESS, 105); /* remapped Right Ctrl: the lookup retains real keycode 105 */
    check_keys(NULL); key(GDK_KEY_RELEASE, 105);
    expect("late remapped release is also suppressed", alpha, 0);
    alpha = 0;

    before = presses;
    key(GDK_KEY_PRESS, 41); key(GDK_KEY_PRESS, 41); /* detectable autorepeat */
    expect("autorepeat still reaches GTK", presses - before, 2);
    expect("repeat does not add another ALPHA owner", alpha, 1);
    key(GDK_KEY_RELEASE, 41);
    expect("repeat followed by release clears ALPHA", alpha, 0);
    alpha = 0;

    key(GDK_KEY_PRESS, 41); key(GDK_KEY_PRESS, 42);
    expect("different simultaneous letters keep both owners", alpha, 2);
    key(GDK_KEY_RELEASE, 41); expect("second owner survives first release", alpha, 1);
    key(GDK_KEY_RELEASE, 42); expect("both released", alpha, 0);
    key(GDK_KEY_RELEASE, 42); expect("unmatched release is harmless", alpha, 0);

    mouse.button.type = GDK_BUTTON_PRESS; mouse.button.button = 1;
    mouse.button.x = 20; mouse.button.y = 30; on_event(&mouse, NULL);
    mouse.button.type = GDK_BUTTON_RELEASE;
    mouse.button.x = 200; mouse.button.y = 300; on_event(&mouse, NULL);
    expect("dragged skin release keeps press x", mouse_x, 20);
    expect("dragged skin release keeps press y", mouse_y, 30);

    if (failures) return 1;
    printf("Keyboard event order passed: %d presses, %d releases, ALPHA balanced; skin release preserved\n", presses, releases);
    return 0;
}
