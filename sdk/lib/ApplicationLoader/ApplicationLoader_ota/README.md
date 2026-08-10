# ApplicationLoader Loader OTA 使用说明

## 1. 功能与前置条件

本目录提供独立的 Loader OTA 链路：设备端 TCP 服务监听 `20203`，上位机脚本发送 Loader 镜像，底层 API 在另一 Loader 槽完成擦除、写入、回读和 marker 最后提交。

使用前必须满足：

- 产品有两份明确划分的 Loader 槽，并已写入正式 Flash 分区表。
- 用户工程实现普通强符号 `ApplicationLoader_ota_fwinfo_get()`；库内不提供默认实现或 weak fallback。
- Loader 镜像格式符合设备使用的公开 `struct fwinfo_hdr` 格式。
- 设备串口日志可用，以便读取 wire 响应之外的精确错误码。

## 2. 配置两个 Loader 分区

在用户工程（FPV 示例为 `project/demo/fpv/txw82x/txw82xApp/device.c`）实现：

```c
int32 ApplicationLoader_ota_fwinfo_get(
    struct ApplicationLoader_ota_fwinfo *pinfo)
{
    if (pinfo == NULL) {
        return -1;
    }

    pinfo->flash0 = &flash0;
    pinfo->addr0 = PRODUCT_LOADER_SLOT0_ADDR;
    pinfo->size0 = PRODUCT_LOADER_SLOT0_SIZE;
    pinfo->flash1 = &flash0;
    pinfo->addr1 = PRODUCT_LOADER_SLOT1_ADDR;
    pinfo->size1 = PRODUCT_LOADER_SLOT1_SIZE;
    return 0;
}
```

`PRODUCT_LOADER_SLOT0_ADDR/SIZE` 和 `PRODUCT_LOADER_SLOT1_ADDR/SIZE` 只是必须由产品替换或定义的常量名称，其值必须来自正式 Flash 分区表。`size0/size1` 表示槽位保留容量，不是当前 Loader 镜像大小；若槽位位于不同 Flash，应同时把 `flash0`、`flash1` 改为各自设备。

**严禁复用 APP OTA 的 `0` 或 `flash0.size / 2` 作为 Loader 分区配置。** APP 固件槽与 Loader 槽是不同对象，只有正式分区表能够确定 Loader 地址、容量和边界。

当前 FPV 示例仓库没有正式 Loader 双槽地址和容量，因此 `device.c` 中的强配置函数故意返回失败，并配置空 Flash、零容量，即 fail-closed。未按产品分区表修改该函数前，Loader OTA 会以 `-17` 拒绝升级且不擦写 Flash。

共享 SRAM 中的 `user_info.loader_bytes` 仅表示**当前 Loader 镜像的实际大小**，用于保护当前 Loader 占用区间。新镜像允许大于该值，但其完整对齐擦除范围必须位于目标槽的 `size0/size1` 内。

### 2.1 元数据地址一致性

共享元数据中的 `ld_addr[i]` 只用于校验，不直接决定写入地址。非无效元数据地址在比较前按**对应槽配置的 Flash size** 归一化，等价于：

```text
normalized_addr = ld_addr[i] % slot_flash[i]->size
```

归一化只用于比较，不修改共享 SRAM；实际写入地址始终来自 `ApplicationLoader_ota_fwinfo_get()`。

- **单有效 Loader**：`target_ld_index` 指向当前唯一有效槽。当前有效槽的归一化地址必须严格匹配其用户配置；另一目标槽尚未有效，其 `ld_addr[target_index]` 内容不可信，必须忽略并直接使用用户配置地址创建第二份 Loader。
- **双有效 Loader**：slot0 和 slot1 的归一化元数据地址必须分别严格匹配 `addr0` 和 `addr1`；任一槽不匹配都拒绝升级。

地址不匹配返回 `-18 APPLICATION_LOADER_OTA_ERR_ADDR_MISMATCH`。所有配置和地址一致性检查都在 `spi_nor_open()`、擦除、写入或读取之前完成，因此 `-18` 时 Flash I/O 为零。

## 3. 设备端工程接入

需要 Loader TCP OTA 的应用工程加入本目录的服务源码/头文件，并链接包含底层 API 的源码或更新后的 common 库。应用初始化示例：

```c
#include "lib/ApplicationLoader/ApplicationLoader_ota/ApplicationLoader_ota.h"

eloop_init();
os_task_create("eloop_run", user_eloop_run, NULL,
               OS_TASK_PRIORITY_NORMAL + 2, 0, NULL, 2048);
os_sleep_ms(1);
ApplicationLoader_ota_server_start();
```

FPV 当前在 `app/app_fpv.c` 中按上述顺序启动服务。三个 OTA 服务互不替代、端口相互独立：

