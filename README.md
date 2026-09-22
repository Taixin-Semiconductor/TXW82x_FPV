# TXW82x FPV SDK

English | [简体中文](README.zh-CN.md)

This repository contains the software development kit for the Taixin Semiconductor TXW82x family. Taixin describes TXW82x as a Wi-Fi SoC family for video-transmission and intelligent-terminal products.

## Releases

Versioned SDK source snapshots, bilingual release notes, and downloadable
assets are published on
[GitHub Releases](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases).
Use the immutable annotated tag, rather than this README or a branch name, as
the release identity. Tags follow the format
`TXW82x_FPV-v<version>-<source-revision>`.

- [Latest release](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases/latest)
- [All releases](https://github.com/Taixin-Semiconductor/TXW82x_FPV/releases)

Release-specific versions, source revisions, upgrade notes, and affected
components belong to the corresponding GitHub Release and are intentionally
not duplicated in this README.

## Development environment

Use [XuanTie CDK](https://www.xrvm.cn/soft-tools/tools/CDK), a Windows-only integrated development environment for XuanTie and general RISC-V processors, to work with this SDK.

XuanTie CDS is a separate development toolset that supports both Windows and Linux. CDK and CDS are distinct products and should not be treated as the same development environment.

> **Host support:** Use Windows and CDK for the supported repository workflow. A CDS-based Linux build workflow is not provided or validated by this repository.

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
