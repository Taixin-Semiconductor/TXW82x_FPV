#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/rpc/cpurpc.h"

void audio_codec_thread(void *d)
{
    int32 len = 0;
    int32 ret = RET_ERR;
    AUCODE_MANAGE_RPC *aucode_manage = (AUCODE_MANAGE_RPC*)d;
    AUCODER_HDL *aucoder_hdl;
    void *memfree = NULL;
    while(1) {
        while(1) {
            len = aucode_manage->rb_read(&aucode_manage->memfree_rbuf, &memfree, sizeof(void*));
            if(len < sizeof(void*)) {
                break;
            }
            aucode_manage->mem_free(memfree);
        }
        ret = os_sema_down(&aucode_manage->code_sema, 5);
        if(ret == RET_OK) {
			len = aucode_manage->rb_read(&aucode_manage->req_rbuf, &aucoder_hdl, sizeof(AUCODER_HDL*));
			if(len >= sizeof(AUCODER_HDL*)) {
                aucoder_hdl->codec_func_rpc(aucoder_hdl);
                if(aucoder_hdl->codec_rpc) {
                    audio_code_finish(aucoder_hdl->aucode_manage_rpc);
                }
			}
		}
        if(aucode_manage->state == codec_rpc_stop) {
            while(1) {
                len = aucode_manage->rb_read(&aucode_manage->memfree_rbuf, &memfree, sizeof(void*));
                if(len < sizeof(void*)) {
                    break;
                }
                aucode_manage->mem_free(memfree);
            }
            break;
        }
    }
    os_sema_del(&(aucode_manage->code_sema));
    aucode_manage->state = codec_rpc_exit;
}

int32 audio_coder_run(AUCODE_MANAGE_RPC *aucode_manage)
{
    int32 ret = RET_ERR;
    ret = os_sema_init(&aucode_manage->code_sema, 0);
    if(ret != RET_OK) {
        return AUCODE_ERR;
    }
    aucode_manage->task_hdl = os_task_create("audio_codec_thread", audio_codec_thread, (void*)aucode_manage, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 3072);
	if(aucode_manage->task_hdl == NULL) {
        os_sema_del(&(aucode_manage->code_sema));
        return AUCODE_ERR; 
    }
    return AUCODE_OK;
}

int32 audio_code_frame(AUCODE_MANAGE_RPC *aucode_manage)
{
    return os_sema_up(&aucode_manage->code_sema);
}

int32 audio_code_finish(AUCODE_MANAGE_RPC *aucode_manage)
{
    uint32 args[] = {(uint32)aucode_manage};
    return CPU_RPC_CALL(audio_code_finish_rpc);
}