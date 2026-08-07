#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/event.h"

#ifdef CSKY_OS
#include "csi_kernel.h"

#define EVENT_MAGIC (0xa67b3cd4)

int32 os_event_init(os_event_t *evt)
{
    if(evt->magic == EVENT_MAGIC){
        os_printf(KERN_WARNING"event repeat initialization ????\r\n");
    }

    evt->hdl = csi_kernel_event_new();
    ASSERT(evt->hdl);
    if (evt->hdl) { 
        evt->magic = EVENT_MAGIC; 
    }
    return (evt->hdl ? RET_OK : RET_ERR);
}

int32 os_event_del(os_event_t *evt)
{
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    int32 ret = csi_kernel_event_del(evt->hdl);
    evt->hdl = NULL;
    evt->magic = 0;
    return ret;
}

int32 os_event_set(os_event_t *evt, uint32 flags, uint32 *rflags)
{
    uint32 _rflags;
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_set(evt->hdl, flags, (uint32_t *)rflags);
}

int32 os_event_clear(os_event_t *evt, uint32 flags, uint32 *rflags)
{
    uint32 _rflags;
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_clear(evt->hdl, flags, (uint32_t *)rflags);
}

int32 os_event_get(os_event_t *evt, uint32 *rflags)
{
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    return csi_kernel_event_get(evt->hdl, (uint32_t *)rflags);
}

int32 os_event_wait(os_event_t *evt, uint32 flags, uint32 *rflags, uint32 mode, int32 timeout)
{
    uint32 _rflags;
    k_event_opt_t options = KEVENT_OPT_SET_ALL;
    uint8_t clr_on_exit   = (mode & OS_EVENT_WMODE_CLEAR) ? 1 : 0;

    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if (mode & OS_EVENT_WMODE_OR) {
        options = KEVENT_OPT_SET_ANY;
    }
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_wait(evt->hdl, flags, options, clr_on_exit, (uint32_t *)rflags, timeout);
}

#endif

