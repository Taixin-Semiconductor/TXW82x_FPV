/*
 * Copyright 2024, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "heap.h"
#include "vfs.h"

#if !defined(PICO_VFS_BLOCKDEVICE_HEAP_BLOCK_SIZE)
#define PICO_VFS_BLOCKDEVICE_HEAP_BLOCK_SIZE 512
#endif

#if !defined(PICO_VFS_BLOCKDEVICE_HEAP_ERASE_VALUE)
#define PICO_VFS_BLOCKDEVICE_HEAP_ERASE_VALUE 0xFF
#endif

typedef struct
{
    size_t size;
    uint8_t *heap;
} blockdevice_heap_config_t;

static const char DEVICE_NAME[] = "heap";

uint8_t *g_heap;
uint32_t g_heap_size;

static int init(blockdevice_t *device)
{
    (void)device;
    blockdevice_heap_config_t *config = device->config;
    if (device->is_initialized)
    {
        return BD_ERROR_OK;
    }
    g_heap = config->heap;
    g_heap_size = config->size;

    device->is_initialized = true;
    return BD_ERROR_OK;
}

static int deinit(blockdevice_t *device)
{
    blockdevice_heap_config_t *config = device->config;

    if (!device->is_initialized)
    {
        return BD_ERROR_OK;
    }
    config->heap = NULL;
    device->is_initialized = false;
    return BD_ERROR_OK;
}

static int sync(blockdevice_t *device)
{
    (void)device;
    return BD_ERROR_OK;
}

static int read(blockdevice_t *device, const void *buffer, bd_size_t addr, bd_size_t length)
{
    blockdevice_heap_config_t *config = device->config;
    VFS_memcpy((uint8_t *)buffer, config->heap + (size_t)addr, (size_t)length);
    return BD_ERROR_OK;
}

static int erase(blockdevice_t *device, bd_size_t addr, bd_size_t length)
{
    blockdevice_heap_config_t *config = device->config;
    assert(config->heap != NULL);
    VFS_memset(config->heap + (size_t)addr, PICO_VFS_BLOCKDEVICE_HEAP_ERASE_VALUE, (size_t)length);

    return BD_ERROR_OK;
}

static int program(blockdevice_t *device, const void *buffer, bd_size_t addr, bd_size_t length)
{
    blockdevice_heap_config_t *config = device->config;
    VFS_memcpy(config->heap + (size_t)addr, buffer, (size_t)length);
    return BD_ERROR_OK;
}

static int trim(blockdevice_t *device, bd_size_t addr, bd_size_t length)
{
    (void)device;
    (void)addr;
    (void)length;
    return BD_ERROR_OK;
}

static bd_size_t size(blockdevice_t *device)
{
    blockdevice_heap_config_t *config = device->config;
    return (bd_size_t)config->size;
}

blockdevice_t *blockdevice_heap_create(uint8_t *heap_buf, size_t length)
{
    if (heap_buf == NULL || length == 0)
    {
        return NULL;
    }
    blockdevice_t *device = VFS_calloc(1, sizeof(blockdevice_t));
    if (device == NULL)
    {
        return NULL;
    }
    blockdevice_heap_config_t *config = calloc(1, sizeof(blockdevice_heap_config_t));
    if (config == NULL)
    {
        VFS_free(device);
        return NULL;
    }

    device->init = init;
    device->deinit = deinit;
    device->read = read;
    device->erase = erase;
    device->program = program;
    device->trim = trim;
    device->sync = sync;
    device->size = size;
    device->read_size = PICO_VFS_BLOCKDEVICE_HEAP_BLOCK_SIZE;
    device->erase_size = PICO_VFS_BLOCKDEVICE_HEAP_BLOCK_SIZE;
    device->program_size = PICO_VFS_BLOCKDEVICE_HEAP_BLOCK_SIZE;
    device->name = DEVICE_NAME;
    device->is_initialized = false;

    config->size = length;
    config->heap = heap_buf;
    device->config = config;
    device->init(device);
    return device;
}

void blockdevice_heap_free(blockdevice_t *device)
{
    device->deinit(device);
    VFS_free(device->config);
    VFS_free(device);
}
