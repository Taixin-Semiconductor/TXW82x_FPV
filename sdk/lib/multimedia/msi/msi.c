#include "basic_include.h"
#include "lib/multimedia/msi.h"

#define MSI_MATCH(msi, name) (os_strcmp((msi)->name, name)==0)

//媒体流组件列表
struct msi_core {
    struct msi *list;
    //struct os_mutex lock;
} g_MSI;

static int32 msi_bound(struct msi *msi, struct msi *o_if)
{
    int i;

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        if (msi->output_list[i] == o_if) {
            return 1;
        }
    }
    return 0;
}

static int32 msi_bind(struct msi *msi, struct msi *o_if)
{
    int i;

    if (msi_bound(msi, o_if)) {
        return RET_OK;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        if (msi->output_list[i] == NULL) {
            msi->output_list[i] = o_if;
            return RET_OK;
        }
    }

    os_printf(KERN_ERR"msi %s bind %s fail!\r\n", msi->name, o_if->name);
    return RET_ERR;
}

static int32 msi_unbind(struct msi *msi, struct msi *o_if)
{
    int i;
    uint32 flag;
    struct msi *out;

    flag = disable_irq();
    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        out = msi->output_list[i];
        if (out && (out == o_if || o_if == NULL)) {
            msi->output_list[i] = NULL;
            enable_irq(flag);
            if (o_if) {
                return RET_OK;
            }
            flag = disable_irq();
        }
    }
    enable_irq(flag);
    return RET_OK;
}

//该函数需要在lock保护下执行
static int32 msi_unbind_me(struct msi *msi)
{
    uint32 flag;
    struct msi *next;
    
    flag = disable_irq();
    struct msi *ptr = g_MSI.list;

    while (ptr) {
        next = ptr->next;
        if (ptr != msi) {
            msi_unbind(ptr, msi);
        }
        ptr = next;
    }
    enable_irq(flag);
    return RET_OK;
}

//该函数需要在lock保护下执行
//按type查找时，不能查找多通道组件的实例化组件
static struct msi *msi_find_lock(const char *name, uint16 type)
{
    uint32 flag = disable_irq();
    struct msi *msi = g_MSI.list;
    while (msi) {
        if ((name && MSI_MATCH(msi, name)) ||
            (type && msi->type == type && msi->mgr == NULL)) {
            break;
        }
        msi = msi->next;
    }
    enable_irq(flag);
    return msi;
}

//该函数需要在lock保护下执行
static void msi_list_add(struct msi *msi)
{
    uint32 flag = disable_irq();
    msi->next = g_MSI.list;
    msi->list_head = 1;
    g_MSI.list = msi;
    enable_irq(flag);
}

//该函数需要在lock保护下执行
static int32 msi_list_del(struct msi *msi)
{
    struct msi *ptr, *prev;
    prev = NULL;
    ptr  = g_MSI.list;
    while (ptr) {
        if (ptr == msi) {
            if (prev) {
                prev->next = ptr->next;
            } else {
                g_MSI.list = ptr->next;
            }
            break;
        }
        prev = ptr;
        ptr  = ptr->next;
    }
    msi->enable = 0;
    msi->list_head = 0;
    return RET_OK;
}

//该函数需要在lock保护下执行
static void msi_notify_up(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    uint32 flag = disable_irq();
    struct msi *ptr  = g_MSI.list;
    struct msi *next = (ptr ? ptr->next : NULL);
    if(ptr)  msi_get(ptr);
    if(next) msi_get(next);
    enable_irq(flag);
    
    while (ptr) {
        if (ptr != msi && msi_bound(ptr, msi)) {
            msi_do_cmd(ptr, cmd, param1, param2);
            msi_notify_up(ptr, cmd, param1, param2);
        }

        msi_put(ptr);

        uint32 flag = disable_irq();
        ptr  = next;
        next = (ptr ? ptr->next : NULL);
        if(next) msi_get(next);
        enable_irq(flag);
    }
}

//该函数需要在lock保护下执行
static struct msi *msi_new_lock(const char *name)
{
    struct msi *msi = msi_find_lock(name, 0);
    if (msi == NULL) {
        msi = os_malloc(sizeof(struct msi));
        if (msi) {
            os_memset(msi, 0, sizeof(struct msi));
            msi->type     = 0xffff;
            msi->name     = name;
            msi->fb_alloc = (malloc_cb_t)fb_mem_alloc; //默认的fb分配函数
            msi->fb_free  = (mfree_cb_t)fb_mem_free;
            msi_list_add(msi);
        }
    }
    return msi;
}

int32 msi_reuse_output(struct msi *n_msi, struct msi *msi)
{
    int8 i;

    if (n_msi == NULL || msi == NULL) {
        return RET_OK;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        if (msi->output_list[i]) {
            msi_add_output(n_msi, NULL, msi->output_list[i], NULL);
        }
    }
    return RET_OK;
}

