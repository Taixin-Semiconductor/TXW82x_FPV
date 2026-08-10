#include "llm.h"

static struct llm_manager llm_mgr = {0};

static void llm_task(void *args)
{
    int i = 0;
    struct llm_taskinfo *tasks = (struct llm_taskinfo *)args;

    while (1) {
        // 当前线程退出
        if (tasks->exit == 1) {
            os_memset(tasks, 0, sizeof(struct llm_taskinfo));
            // 退出前检查一下是否最后一个线程
            for (i = 0; i < LLM_SESSION_MAX; i++) {
                if (llm_mgr.session[i]) {
                    break;
                }
            }
            if (i == LLM_SESSION_MAX) {
                llm_trans_deinit();
            }
            break;
        }
        for (i = 0; i < LLM_SESSION_MAX; i++) {
            if (llm_mgr.session[i] && ((tasks->bitmap >> i) & 0x1)) {
                llm_mgr.session[i]->run(llm_mgr.session[i]);
            }
        }
//        os_sleep_ms(10);
    }
}

struct llm_model *llm_find_model(char *name)
{
    int32 i = 0;
    for (i = 0; i < llm_mgr.model_count; i++) {
        if (os_strcasecmp(llm_mgr.models[i].name, name) == 0) {
            return (void *)&llm_mgr.models[i];
        }
    }
    return NULL;
}

int32 llm_del_session(struct llm_session *session)
{
    int i = 0;

    for (i = 0; i < LLM_SESSION_MAX; i++) {
        if (os_strcasecmp(llm_mgr.session[i]->name, session->name) == 0) {
            llm_mgr.session[i] = NULL;
            llm_mgr.tasks[i].bitmap &= ~(1 << i);
            return RET_OK;
        }
    }

    return RET_ERR;
}

int32 llm_add_session(struct llm_session *session)
{
    int i = 0;

    if (!session) {
        llm_err("session is not exist!\r\n");
        return RET_ERR;
    }

    for (i = 0; i < LLM_SESSION_MAX; i++) {
        if (llm_mgr.session[i] == NULL) {
            llm_mgr.session[i] = session;
            if (llm_mgr.task_reuse) {
                if (session->type == LLM_TTS) {
                    llm_mgr.tasks[1].hdl = os_task_create("llm_task1", llm_task, (void *)&llm_mgr.tasks[1], OS_TASK_PRIORITY_NORMAL, 0, NULL, 8192);
                    llm_mgr.tasks[1].bitmap |= (0x1 << i);
                } else {
                    llm_mgr.tasks[0].bitmap |= (0x1 << i);
                }
            } else {
                llm_mgr.tasks[i].bitmap |= (0x1 << i);
                llm_mgr.tasks[i].hdl = os_task_create(session->name, llm_task, (void *)&llm_mgr.tasks[i], OS_TASK_PRIORITY_NORMAL, 0, NULL, 8192);
            }
            return RET_OK;
        }
    }

    return RET_ERR;
}

static void llm_core_init(uint8 task_reuse)
{
    llm_mgr.task_reuse = task_reuse;
    if (llm_mgr.task_reuse) {
        llm_mgr.tasks[0].hdl = os_task_create("llm_task0", llm_task, (void *)&llm_mgr.tasks[0], OS_TASK_PRIORITY_NORMAL, 0, NULL, 8192);
    }
}

static void llm_core_deinit(void)
{
    int i = 0;
        for (i = 0; i < LLM_SESSION_MAX; i++) {
		if (llm_mgr.tasks[i].hdl) {
			llm_mgr.tasks[i].exit = 1;
			//os_task_destroy(llm_mgr.tasks[i].hdl);
        }
    }
}

//GLOBAL
int32 llm_global_init(struct llm_global_param *global)
{
    if (llm_equipment_legality_check()) {
        return RET_ERR;
    }

    ASSERT(global);
    if (global) {
        llm_mgr.model_count = global->model_count;
        llm_mgr.models = global->models;
        llm_core_init(global->task_reuse ? 1 : 0);
        llm_trans_init();
    }
    return RET_OK;
}

int32 llm_global_deinit(void)
{
    llm_core_deinit();
    return RET_OK;
}

