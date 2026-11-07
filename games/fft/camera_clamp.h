#ifndef FFT_CAMERA_CLAMP_H
#define FFT_CAMERA_CLAMP_H
#define CAM_MX 24                       // the margins (pixels) a unit's feet keep from the edges
#define CAM_MT 39                       // body, smaller round pointer and six pixels of headroom
#define CAM_MB 12                       // bottom margin while following the unit
static void cam_clamp(s16 *x, s16 *y)
{
    if (*x < 0) *x = 0;
    if (*x > SC_W - RT_W) *x = SC_W - RT_W;
    if (*y < -CAM_MT) *y = -CAM_MT;       // virtual blank rows preserve actor/number headroom
    if (*y > SC_H - RT_H) *y = SC_H - RT_H;
}

#endif
