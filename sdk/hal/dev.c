#include "basic_include.h"

struct dev_mgr {
    struct dev_obj  *devs;
    struct dev_hotplug_obj  *hotplug;
    os_mutex_t       mutex;
};

__bobj static struct dev_mgr s_dev_mgr;

__init int32 dev_init()
{
    os_mutex_init(&s_dev_mgr.mutex);
    s_dev_mgr.devs = NULL;
    s_dev_mgr.hotplug = NULL;
    return RET_OK;
}

void dev_put(struct dev_obj *dev)
{
    if (dev && dev->hotplug) {
        if(atomic_dec_and_test(&dev->ref)){
            os_free(dev);
        }
    }
}

struct dev_obj *dev_get(uint16 dev_id)
{
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.devs;
    while (dev) {
        if (dev->dev_id == dev_id) {
            break;
        }
        dev = dev->next;
    }

    if (dev && dev->hotplug) {
        atomic_inc(&dev->ref);
        if (atomic_read(&dev->ref) > 32) {
            os_printf(KERN_WARNING"Device %d Ref is %d, Maybe device reference exception!!!\r\n", dev_id, atomic_read(&dev->ref));
        }
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return dev;
}

void dev_busy(struct dev_obj *dev, uint8 busy, uint8 set)
{
    uint32 flag = disable_irq();
    if (set) {
        dev->busy |= busy;
    } else {
        dev->busy &= ~busy;
    }
    enable_irq(flag);
}

__init int32 dev_register(uint16 dev_id, struct dev_obj *device)
{
    struct dev_obj *dev;

    if (dev_id == 0 || device == NULL) {
        return -EINVAL;
    }

    if (atomic_read(&device->ref) == 0) {
        atomic_set(&device->ref, 1); //未初始化
    }

    dev = dev_get(dev_id);
    if (dev) {
        dev_put(dev);
        os_printf(KERN_WARNING"Device id %d already exists!!\r\n", dev_id);
        return -EEXIST;
    }

#ifdef CONFIG_SLEEP
    os_blklist_init(&device->blklist);
#endif
    device->dev_id = dev_id;
    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    device->next = s_dev_mgr.devs;
    s_dev_mgr.devs = device;
    atomic_inc(&device->ref);
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

int32 dev_unregister(struct dev_obj *device)
{
    struct dev_obj *dev = NULL;
    struct dev_obj *last = NULL;

    if (device == NULL) {
        return RET_OK;
    }

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.devs;
    while (dev) {
        if (dev == device) {
            if (device == s_dev_mgr.devs) {
                s_dev_mgr.devs = device->next;
            } else {
                last->next = device->next;
            }
#ifdef CONFIG_SLEEP
            os_blklist_del(&device->blklist);
#endif
            break;
        }
        last = dev;
        dev  = dev->next;
    }

    dev_put(device);
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

#ifdef CONFIG_SLEEP
int32 dev_suspended(struct dev_obj *dev, uint8 suspend)
{
    if (dev->suspend && suspend && !__in_interrupt()) { /* 非中断上下文，直接挂起当前task */
        os_printf(KERN_WARNING"device %d has been suspended!\r\n", dev->dev_id);
        os_blklist_suspend(&dev->blklist, NULL);
    }
    return dev->suspend;
}

__weak int32 dev_suspend_hook(struct dev_obj *dev, uint16 type)
{
    return 1;
}
int32 dev_suspend(uint16 type)
{
    int32 loop;
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.devs;
    while (dev) {
        if (dev->ops && dev->ops->suspend && !dev->suspend) {
            if (dev_suspend_hook(dev, type)) {
                loop = 0;
                dev->suspend = 1;
                while (dev->busy) {
                    os_sleep_ms(1);
                    if (loop++ > 100) {
                        loop = 0;
                        os_printf(KERN_WARNING"device %d busy !!!!\r\n", dev->dev_id);
                    }
                }
                dev->ops->suspend(dev);
            }
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

__weak int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason)
{
    return 1;
}
int32 dev_resume(uint16 type, uint32 wkreason)
{
    struct dev_obj *dev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.devs;
    while (dev) {
        if (dev->ops && dev->ops->resume && dev->suspend) {
            if (dev_resume_hook(dev, type, wkreason)) {
                dev->ops->resume(dev);
                dev->suspend = 0;
                os_blklist_resume(&dev->blklist);//唤醒所有被挂起的task
            }
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}
#else
int32 dev_suspended(struct dev_obj *dev, uint8 suspend)
{
    return 0;
}

__weak int32 dev_suspend_hook(struct dev_obj *dev, uint16 type)
{
    return 1;
}
int32 dev_suspend(uint16 type)
{
    return RET_ERR;
}

__weak int32 dev_resume_hook(struct dev_obj *dev, uint16 type, uint32 wkreason)
{
    return 1;
}
int32 dev_resume(uint16 type, uint32 wkreason)
{
    return RET_ERR;
}

#endif

int32 dev_hotplug_in(void *hdl, uint16 dev_id, uint16 dev_type, void *info)
{
    struct dev_hotplug_obj *dev;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.hotplug;
    while (dev) {
        if (dev->hdl == hdl) {
            break;
        }
        dev = dev->next;
    }

    if (dev) {
        dev->dev_id   = dev_id;
        dev->dev_type = dev_type;
        dev->info     = info;
    } else {
        dev = os_zalloc(sizeof(struct dev_hotplug_obj));
        if (dev) {
            dev->hdl      = hdl;
            dev->dev_id   = dev_id;
            dev->dev_type = dev_type;
            dev->info     = info;
            dev->next = s_dev_mgr.hotplug;
            s_dev_mgr.hotplug = dev;
        }
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return dev ? RET_OK : -ENOMEM;
}

int32 dev_hotplug_out(void *hdl)
{
    struct dev_hotplug_obj *dev  = NULL;
    struct dev_hotplug_obj *prev = NULL;

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.hotplug;
    while (dev) {
        if (dev->hdl == hdl) {
            if (prev == NULL) {
                s_dev_mgr.hotplug = dev->next;
            } else {
                prev->next = dev->next;
            }
            os_free(dev);
            break;
        } else {
            prev = dev;
            dev  = dev->next;
        }
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

int32 dev_hotplug_walk(uint16 type, dev_hotplug_walkcb cb)
{
    struct dev_hotplug_obj *dev;

    if (cb == NULL) {
        return -EINVAL;
    }

    os_mutex_lock(&s_dev_mgr.mutex, osWaitForever);
    dev = s_dev_mgr.hotplug;
    while (dev) {
        if (type == 0 || dev->dev_type == type) {
            if (cb((const struct dev_hotplug_obj *)dev)) {
                break;
            }
        }
        dev = dev->next;
    }
    os_mutex_unlock(&s_dev_mgr.mutex);
    return RET_OK;
}

