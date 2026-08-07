#ifndef _HGIC_SYSCFG_H_
#define _HGIC_SYSCFG_H_
#ifdef __cplusplus
extern "C" {
#endif

#define SYSCFG_MAGIC (0x1234)

#define SYSCFG_NUM   (20)

enum SYSCFG_ERASE_MODE{
    SYSCFG_ERASE_MODE_SECTOR,
    SYSCFG_ERASE_MODE_BLOCK,
    SYSCFG_ERASE_MODE_CHIP,
};

struct syscfg_info {
    struct spi_nor_flash *flash1, *flash2;
    uint32 addr1, addr2;
    uint32 size;
    uint8  erase_mode;
};

struct syscfg_state {
    uint16 state;
    uint16 size;
    uint16 offset;
    uint16 check;
};

int32 syscfg_init(const char *name, void *cfg, uint32 size);
int32 syscfg_read(const char *name, void *cfg, uint32 size);
int32 syscfg_write(const char *name, void *cfg, uint32 size);
int32 syscfg_info_get(struct syscfg_info *pinfo, const char *name);
void syscfg_loaddef(const char *name);
int32 syscfg_save(void);

#ifdef __cplusplus
}
#endif
#endif