static struct msi *msi_new_chan(struct msi *msi, void *arg)
{
    struct msi *inst = NULL;

    msi_do_cmd(msi, MSI_CMD_NEW_CHANNEL, (uint32)arg, (uint32)&inst);
    if (inst) {
        //动态对象：修改inited为0，执行一次get，让外部代码可以使用msi_put释放
        msi_get(inst);
        inst->inited.counter = 0;
        inst->mgr = msi_get(msi);
        msi_reuse_output(inst, msi);
        os_printf(KERN_NOTICE"%s: new channel %s\r\n", msi->name, inst->name);
        return inst;
    }

    return NULL;
}

void msi_free(struct msi *msi)
{
    msi_put(msi->mgr);
    fbq_destroy(&msi->fbQ);
    msi_do_cmd(msi, MSI_CMD_POST_DESTROY, 0, 0);
    msi_unbind_me(msi);
    msi_unbind(msi, NULL); //取消自己关联的所有组件
    os_printf(KERN_NOTICE"msi [%s] destory!\r\n", msi->name);
    os_free(msi);
}

/////////////////////////////////////////////////////////////////////////////
//全局函数
struct msi *msi_get(struct msi *msi)
{
    if (msi) {
        atomic_inc(&msi->users);
    }
    return msi;
}

void msi_put(struct msi *msi)
{
    uint32 inited;
    uint32 users;
    uint32 flag;

    if (msi) {
        flag = disable_irq();
        inited = msi->inited.counter;
        users  = --msi->users.counter;
        if(inited == 0 && users == 0 && msi->list_head){
            msi_list_del(msi);
        }
        enable_irq(flag);

        if (users == 0 && inited == 0) {
            int32 ret = RET_OK;
            if(msi->mgr){
                ret = msi_do_cmd(msi, MSI_CMD_PRE_DESTROY, 0, 0);
            }
            if(ret == RET_OK){
                msi_free(msi);
            }
        }
    }
}

struct msi *msi_new(const char *name, uint32 qsize, uint8 *isnew)
{
    struct msi *msi;

    msi = msi_new_lock(name);
    if (msi) {
        if (isnew) {
            *isnew = (msi->inited.counter == 0);
        }
        atomic_inc(&msi->inited);
        if (!msi->fbQ.init && qsize) {
            fbq_init(&msi->fbQ, NULL, qsize, msi);
        }
    } else {
        if (isnew) {
            *isnew = 0;
        }
    }
    return msi;
}

void msi_destroy(struct msi *msi)
{
    if (msi) {
        ASSERT(msi->inited.counter > 0);
        os_printf(KERN_NOTICE"msi %s destory! lr:%x\r\n", msi->name, RETURN_ADDR());
    
        uint32 flag = disable_irq();
        uint32 inited = --msi->inited.counter;
        uint32 users  = msi->users.counter;
        if(inited == 0) msi_list_del(msi);
        enable_irq(flag);

        if(inited == 0){
            int32 ret = msi_do_cmd(msi, MSI_CMD_PRE_DESTROY, 0, 0);
            if (users == 0 && ret == RET_OK) {
                msi_free(msi);
            }
        }
    }
}

//清除msi队列中的framebuff
void msi_clear(struct msi *msi)
{
    if (msi) {
        fbq_clear(&msi->fbQ);
    }
}

struct msi *msi_find(const char *name, uint8 inited)
{
    struct msi *msi;

    if (name == NULL) {
        return NULL;
    }

    msi = msi_find_lock(name, 0);
    if (msi && inited && atomic_read(&msi->inited) == 0) {
        msi = NULL;
    } else {
        msi_get(msi);
    }
    return msi;
}

struct msi *msi_find2(const char *name, uint16 type, uint8 inited, void *arg)
{
    struct msi *msi;
    struct msi *inst;

    if (name == NULL && type == 0) {
        return NULL;
    }

    msi = msi_find_lock(name, type);
    if (msi && inited && atomic_read(&msi->inited) == 0) {
        msi = NULL;
    } else {
        msi_get(msi);
    }

    inst = msi;
    if (msi && msi->chan_mgr) {
        inst = msi_new_chan(msi, arg);
        msi_put(msi);
        os_printf("find new msi %s\r\n", inst->name);
    }
    return inst;
}

