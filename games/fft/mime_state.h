// One original root is frozen until every imitation and response has settled.
#ifndef FFT_MIME_STATE_H
#define FFT_MIME_STATE_H
typedef struct {
    u16 pending,ability;
    s8 dx,dz;
    u8 actor,team,face,set,follow,ref,right,left;
    u8 active,who,saved_set,saved_right,saved_left;
} MimeState;
#endif
