#include "screen_command.h"
#include "screen_memory.h"
#include "screen_common.h"

static os_msgqueue_t screen_cmd_msgq;
static uint8_t       command_init = 0;

void screen_command_init(void)
{
    os_msgq_init(&screen_cmd_msgq, 1);
    command_init = 1;
}

void screen_command_deinit(void)
{
}

// 返回一个screen_id,相当于一个screen句柄,只有一样才可以控制
// id是一个枚举,用于启动哪个ui
uint32_t screen_command_send(uint16_t id, uint32_t screen_id, void *params)
{
    if (!command_init)
    {
        return 0;
    }
    int32_t          ret;
    screen_command_t node;
    node.screen_id = screen_id;
    node.id        = id;
    node.params    = params;
    os_event_init(&node.evt);

    ret = os_msgq_put(&screen_cmd_msgq, (uint32_t) &node, 500);
    // 超时就不管了
    if (ret == RET_OK)
    {
        // 等待返回信号量?返回创建ui的screen_id
        os_event_wait(&node.evt, BIT(0), NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    }
    else
    {
        node.screen_id = 0;
    }
    os_event_del(&node.evt);
    return node.screen_id;
}

static uint32_t screen_push_cmd(uint32_t screen_id, struct screen_common_s *cmd)
{
    if (!command_init)
    {
        return 0;
    }
    int32_t          ret;
    screen_command_t node;
    node.screen_id = screen_id;
    node.params    = cmd;
    os_event_init(&node.evt);

    ret = os_msgq_put(&screen_cmd_msgq, (uint32_t) &node, 500);
    // 超时就不管了
    if (ret == RET_OK)
    {
        // 等待返回信号量?返回创建ui的screen_id
        os_event_wait(&node.evt, BIT(0), NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    }
    else
    {
        cmd->screen_id = 0;
    }
    os_event_del(&node.evt);
    return cmd->screen_id;
}

void *screen_command_recv(void)
{
    screen_command_t *node;
    node = (screen_command_t *) os_msgq_get(&screen_cmd_msgq, 0);
    return node;
}

void screen_command_finish(screen_command_t *node)
{
    if (node)
    {
        os_event_set(&node->evt, BIT(0), NULL);
    }
}

int32_t screen_ioctl(uint32_t screen_id, uint32_t cmd, uint32_t param1, uint32_t param2)
{
    struct screen_common_s *common_cmd = (struct screen_common_s *) param1;
    if (!common_cmd)
    {
        return RET_ERR;
    }
    switch (cmd)
    {
        case SCREEN_IOCTL_CMD_CREATE_UI:
        {
            screen_push_cmd(screen_id, common_cmd);
            break;
        }

        default:
        {
            os_printf("screen_ioctl: unknown cmd %d\n", cmd);
            break;
        }
    }
    return 0;
}