//设置该组件的输出组件
int32 msi_add_output(struct msi *l_msi, const char *lname, struct msi *o_msi, const char *oname)
{
    int32 ret = RET_ERR;
    int8  p1 = 0;
    int8  p2 = 0;

    if (l_msi == NULL && lname) {
        l_msi = msi_find(lname, 0);
        if (l_msi) {
            p1 = 1;
        } else {
            l_msi = msi_new_lock(lname);
        }
    }

    if (o_msi == NULL && oname) {
        o_msi = msi_find(oname, 0);
        if (o_msi) {
            p2 = 1;
        } else {
            o_msi = msi_new_lock(oname);
        }
    }

    if (l_msi && o_msi) {
        ret = msi_bind(l_msi, o_msi);
    }

    if (p1) { msi_put(l_msi); }
    if (p2) { msi_put(o_msi); }
    return ret;
}

//删除该组件的输出组件
int32 msi_del_output(struct msi *l_msi, const char *lname, struct msi *o_msi, const char *oname)
{
    int32 ret = RET_ERR;
    int8  p1 = 0;
    int8  p2 = 0;

    if (l_msi == NULL && lname) {
        l_msi = msi_find(lname, 0);
        p1 = 1;
    }

    if (o_msi == NULL && oname) {
        o_msi = msi_find(oname, 0);
        p2 = 1;
    }

    if (l_msi) {
        ret = msi_unbind(l_msi, o_msi);
    } else if (o_msi) {
        ret = msi_unbind_me(o_msi);
    }

    if (p1) { msi_put(l_msi); }
    if (p2) { msi_put(o_msi); }
    return ret;
}

//组件输出cmd: 遍历自己的输出组件列表，调用输出组件的 action 接口
int32 msi_output_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    int8 i;

    if (!msi) {
        return RET_OK;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        msi_do_cmd(msi->output_list[i], cmd, param1, param2);
        msi_output_cmd(msi->output_list[i], cmd, param1, param2); //向下传递
    }

    return RET_OK;
}

int32 msi_recv_fb(struct msi *msi, struct framebuff *fb)
{
    if (!msi || !msi->enable || !fb) {
        return RET_ERR;
    }

    uint16 type = (fb->mtype << 8 | fb->stype);
    if (msi->type == 0xffff || type == msi->type || ((msi->type & 0xff) == 0xff && (msi->type >> 8) == fb->mtype)) {
        if (msi_do_cmd(msi, MSI_CMD_TRANS_FB, (uint32)fb, 0) == RET_OK) {
            if (msi->fbQ.init && fbq_enqueue(&msi->fbQ, fb, 1)) {
                msi_do_cmd(msi, MSI_CMD_TRANS_FB_END, (uint32)fb, 0);
                return RET_OK;
            }
        }
    }
    return RET_ERR;
}

int32 msi_output_fb(struct msi *msi, struct framebuff *fb, uint8 care)
{
    int ret = 0;
    int8 i;
    uint32 flag;
    struct msi *out;

    if (!msi) {
        if (!care) {
            fb_put(fb);
        }
        return 0;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        flag = disable_irq();
        out  = msi->output_list[i];
        if (out && out->enable) {
            msi_get(out);
        } else {
            out = NULL;
        }
        enable_irq(flag);

        if (out && out->enable) {
            if (!fb) {
                ret = -1;
                msi_put(out);
                break;
            } else if (msi_recv_fb(out, fb) == RET_OK) {
                ret++;
            }
        }

        msi_put(out);
    }

    if (ret > 0 || !care) {
        fb_put(fb);
    }
    return ret;
}

//组件不输出framebuff，需要删除framebuff
//注意：fb参数只能是 framebuff链表的第1个节点，不能是中间节点
int32 msi_delete_fb(struct msi *msi, struct framebuff *fb)
{
    fb_put(fb);
    return RET_OK;
}

struct framebuff *msi_get_fb(struct msi *msi, uint32 tmo_ms)
{
    if (msi && msi->fbQ.init) {
        return fbq_dequeue(&msi->fbQ,  tmo_ms);
    }
    return NULL;
}

struct framebuff *msi_get_fb_r(struct msi *msi, uint32 reader)
{
    if (msi && msi->fbQ.init) {
        return fbq_dequeue_r(&msi->fbQ,  reader);
    }
    return NULL;
}

//追踪某个framebuff当前在被哪些模块处理
static int32 msi_fb_trace_discard(struct msi *msi, struct framebuff *fb, struct msi *owner, uint8 discard)
{
    int8 i;
    uint32 flag;
    struct msi *out;

    if (msi == NULL) {
        return RET_OK;
    }

    fbq_trace(&msi->fbQ, fb, owner, discard, 0);
    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        flag = disable_irq();
        out  = msi->output_list[i];
        msi_get(out);
        enable_irq(flag);

        msi_fb_trace_discard(out, fb, owner, discard);
        msi_put(out);
    }

    return RET_OK;
}

//追踪framebuff：查看指定的framebuff当前在被哪些模块处理
int32 msi_trace_fb(struct msi *msi, struct framebuff *fb, struct msi *owner)
{
    return msi_fb_trace_discard(msi, fb, owner, 0);
}

