#include "basic_include.h"
#include "lowPower_module_registry.h"

enum lowPower_registry_state {
    LOWPOWER_REGISTRY_ACTIVE,
    LOWPOWER_REGISTRY_SUSPENDING,
    LOWPOWER_REGISTRY_SUSPENDED,
    LOWPOWER_REGISTRY_RESUMING
};

struct lowPower_module_node {
    struct lowPower_module_ops ops;
    unsigned int suspended;
    struct lowPower_module_node *prev;
    struct lowPower_module_node *next;
};

static struct lowPower_module_node *module_head = NULL;
static struct lowPower_module_node *module_tail = NULL;
static unsigned int module_count = 0;
static enum lowPower_registry_state registry_state = LOWPOWER_REGISTRY_ACTIVE;

int lowPower_app_register_module(const struct lowPower_module_ops *ops)
{
    struct lowPower_module_node *node;

    if ((registry_state != LOWPOWER_REGISTRY_ACTIVE)    || 
        (ops == NULL)                                   ||
        (ops->name == NULL)                             || 
        (ops->suspend == NULL && ops->resume == NULL))
    {
        os_printf("%s %d failed! module_count:%d\n", __FUNCTION__, __LINE__, module_count);
        return LOWPOWER_MODULE_ERR;
    }

    node = (struct lowPower_module_node *)os_zalloc(sizeof(struct lowPower_module_node));
    if (node == NULL) {
        os_printf("%s %d malloc failed! module_count:%d\n", __FUNCTION__, __LINE__, module_count);
        return LOWPOWER_MODULE_ERR;
    }

    node->ops = *ops;
    node->suspended = 0;
    node->prev = module_tail;
    node->next = NULL;

    if (module_tail != NULL) {
        module_tail->next = node;
    } else {
        module_head = node;
    }
    module_tail = node;
    ++module_count;
    return LOWPOWER_MODULE_OK;
}

int lowPower_app_unregister_all_modules(void)
{
    struct lowPower_module_node *node;
    struct lowPower_module_node *next;

    if (registry_state != LOWPOWER_REGISTRY_ACTIVE) {
        return LOWPOWER_MODULE_ERR;
    }

    node = module_head;
    while (node != NULL) {
        next = node->next;
        os_free(node);
        node = next;
    }

    module_head = NULL;
    module_tail = NULL;
    module_count = 0;
    return LOWPOWER_MODULE_OK;
}

int lowPower_app_has_registered_modules(void)
{
    return module_count != 0;
}

int lowPower_app_suspend_modules(void)
{
    struct lowPower_module_node *node;
    struct lowPower_module_node *rollback;

    if (registry_state != LOWPOWER_REGISTRY_ACTIVE) {
        return LOWPOWER_MODULE_ERR;
    }

    registry_state = LOWPOWER_REGISTRY_SUSPENDING;
    node = module_tail;
    while (node != NULL) {
        if (node->ops.suspend != NULL) {
            os_printf("%s %d ops.name:%s\n", __FUNCTION__, __LINE__, node->ops.name);
            int res = node->ops.suspend(node->ops.priv[0], node->ops.priv[1], node->ops.priv[2], node->ops.priv[3]);
            if (res) {
                rollback = node->next;
                while (rollback != NULL) {
                    if (rollback->suspended && rollback->ops.resume != NULL) {
                        rollback->ops.resume(rollback->ops.priv[0],
                                             rollback->ops.priv[1],
                                             rollback->ops.priv[2],
                                             rollback->ops.priv[3]);
                    }
                    rollback->suspended = 0;
                    rollback = rollback->next;
                }
                registry_state = LOWPOWER_REGISTRY_ACTIVE;
                return LOWPOWER_MODULE_ERR;
            }
        }
        node->suspended = 1;
        node = node->prev;
    }
    registry_state = LOWPOWER_REGISTRY_SUSPENDED;
    return LOWPOWER_MODULE_OK;
}

int lowPower_app_resume_modules(void)
{
    int ret = LOWPOWER_MODULE_OK;
    struct lowPower_module_node *node;

    if (registry_state != LOWPOWER_REGISTRY_SUSPENDED) {
        return LOWPOWER_MODULE_ERR;
    }

    registry_state = LOWPOWER_REGISTRY_RESUMING;
    node = module_head;
    while (node != NULL) {
        if (node->ops.resume != NULL) {
            if (node->suspended) {
                os_printf("%s %d ops.name:%s\n", __FUNCTION__, __LINE__, node->ops.name);
                int res = node->ops.resume(node->ops.priv[0], node->ops.priv[1], node->ops.priv[2], node->ops.priv[3]);
                if (res) {
                    ret = LOWPOWER_MODULE_ERR;
                } else {
                    node->suspended = 0;
                }
            }
        } else {
            node->suspended = 0;
        }
        node = node->next;
    }
    if (ret == LOWPOWER_MODULE_OK) {
        registry_state = LOWPOWER_REGISTRY_ACTIVE;
    } else {
        registry_state = LOWPOWER_REGISTRY_SUSPENDED;
    }
    return ret;
}