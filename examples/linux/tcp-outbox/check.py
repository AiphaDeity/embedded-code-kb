from contextlib import closing
from pathlib import Path
import socket
import sqlite3
import subprocess
import sys
from tempfile import TemporaryDirectory
import time


app = Path(__file__).with_name("main.py")


def run(*args, expected=0):
    result = subprocess.run([sys.executable, str(app), *map(str, args)],
                            text=True, capture_output=True, timeout=5)
    assert result.returncode == expected, (result.args, result.returncode, result.stdout, result.stderr)
    return result.stdout


def query(path, statement):
    with closing(sqlite3.connect(path)) as db:
        return db.execute(statement).fetchall()


with TemporaryDirectory(prefix="kb-outbox-") as tmp:
    pending = Path(tmp) / "pending.db"
    seen = Path(tmp) / "seen.db"
    ids = []
    for value in (837, 1024):
        ids.append(run("enqueue", pending, value).strip().split()[1])

    with socket.socket() as stopped:
        stopped.bind(("127.0.0.1", 0))
        port = stopped.getsockname()[1]
        log = run("drain", pending, port, "--attempts", 2, expected=2)
        assert "attempt 1" in log and "attempt 2" in log, log
    assert query(pending, "SELECT id, raw FROM pending ORDER BY sequence") == [(ids[0], 837), (ids[1], 1024)]

    server = subprocess.Popen([sys.executable, str(app), "serve", str(seen), str(port), "--drop-first-ack"],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert server.stdout.readline().strip() == f"listening {port}"
        first = run("drain", pending, port, "--attempts", 1, expected=2)
        assert "attempt 1" in first and "confirmed" not in first, first
        assert server.stdout.readline().strip() == f"ack dropped {ids[0]}"
        assert query(seen, "SELECT id, raw FROM seen") == [(ids[0], 837)]
        assert query(pending, "SELECT count(*) FROM pending") == [(2,)]

        result = run("drain", pending, port)
        assert result.splitlines() == [f"confirmed {message_id}" for message_id in ids], result
        assert run("drain", pending, port) == ""
        assert query(pending, "SELECT count(*) FROM pending") == [(0,)]
        assert query(seen, "SELECT id, raw FROM seen ORDER BY rowid") == [(ids[0], 837), (ids[1], 1024)]

        with socket.create_connection(("127.0.0.1", port), timeout=2) as peer:
            request = ('{"id":"' + ids[0] + '","raw":5}\n').encode("ascii")
            peer.sendall(request[:17])
            time.sleep(0.02)
            peer.sendall(request[17:])
            assert peer.recv(256) == b""
        assert server.stdout.readline().strip() == "rejected: same ID with different payload"
        assert query(seen, "SELECT raw FROM seen ORDER BY rowid") == [(837,), (1024,)]
    finally:
        server.terminate()
        try:
            server.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.communicate(timeout=3)

print("PASS offline retry, persisted no-ACK, ordered replay, dedupe, conflicting fragmented payload")
