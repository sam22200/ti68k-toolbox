// Cold map replacement only. Work is discarded with its previous scene; avoid
// holding two ~54KiB blocks at once. A failed replacement leaves a retryable NULL.
static u8 scene_replace(const SceneBuffer *request,u16 colors)
{
    u8 **slot=request->slot;
    if(!colors||colors>FFT_CORE_SCENE_MAX||(colors&3))return 0;
    if(*slot&&*request->capacity==colors)return 1;
    if(*slot)free(*slot);
    *slot=RT_NULL;
    *slot=malloc((u32)colors+sizeof(Work));
    *request->capacity=*slot?colors:0;
    return *slot!=RT_NULL;
}
