#ifndef _LLM_ERRNO_H_
#define _LLM_ERRNO_H_

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef RET_OK
#define RET_OK              0
#endif

#ifndef RET_ERR
#define RET_ERR             -1
#endif

#ifndef LLME_AGAIN
#define LLME_AGAIN          -2
#endif

#ifndef LLME_NOMEM
#define LLME_NOMEM          -3
#endif

#ifndef LLME_INTR
#define LLME_INTR           -4
#endif

#ifndef LLME_WAIT_TIMEOUT
#define LLME_WAIT_TIMEOUT   -5
#endif

#ifdef __cplusplus
}
#endif

#endif

