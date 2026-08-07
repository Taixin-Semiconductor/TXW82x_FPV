#include "basic_include.h"
#include "lib/multimedia/msi.h"

#define MSI_MATCH(msi, name) (os_strcmp((msi)->name, name)==0)

//媒体流组件列表
struct msi_core {
    struct msi *list;
    struct os_mutex lock;
} g_MSI;

static void msi_free(struct msi *msi);

/////////////////////////////////////////////////////////////////////////////
//局部函数

void msi_get(struct msi *msi)
{
    if (msi) {
        atomic_inc(&msi->users);
    }
}

void msi_put(struct msi *msi)
{
    uint32 bound;
    uint32 inited;
    if (msi) {
        ASSERT(atomic_read(&msi->users) > 0);
        bound  = atomic_read(&msi->bound);
        inited = atomic_read(&msi->inited);
        if (atomic_dec_and_test(&msi->users) && inited == 0) {
            msi_do_cmd(msi, MSI_CMD_POST_DESTROY, 0, 0);
            msi->priv   = NULL;
            msi->action = NULL;
            fbq_destory(&msi->fbQ);
            if (bound == 0) {
                msi_free(msi);
            }else{
                msi->deleted = 0;
            }
        }
    }
}

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
            atomic_inc(&o_if->bound);
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
    uint32 users;
    struct msi *out;

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        flag = disable_irq();
        out  = msi->output_list[i];
        if (out && (out == o_if || o_if == NULL)) {
            msi->output_list[i] = NULL;
            enable_irq(flag);

            users = atomic_read(&out->users);
            if (atomic_dec_and_test(&out->bound) && users == 0) {
                msi_free(out); //引用计数 和 绑定计数都等于0，释放msi
            }

            if (o_if) {
                return RET_OK;
            }
        }
        enable_irq(flag);
    }
    return RET_OK;
}

