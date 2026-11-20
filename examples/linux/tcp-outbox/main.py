import argparse
from contextlib import closing
from ipaddress import IPv4Address
import json
import socket
import sqlite3
import time
from uuid import UUID, uuid4


IO_TIMEOUT = 1.0
MAX_LINE = 256
RETRY_DELAY = 0.2


def receive_line(peer):
    """读取一条以 LF 结尾的记录。整条记录共用一个接收期限。"""
    deadline = time.monotonic() + IO_TIMEOUT
    line = bytearray()
    while len(line) < MAX_LINE:
        # 1. 每次只等尚余的时间，防止零星到达的字节反复延长等待。
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("record timeout")
        peer.settimeout(remaining)
        chunk = peer.recv(MAX_LINE - len(line))
        if not chunk:
            raise ConnectionError("peer closed before LF")
        line.extend(chunk)

        # 2. 读到 LF 才把整条记录交给调用者。每个连接只传一条。
        end = line.find(b"\n")
        if end >= 0:
            if end != len(line) - 1:
                raise ValueError("extra data in record")
            return bytes(line[:end])
    raise ValueError("record too long")


def enqueue(db, raw):
    # 1. 先生成长期稳定的消息 ID。事务提交后，重启仍可查到这条记录。
    message_id = str(uuid4())
    with db:
        db.execute("INSERT INTO pending(id, raw) VALUES (?, ?)", (message_id, raw))
    print(f"queued {message_id}")


def exchange(address, message_id, raw):
    request = json.dumps({"id": message_id, "raw": raw}).encode("ascii") + b"\n"
    with socket.create_connection(address, timeout=IO_TIMEOUT) as peer:
        peer.sendall(request)
        ack = receive_line(peer)
        if ack != f"ACK {message_id}".encode("ascii"):
            raise ValueError("acknowledgement ID mismatch")


def drain(db, address, attempts):
    while True:
        # 1. 总是读取最早的缓存项。重试复用同一个 ID，直到收到匹配确认。
        row = db.execute("SELECT id, raw FROM pending ORDER BY sequence LIMIT 1").fetchone()
        if row is None:
            return 0
        message_id, raw = row

        # 2. 连接失败、超时或确认无效时保留记录。达到本轮上限就带着缓存退出。
        for attempt in range(1, attempts + 1):
            try:
                exchange(address, message_id, raw)
                break
            except (OSError, ValueError) as error:
                print(f"attempt {attempt}: {error}", flush=True)
                if attempt == attempts:
                    return 2
                time.sleep(RETRY_DELAY)

        # 3. 收到匹配确认后才结束这条记录，然后处理下一条。
        with db:
            db.execute("DELETE FROM pending WHERE id = ?", (message_id,))
        print(f"confirmed {message_id}", flush=True)


def read_request(peer):
    body = json.loads(receive_line(peer))
    if not isinstance(body, dict):
        raise ValueError("request must be an object")
    message_id, raw = body.get("id"), body.get("raw")
    if not isinstance(message_id, str) or str(UUID(message_id)) != message_id:
        raise ValueError("invalid message ID")
    if type(raw) is not int or not 0 <= raw <= 4095:
        raise ValueError("raw sample out of range")
    return message_id, raw


def receive_one(db, peer, drop_first_ack):
    # 1. 校验记录并持久化。收到同一 ID 时读取已保存的值，避免重复插入。
    message_id, raw = read_request(peer)
    with db:
        result = db.execute("INSERT OR IGNORE INTO seen(id, raw) VALUES (?, ?)", (message_id, raw))
        saved, = db.execute("SELECT raw FROM seen WHERE id = ?", (message_id,)).fetchone()
        if saved != raw:
            raise ValueError("same ID with different payload")

    # 2. 数据库提交后再回 ACK。练习模式在一次新写入之后先断开，模拟确认丢失。
    if drop_first_ack and result.rowcount == 1:
        print(f"ack dropped {message_id}", flush=True)
        return False
    peer.settimeout(IO_TIMEOUT)
    peer.sendall(f"ACK {message_id}\n".encode("ascii"))
    return drop_first_ack


def serve(db, port, drop_first_ack):
    # 1. 实验接收端只监听回环地址。每个连接只取一条记录并确认。
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", port))
        listener.listen()
        print(f"listening {listener.getsockname()[1]}", flush=True)
        while True:
            peer, _ = listener.accept()
            with peer:
                # 2. 处理失败就关闭本次连接。监听端继续接下一次重试。
                try:
                    drop_first_ack = receive_one(db, peer, drop_first_ack)
                except (OSError, ValueError) as error:
                    print(f"rejected: {error}", flush=True)


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    add = commands.add_parser("enqueue")
    add.add_argument("db")
    add.add_argument("raw", type=int)
    send = commands.add_parser("drain")
    send.add_argument("db")
    send.add_argument("port", type=int)
    send.add_argument("--host", type=IPv4Address, default=IPv4Address("127.0.0.1"))
    send.add_argument("--attempts", type=int, default=3)
    receive = commands.add_parser("serve")
    receive.add_argument("db")
    receive.add_argument("port", type=int)
    receive.add_argument("--drop-first-ack", action="store_true")
    args = parser.parse_args()
    if args.command == "enqueue" and not 0 <= args.raw <= 4095:
        parser.error("raw sample must be in 0..4095")
    if args.command == "drain" and not (1 <= args.port <= 65535 and 1 <= args.attempts <= 20):
        parser.error("port must be in 1..65535 and attempts in 1..20")
    if args.command == "serve" and not 0 <= args.port <= 65535:
        parser.error("port must be in 0..65535")

    # 1. 接收端与发送端各用自己的数据库。进程退出时释放连接。
    with closing(sqlite3.connect(args.db)) as db:
        with db:
            if args.command == "serve":
                db.execute("CREATE TABLE IF NOT EXISTS seen(id TEXT PRIMARY KEY, raw INTEGER NOT NULL)")
            else:
                db.execute("CREATE TABLE IF NOT EXISTS pending("
                           "sequence INTEGER PRIMARY KEY, id TEXT NOT NULL UNIQUE, raw INTEGER NOT NULL)")
        if args.command == "serve":
            try:
                serve(db, args.port, args.drop_first_ack)
            except KeyboardInterrupt:
                pass
        elif args.command == "enqueue":
            enqueue(db, args.raw)
        else:
            return drain(db, (str(args.host), args.port), args.attempts)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