//通知通路中的各个模块，丢弃指定的framebuff
int32 msi_discard_fb(struct msi *msi, struct framebuff *fb, struct msi *owner)
{
    return msi_fb_trace_discard(msi, fb, owner, 1);
}

//组件产生notify操作，可以向上/向下传递
void msi_notify(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    msi_notify_up(msi, cmd, param1, param2);    //向上
    msi_cmd2(msi, cmd, param1, param2);   //向下
}

//组件执行自己的action
int32 msi_do_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    uint32 flag;
    int32 ret = RET_OK;
    msi_action action;

    if (msi) {
        flag = disable_irq();
        action = msi->action;
        enable_irq(flag);
        if (action) {
            ret = action(msi, cmd, param1, param2);
        }
    }
    return ret;
}

//指定从某个组件开始执行cmd
void msi_cmd2(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 i = 0;
    if (msi) {
        msi_do_cmd(msi, cmd, param1, param2);
        for (i = 0; i < MSI_OUTIF_MAX; i++) {
            msi_cmd2(msi->output_list[i], cmd, param1, param2);
        }
    }
}

//指定从某个组件开始执行cmd
void msi_cmd(const char *name, uint32 cmd, uint32 param1, uint32 param2)
{
    struct msi *msi = msi_find(name, 0);
    if (msi) {
        msi_cmd2(msi, cmd, param1, param2);
        msi_put(msi);
    }
}

int32 msi_core_init()
{
    //os_mutex_init(&g_MSI.lock);
    return RET_OK;
}

struct framebuff *msi_alloc_fb(struct msi *msi, void *alloc_priv, uint8 *data, uint32 data_size, uint32 codecinfo_size, uint32 privinfo_size)
{
    uint32 len = data_size;
    struct framebuff *fb = NULL;
    uint16 info_off = sizeof(struct framebuff);
    uint16 hdr_size = sizeof(struct framebuff) + codecinfo_size + privinfo_size;

    //msi->fb_limits=0: 表示MSI不能再分配新的fb，等待旧的fb释放后才能继续分配
    if (atomic_dec2_return(&msi->fb_limits) > 0) {
        if (data == NULL) {
            hdr_size = ALIGN(hdr_size, 4);
        }

        len += hdr_size;
        fb = (struct framebuff *)msi->fb_alloc(len, alloc_priv);
        if (fb) {
            os_memset(fb, 0, hdr_size);
            atomic_set(&fb->users, 1);

            if (codecinfo_size) {
                fb->codec_info = (uint8 *)fb + info_off;
                info_off += codecinfo_size;
            }
            if (privinfo_size) {
                fb->priv = (uint8 *)fb + info_off;;
                info_off += privinfo_size;
            }

            fb->mtype = 0xff;
            fb->stype = 0xff;
            fb->msi   = msi_get(msi);
            fb->free  = (mfree_cb_t)msi->fb_free;
            fb->free_priv = alloc_priv;
            fb->len   = data_size;
            if(!data_size) fb->data = NULL;
            else fb->data = (data ? data : (uint8 *)fb + hdr_size);
        } else {
            atomic_inc(&msi->fb_limits);
        }
    } else {
        msi->alloc_fail++;
        if (msi->alloc_fail >= 32) {
            os_printf(KERN_WARNING"[%s] need more fb:%d, please check msi->fb_limits!!!\r\n", msi->name, msi->alloc_fail);
            msi->alloc_fail = 0;
        }
    }

    return fb;
}

void msi_dump(void)
{
    struct msi *msi = g_MSI.list;
    while (msi) {
        int i = 0;
        os_printf(KERN_NOTICE"-----------------------------------------------------------------------------------------------\r\n");
        os_printf(KERN_NOTICE"MSI: [%s], type:%d/%d, inited:%d, users:%d, fb_limits:%d, fbQ:%d, %s, %s, priv:%p\r\n",
                  msi->name, (msi->type >> 8) & 0xff, (msi->type & 0xff),
                  msi->inited.counter, msi->users.counter, msi->fb_limits.counter,
                  msi->fbQ.init ? fbq_count(&msi->fbQ) : 0,
                  msi->enable ? "Enable" : "Disable",
                  msi->mgr ? "Dynamic" : (msi->chan_mgr ? "Manager" : "Static"),
                  msi->priv);
        os_printf(KERN_NOTICE"Output:");
        for (i = 0; i < MSI_OUTIF_MAX; i++) {
            if (msi->output_list[i]) {
                _os_printf(KERN_NOTICE" [%s]", msi->output_list[i]->name);
            }
        }
        _os_printf(KERN_NOTICE"\r\n");
        fbq_trace(&msi->fbQ, NULL, NULL, 0, 1);
        msi_do_cmd(msi, MSI_CMD_DUMP, 0, 0);
        msi = msi->next;
    }
}


