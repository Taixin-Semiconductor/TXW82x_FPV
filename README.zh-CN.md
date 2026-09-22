# TXW82x FPV SDK

[English](README.md) | 简体中文

本仓库包含泰芯半导体 TXW82x 系列的软件开发套件。泰芯官网将 TXW82x 定位为面向图传与智能终端产品的 Wi-Fi SoC 系列。

## 版本发布

各版本的 SDK 源码快照、中英文发布说明及下载文件统一发布在
[GitHub Releases](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases)。
发布版本以不可变的附注标签为准，不从本 README 或分支名称判断。标签格式为
`TXW82x_FPV-v<版本>-<来源版本>`。

- [最新版本](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases/latest)
- [全部版本](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases)

具体版本号、来源版本、升级说明和受影响组件应记录在对应的 GitHub Release 中，
本 README 不再重复这些随版本变化的信息。

## 开发环境

本 SDK 使用[玄铁 CDK](https://www.xrvm.cn/soft-tools/tools/CDK)。CDK 是面向玄铁及通用 RISC-V 处理器、仅支持 Windows 的集成开发环境。

玄铁 CDS 是独立的开发工具集，同时支持 Windows 和 Linux。CDK 与 CDS 是不同的产品，不应视为同一个开发环境。

> **主机支持：** 本仓库支持的工作流程使用 Windows 和 CDK。本仓库未提供或验证基于 CDS 的 Linux 构建流程。

仓库中的文本文件统一使用 LF 换行符，以兼容 Windows 和 Linux 工具。

## 快速开始

1. 在 Windows 上安装玄铁 CDK。
2. 在 CDK 中打开或导入 [`project/txw82xApp.cdkproj`](project/txw82xApp.cdkproj)。
3. 选择 `FLASH` 构建配置。
4. 在 CDK 中构建项目。生成的 `Obj`、`Lst`、固件镜像和打包文件已由 `.gitignore` 排除。

## 仓库结构

- `project/` — TXW82x 应用、CDK 工程、配置及打包脚本
- `sdk/` — 芯片 SDK 源码、头文件、驱动、中间件及库
- `libs/` — TXW82x SDK 预编译库
- `csky/` — C-SKY/RISC-V 内核支持及运行时组件
- `ohos/` — OpenHarmony LiteOS-M 组件
- `tools/` — 随附的开发工具

## 量产使用

产品量产前，请检查并替换所有演示或默认凭据、私钥、证书以及设备专用配置。

## 相关链接

- [泰芯半导体产品官网](https://taixin-semi.com)
- [玄铁 CDK 与 CDS 开发工具](https://www.xrvm.cn/soft-tools/tools/CDK)

## 许可证

Copyright 2026 Taixin Semiconductor.

本项目采用 [Apache License 2.0](LICENSE) 许可证。第三方组件保留其各自的版权及许可证声明。
