#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/rpc/cpurpc.h"

int32 audio_coder_run_rpc(AUCODE_MANAGE_RPC *aucode_manage)
{
    uint32 args[] = {(uint32)aucode_manage};
    return CPU_RPC_CALL(audio_coder_run);
}

int32 audio_code_frame_rpc(AUCODE_MANAGE_RPC *aucode_manage)
{
    uint32 args[] = {(uint32)aucode_manage};
    return CPU_RPC_CALL(audio_code_frame);    
}

int32 audio_code_wait_finish_rpc(AUCODE_MANAGE_RPC *aucode_manage)
{
    return os_event_wait(&aucode_manage->finish_event, codec_finish, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
}

int32 audio_code_finish_rpc(AUCODE_MANAGE_RPC *aucode_manage)
{
    return os_event_set(&aucode_manage->finish_event, codec_finish, NULL);
}