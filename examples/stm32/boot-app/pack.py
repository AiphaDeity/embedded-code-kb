import argparse
from pathlib import Path
import struct
import zlib


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("header", type=Path)
    parser.add_argument("--origin", type=lambda value: int(value, 0), default=0x08008000)
    parser.add_argument("--capacity", type=lambda value: int(value, 0), default=0x78000)
    args = parser.parse_args()

    # 1. 读 objcopy 输出的纯 Flash 内容。最小镜像包括向量表和入口指令。
    image = args.image.read_bytes()
    if not 0x18A <= len(image) <= args.capacity:
        parser.error("image does not fit App slot")
    _, reset = struct.unpack_from("<II", image)
    if reset & 1 == 0 or not args.origin + 0x188 <= (reset & ~1) <= args.origin + len(image) - 2:
        parser.error("reset vector is outside App slot")

    # 2. 按固定小端格式写独立描述符。CRC 涵盖 App 向量到 Flash 装载数据的全部字节。
    crc = zlib.crc32(image)
    args.header.write_bytes(struct.pack("<4sIII", b"KBA1", 1, len(image), crc))
    print(f"App image: {len(image)} bytes, CRC-32 {crc:08X}")


if __name__ == "__main__":
    main()
