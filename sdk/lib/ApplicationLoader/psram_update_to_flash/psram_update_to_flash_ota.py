#!/usr/bin/env python3
"""通过 ApplicationLoader TCP 协议将 APP.bin 暂存到 PSRAM 并升级到 Flash。

示例：
    python psram_update_to_flash_ota.py 192.168.1.100 APP.bin
    python psram_update_to_flash_ota.py 192.168.1.100 APP.bin --version V2.7.0
    python .\psram_update_to_flash_ota.py 192.168.169.1 ..\..\..\project\demo\fpv\txw82x\txw82xApp\APP.bin --version V2.7.0
"""

from __future__ import annotations

import argparse
import socket
import struct
import sys
import time
from pathlib import Path
from typing import Final

REQUEST_COMMAND: Final = 0xA0
ACK_COMMAND: Final = 0xA1
DEFAULT_PORT: Final = 20202
DEFAULT_CONNECT_TIMEOUT: Final = 10.0
DEFAULT_RESULT_TIMEOUT: Final = 30.0
DEFAULT_CHUNK_SIZE: Final = 64 * 1024
MAX_FILE_SIZE: Final = 0x7FFFFFFF

TCP_HEADER_FORMAT: Final = "<9I"
TCP_HEADER_SIZE: Final = struct.calcsize(TCP_HEADER_FORMAT)
FIRMWARE_INFO_FORMAT: Final = "<16si"
FIRMWARE_INFO_SIZE: Final = struct.calcsize(FIRMWARE_INFO_FORMAT)

RESULT_OK: Final = b"Upgrade_ok\x00"
RESULT_ERROR: Final = b"Upgrade_err\x00"


class UpgradeError(RuntimeError):
    """设备连接、协议交互或固件传输失败。"""


def receive_exact(connection: socket.socket, size: int) -> bytes:
    """从 TCP 连接精确接收指定字节数。"""
    data = bytearray()
    while len(data) < size:
        chunk = connection.recv(size - len(data))
        if not chunk:
            raise UpgradeError(
                f"连接已关闭，只接收到 {len(data)}/{size} 字节"
            )
        data.extend(chunk)
    return bytes(data)


def encode_version(version: str) -> bytes:
    """编码固件版本字段，并保留结尾 NUL。"""
    encoded = version.encode("utf-8")
    if len(encoded) > 15:
        raise UpgradeError("版本字符串 UTF-8 编码后不能超过 15 字节")
    return encoded.ljust(16, b"\x00")


def format_progress(sent_bytes: int, total_bytes: int, started_at: float) -> str:
    """生成单行传输进度信息。"""
    elapsed = max(time.monotonic() - started_at, 0.001)
    percent = sent_bytes * 100.0 / total_bytes
    speed_kib = sent_bytes / elapsed / 1024.0
    return (
        f"\r发送进度: {percent:6.2f}%  "
        f"{sent_bytes}/{total_bytes} 字节  {speed_kib:8.1f} KiB/s"
    )


def wait_for_result(connection: socket.socket) -> bytes:
    """等待设备返回 NUL 结尾的 Upgrade_ok 或 Upgrade_err。"""
    response = bytearray()
    maximum_size = max(len(RESULT_OK), len(RESULT_ERROR))

    while len(response) < maximum_size:
        chunk = connection.recv(maximum_size - len(response))
        if not chunk:
            break
        response.extend(chunk)
        if b"\x00" in response:
            break

    result = bytes(response)
    if result.startswith(RESULT_OK):
        return RESULT_OK
    if result.startswith(RESULT_ERROR):
        return RESULT_ERROR

    printable = result.rstrip(b"\x00").decode("ascii", errors="replace")
    raise UpgradeError(f"设备返回未知结果: {printable!r}")


