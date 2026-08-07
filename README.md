# TXW82x FPV SDK

English | [简体中文](README.zh-CN.md)

This repository contains the software development kit for the Taixin Semiconductor TXW82x family. Taixin describes TXW82x as a Wi-Fi SoC family for video-transmission and intelligent-terminal products.

## Release information

- Version: `v2.7.0.7-42229`
- Source: SVN release `v2.7.0.7-42229`
- Fix known issues and improve the BBM, walkie-talkie, and dashcam solution code.

## Development environment

This release requires [XuanTie CDK](https://www.xrvm.cn/soft-tools/tools/CDK), a Windows-only integrated development environment for XuanTie and general RISC-V processors.

XuanTie CDS is a separate development toolset that supports both Windows and Linux. CDK and CDS are distinct products and should not be treated as the same development environment.

> **Host support:** Use Windows and CDK for this SDK release. A CDS-based Linux build environment has not been prepared or validated for this release yet.

Text files are stored with LF line endings for compatibility across Windows and Linux tools.

## Getting started

1. On Windows, install XuanTie CDK.
2. Open or import [`project/txw82xApp.cdkproj`](project/txw82xApp.cdkproj) in CDK.
3. Select the `FLASH` build configuration.
4. Build the project from CDK. Generated `Obj`, `Lst`, firmware-image, and packaging files are excluded by `.gitignore`.

## Repository layout

- `project/` — TXW82x application, CDK project, configuration, and packaging scripts
- `sdk/` — chip SDK sources, headers, drivers, middleware, and libraries
- `libs/` — prebuilt TXW82x SDK libraries
- `csky/` — C-SKY/RISC-V core support and runtime components
- `ohos/` — OpenHarmony LiteOS-M components
- `tools/` — bundled development utilities

## Production use

Review and replace all demonstration or default credentials, private keys, certificates, and device-specific settings before shipping a product.

## Links

- [Taixin Semiconductor product website](https://taixin-semi.com)
- [XuanTie CDK and CDS development tools](https://www.xrvm.cn/soft-tools/tools/CDK)

## License

Copyright 2026 Taixin Semiconductor.

Licensed under the [Apache License 2.0](LICENSE). Third-party components retain their respective copyright and license notices.