//该函数需要在lock保护下执行
static struct msi *msi_find_lock(const char *name)
{
    uint32 flag = disable_irq();
    struct msi *msi = g_MSI.list;
    while (msi) {
        if (MSI_MATCH(msi, name)) {
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
    g_MSI.list = msi;
    enable_irq(flag);
}

//该函数需要在lock保护下执行
static int32 msi_list_del(struct msi *msi)
{
    struct msi *ptr, *prev;
    uint32 flag = disable_irq();
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
    enable_irq(flag);
    return RET_OK;
}

//该函数需要在lock保护下执行
static void msi_notify_up(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    struct msi *ptr = g_MSI.list;
    while (ptr) {
        if (ptr != msi && msi_bound(ptr, msi)) {
            msi_do_cmd(ptr, cmd, param1, param2);
            msi_notify_up(ptr, cmd, param1, param2);
        }
        ptr = ptr->next;
    }
}

//该函数需要在lock保护下执行
static struct msi *msi_new_lock(const char *name)
{
    struct msi *msi = msi_find_lock(name);
    if (msi == NULL) {
        msi = os_zalloc(sizeof(struct msi));
        if (msi) {
            msi->name = name;
            msi_list_add(msi);
        }
    }
    return msi;
}

static void msi_free(struct msi *msi)
{
    if (msi) {
        msi_list_del(msi);
        msi_unbind(msi, NULL); //取消所有关联的组件
        os_free(msi);
    }
}

/////////////////////////////////////////////////////////////////////////////
//全局函数
struct msi *msi_new(const char *name, uint32 qsize, uint8 *isnew)
{
    struct msi *msi;

    os_mutex_lock(&g_MSI.lock, osWaitForever);
    msi = msi_new_lock(name);
    if(msi && msi->deleted){
        os_printf(KERN_ERR"msi %s destory running ....\r\n", name);
        msi = NULL;
    }

    if (msi) {
        if(isnew) {
            *isnew = msi->inited.counter == 0;
        }
        atomic_inc(&msi->inited);
        atomic_inc(&msi->users);
        if (!msi->fbQ.init && qsize) {
            fbq_init(&msi->fbQ, NULL, qsize+1);
        }
    }
    os_mutex_unlock(&g_MSI.lock);
    return msi;
}

void msi_destroy(struct msi *msi)
{
    if (msi) {
        os_printf(KERN_ERR"msi %s destory, lr:%x\r\n", msi->name, RETURN_ADDR());
        os_mutex_lock(&g_MSI.lock, osWaitForever);
        if(atomic_dec_and_test(&msi->inited)){
            msi->enable = 0;
            msi->deleted = 1;
            msi_do_cmd(msi, MSI_CMD_PRE_DESTROY, 0, 0);
            msi_put(msi);
        }else{
            atomic_dec(&msi->users);
        }
        os_mutex_unlock(&g_MSI.lock);
    }
}

//根据ID查找是否存在该组件，引用计数为加1，使用后需要执行 msi_put
struct msi *msi_find(const char *name, uint8 inited)
{
    struct msi *msi;

    if(name == NULL){
        return NULL;
    }

    os_mutex_lock(&g_MSI.lock, osWaitForever);
    msi = msi_find_lock(name);
    if(msi && inited && atomic_read(&msi->inited) == 0){
        msi = NULL;
    }else{
        msi_get(msi);
    }
    os_mutex_unlock(&g_MSI.lock);
    return msi;
}

//设置该组件的输出组件
int32 msi_add_output(struct msi *msi, const char *lname, const char *oname)
{
    int32 ret = RET_ERR;
    struct msi *l_msi, *o_msi;

    os_mutex_lock(&g_MSI.lock, osWaitForever);
    l_msi = msi ? msi : msi_new_lock(lname);
    o_msi = msi_new_lock(oname);
    if (l_msi && o_msi) {
        ret = msi_bind(l_msi, o_msi);
    }
    os_mutex_unlock(&g_MSI.lock);
    return ret;
}

//删除该组件的输出组件
int32 msi_del_output(struct msi *msi, const char *lname, const char *oname)
{
    int32 ret = RET_ERR;
    struct msi *l_msi, *o_msi;

    os_mutex_lock(&g_MSI.lock, osWaitForever);
    l_msi = msi ? msi : msi_find(lname, 0);
    o_msi = msi_find(oname, 0);
    if (l_msi && o_msi) {
        ret = msi_unbind(l_msi, o_msi);
    }

    if(msi == NULL){
        msi_put(l_msi);
    }
    msi_put(o_msi);
    os_mutex_unlock(&g_MSI.lock);
    return ret;
}

//组件输出cmd: 遍历自己的输出组件列表，调用输出组件的 action 接口
int32 msi_output_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    int8 i;

    if(!msi){
        return RET_OK;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        msi_do_cmd(msi->output_list[i], cmd, param1, param2);
    }

    return RET_OK;
}

//组件输出framebuff: 输出framebuff给自己的组件列表
//注意：fb参数只能是 framebuff链表的第1个节点，不能是中间节点
//如果fb为空,就是检查是否有数据流接收(返回0x80000000)
int32 msi_output_fb(struct msi *msi, struct framebuff *fb)
{
    int ret = 0;
    int8 i;
    uint32 flag;
    struct msi *out;
    
    if(!msi || !msi->enable){
        fb_put(fb);
        return 0;
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        flag = disable_irq();
        out  = msi->output_list[i];
        if(out && out->enable){
            msi_get(out);
        }else{
            out = NULL;
        }
        enable_irq(flag);

        if(out && out->enable){
            if(!fb){
                ret = -1;
                msi_put(out);
                break;
            } else {
                if(msi_do_cmd(out, MSI_CMD_TRANS_FB, (uint32)fb, 0) == RET_OK){
                    if(out->fbQ.init && fbq_enqueue(&out->fbQ, fb)){
                        msi_do_cmd(out, MSI_CMD_TRANS_FB_END, (uint32)fb, 0);
                        ret++;
                    }
                }
            }
        }

        msi_put(out);
    }

    fb_put(fb);
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
    if(msi && msi->fbQ.init){
        return fbq_dequeue(&msi->fbQ,  tmo_ms);
    }
    return NULL;
}

struct framebuff *msi_get_fb_r(struct msi *msi, uint32 reader)
{
    if(msi && msi->fbQ.init){
        return fbq_dequeue_r(&msi->fbQ,  reader);
    }
    return NULL;
}

//追踪某个framebuff当前在被哪些模块处理
static int32 msi_fb_trace_discard(struct msi *msi, struct framebuff *fb, uint8 discard)
{
    int8 i;
    uint32 flag;
    struct msi *out;
    
    if(msi == NULL){
        return RET_OK;
    }

    if(fbq_trace(&msi->fbQ, fb, discard)){
        os_printf(KERN_ERR"MSI:%s framebuff %p is here!\r\n", msi->name, fb);
    }

    for (i = 0; i < MSI_OUTIF_MAX; i++) {
        flag = disable_irq();
        out  = msi->output_list[i];
        msi_get(out);
        enable_irq(flag);

        if(out && out->fbQ.init && fbq_trace(&msi->fbQ, fb, discard)){
            os_printf(KERN_ERR"MSI:%s framebuff %p is here!\r\n", msi->name, fb);
        }

        msi_put(out);
    }

    return RET_OK;
}

//追踪framebuff：查看指定的framebuff当前在被哪些模块处理
int32 msi_trace_fb(struct msi *msi, struct framebuff *fb)
{
    return msi_fb_trace_discard(msi, fb, 0);
}

//通知通路中的各个模块，丢弃指定的framebuff
int32 msi_discard_fb(struct msi *msi, struct framebuff *fb)
{
    return msi_fb_trace_discard(msi, fb, 1);
}

//组件产生notify操作，可以向上/向下传递
void msi_notify(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    os_mutex_lock(&g_MSI.lock, osWaitForever);
    msi_notify_up(msi, cmd, param1, param2);    //向上
    os_mutex_unlock(&g_MSI.lock);
    msi_output_cmd(msi, cmd, param1, param2);   //向下
}

//组件执行自己的action
int32 msi_do_cmd(struct msi *msi, uint32 cmd, uint32 param1, uint32 param2)
{
    uint32 flag;
	int32 ret = RET_OK;
    msi_action action;

    if(msi){
        flag = disable_irq();
        action = msi->action;
        enable_irq(flag);
        if(action){
            ret = action(msi, cmd, param1, param2);
        }
    }
	return ret;
}

//指定从某个组件开始执行cmd
void msi_cmd(const char *name, uint32 cmd, uint32 param1, uint32 param2)
{
    struct msi *msi = msi_find(name, 0);
    if (msi) {
        msi_do_cmd(msi, cmd, param1, param2);
        msi_output_cmd(msi, cmd, param1, param2);
        msi_put(msi);
    }
}

int32 msi_core_init()
{
    os_mutex_init(&g_MSI.lock);
    return RET_OK;
}