def upgrade_app(
    host: str,
    port: int,
    firmware_path: Path,
    version: str,
    connect_timeout: float,
    result_timeout: float,
    chunk_size: int,
) -> None:
    """连接设备并发送 APP 固件。"""
    if not firmware_path.is_file():
        raise UpgradeError(f"固件文件不存在: {firmware_path}")

    file_size = firmware_path.stat().st_size
    if file_size <= 0:
        raise UpgradeError("固件文件不能为空")
    if file_size > MAX_FILE_SIZE:
        raise UpgradeError(
            f"固件大小 {file_size} 超过协议上限 {MAX_FILE_SIZE} 字节"
        )
    if not 1 <= port <= 65535:
        raise UpgradeError("TCP 端口必须在 1~65535 范围内")
    if chunk_size <= 0:
        raise UpgradeError("发送分块大小必须大于 0")

    version_field = encode_version(version)
    request_header = struct.pack(TCP_HEADER_FORMAT, REQUEST_COMMAND, *([0] * 8))
    firmware_info = struct.pack(FIRMWARE_INFO_FORMAT, version_field, file_size)

    print(f"设备地址: {host}:{port}")
    print(f"固件文件: {firmware_path.resolve()}")
    print(f"固件版本: {version}")
    print(f"固件大小: {file_size} 字节")

    try:
        connection = socket.create_connection(
            (host, port), timeout=connect_timeout
        )
    except OSError as error:
        raise UpgradeError(f"无法连接设备: {error}") from error

    with connection:
        try:
            connection.settimeout(connect_timeout)
            connection.sendall(request_header)

            ack_data = receive_exact(connection, TCP_HEADER_SIZE)
            ack_fields = struct.unpack(TCP_HEADER_FORMAT, ack_data)
            if ack_fields[0] != ACK_COMMAND:
                raise UpgradeError(
                    f"设备应答命令错误: 0x{ack_fields[0]:08X}，"
                    f"期望 0x{ACK_COMMAND:08X}"
                )

            connection.sendall(firmware_info)
            sent_bytes = 0
            started_at = time.monotonic()

            with firmware_path.open("rb") as firmware:
                while True:
                    chunk = firmware.read(chunk_size)
                    if not chunk:
                        break
                    connection.sendall(chunk)
                    sent_bytes += len(chunk)
                    print(
                        format_progress(sent_bytes, file_size, started_at),
                        end="",
                        flush=True,
                    )

            print()
            connection.settimeout(result_timeout)
            result = wait_for_result(connection)
            if result == RESULT_ERROR:
                raise UpgradeError("设备接收或暂存固件失败")
        except socket.timeout as error:
            raise UpgradeError("等待设备响应超时") from error
        except OSError as error:
            raise UpgradeError(f"TCP 通信失败: {error}") from error

    print("设备已接收完整固件，并开始从 PSRAM 升级到 Flash。")
    print("升级期间请勿断电，等待设备完成升级并重新启动。")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="通过 TCP 将 APP.bin 发送到设备 PSRAM，并请求二次 Loader 写入 Flash。"
    )
    parser.add_argument("host", help="设备 IPv4 地址或主机名")
    parser.add_argument("firmware", type=Path, help="需要升级的 APP.bin 路径")
    parser.add_argument(
        "--port", type=int, default=DEFAULT_PORT, help="TCP 端口，默认 20202"
    )
    parser.add_argument(
        "--version",
        default="APP",
        help="固件版本，UTF-8 编码后最多 15 字节，默认 APP",
    )
    parser.add_argument(
        "--connect-timeout",
        type=float,
        default=DEFAULT_CONNECT_TIMEOUT,
        help="连接和协议应答超时秒数，默认 10",
    )
    parser.add_argument(
        "--result-timeout",
        type=float,
        default=DEFAULT_RESULT_TIMEOUT,
        help="等待升级接收结果的超时秒数，默认 30",
    )
    parser.add_argument(
        "--chunk-size",
        type=int,
        default=DEFAULT_CHUNK_SIZE,
        help="主机发送分块字节数，默认 65536",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        upgrade_app(
            host=arguments.host,
            port=arguments.port,
            firmware_path=arguments.firmware,
            version=arguments.version,
            connect_timeout=arguments.connect_timeout,
            result_timeout=arguments.result_timeout,
            chunk_size=arguments.chunk_size,
        )
    except UpgradeError as error:
        print(f"升级失败: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\n升级已由用户取消。", file=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