| 服务 | 端口 |
|---|---:|
| APP OTA | `5007` |
| PSRAM APP OTA | `20202` |
| Loader OTA | `20203` |

Loader OTA 根据 `loader_info` 与用户双槽配置选择当前槽和目标槽，**不得调用旧 `get_current_loader_addr()`**；该旧接口属于 APP 当前固件地址路径，不能作为 Loader OTA 依赖。升级成功后设备只返回结果并记录日志，不执行重启；用户检查日志后自行决定何时重启。

## 4. 上位机脚本

脚本路径：

```text
sdk\lib\ApplicationLoader\ApplicationLoader_ota\ApplicationLoader_ota.py
```

基本 Windows 示例：

```text
python sdk\lib\ApplicationLoader\ApplicationLoader_ota\ApplicationLoader_ota.py 192.168.169.1 path\to\loader.bin --version Loader-V2.7.1
```

自定义连接和发送参数示例：

```text
python sdk\lib\ApplicationLoader\ApplicationLoader_ota\ApplicationLoader_ota.py 192.168.169.1 path\to\loader.bin --port 20203 --connect-timeout 10 --result-timeout 30 --chunk-size 4096
```

实际参数：

| 参数 | 默认值 | 说明 |
|---|---:|---|
| `--port` | `20203` | Loader OTA TCP 端口，范围 1～65535。 |
| `--version` | `Loader` | 协议显示版本，UTF-8 编码后最多 15 字节；镜像头 SVN 版本仍由设备按当前最新版本加一。 |
| `--connect-timeout` | `10` | 建立连接和等待 36 字节 ACK 的秒数。 |
| `--result-timeout` | `30` | 镜像发送完成后等待设备最终结果的秒数。 |
| `--chunk-size` | `4096` | 每次从镜像读取并发送的字节数，必须大于零。 |

## 5. 正常升级流程

1. 设备初始化 eloop 并成功启动 `20203` 服务。
2. 根据正式 Flash 分区表确认 `ApplicationLoader_ota_fwinfo_get()` 中两个 Loader 分区均配置正确。
3. 运行上位机脚本，等待设备返回 `Upgrade_ok`。
4. 检查设备串口日志，确认升级过程没有底层错误。
5. 只有日志中无 `error=-16` 时，才由用户手动重启设备。
6. 重启后确认 ROM 扫描并选择 SVN 版本为升级前最新 Loader 版本加一的新 Loader。

脚本不会发送 reset/reboot 命令。即使已收到 `Upgrade_ok`，也必须先完成日志检查，再手动重启。

## 6. 错误与安全处理

TCP wire 为兼容现有工具，失败时统一只返回 `Upgrade_err`。wire 不携带底层数值错误码；精确错误码、失败偏移和原因必须查看设备串口日志。

| 错误 | 含义与处理 |
|---|---|
| `-17 APPLICATION_LOADER_OTA_ERR_CONFIG` | 用户配置函数失败，或 Flash/地址/容量配置无效。检查强配置函数和正式分区表；该错误在擦写前返回，不擦写 Flash。 |
| `-18 APPLICATION_LOADER_OTA_ERR_ADDR_MISMATCH` | 用户配置与 `loader_info.ld_addr[]` 按对应 Flash size 归一化后的地址不一致。检查 ROM/Loader 元数据和产品分区；该错误发生时 Flash I/O 为零。 |
| `-16 APPLICATION_LOADER_OTA_ERR_COMMIT_UNCERTAIN` | marker 提交失败后无法确认目标 marker 已失效。禁止重启、掉电或继续普通升级，必须保持设备供电并执行产品定义的恢复策略。 |

其他错误同样会以 `Upgrade_err` 返回；不要仅根据主机提示猜测原因，应以串口日志中的精确 `error=<code>` 为准。

## 7. 迁移后的首次构建

本功能文件曾从旧目录迁移到当前目录。若工作树在迁移前已经编译过，CDK 的 `Rebuild project fpv app` 可能保留旧路径产生的 object 或 archive member，链接时出现重复定义。

迁移后的首次构建建议：

1. 先运行 VS Code 任务 `Clean project fpv app`；若本地构建系统仍保留旧成员，则仅清理该工程的派生构建产物。
2. 再运行 `Rebuild project fpv app`。

只应清理可重新生成的工程构建产物。不要删除任何源码，也不要删除或替换正式发布库。

## 8. 目录文件

```text
ApplicationLoader_ota/
├── ApplicationLoader_ota_api.c
├── ApplicationLoader_ota_api.h
├── ApplicationLoader_ota.c
├── ApplicationLoader_ota.h
├── ApplicationLoader_ota.py
└── README.md
```

CDK 工程的 `ApplicationLoader/ApplicationLoader_ota` 虚拟目录应包含以上六个文件，每个文件恰好一次。
