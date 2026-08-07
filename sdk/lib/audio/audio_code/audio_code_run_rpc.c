#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/rpc/cpurpc.h"

int32 audio_coder_run_rpc(AUCODE_MANAGE *aucode_manage)
{
    uint32 args[] = {(uint32)aucode_manage};
    return CPU_RPC_CALL(audio_coder_run);
}
