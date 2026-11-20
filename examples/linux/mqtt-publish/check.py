import json
from pathlib import Path
import socket
import subprocess
import sys
from tempfile import TemporaryDirectory
from threading import Event
import time

import paho.mqtt.client as mqtt


def call(port, data):
    app = Path(__file__).with_name("main.py")
    return subprocess.run([sys.executable, str(app), "--port", str(port), "--timeout", "1"],
                          input=data, text=True, capture_output=True, timeout=6)


def broker_ready(broker, port):
    end = time.monotonic() + 4
    while broker.poll() is None and time.monotonic() < end:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError("loopback broker failed to start")


with TemporaryDirectory(prefix="kb-mqtt-") as stage:
    with socket.socket() as reserved:
        reserved.bind(("127.0.0.1", 0))
        port = reserved.getsockname()[1]
        # 1. 在没有 broker 时分别检查空输入、坏首行和连接失败。
        assert call(port, "").returncode == 0
        assert call(port, '{"adc_raw":true}\n').returncode == 2
        result = call(port, '{"adc_raw":837}\n')
        assert result.returncode == 1 and "acknowledged" not in result.stdout

    conf = Path(stage) / "mosquitto.conf"
    conf.write_text(f"listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n", encoding="ascii")
    broker = subprocess.Popen(["mosquitto", "-c", str(conf)],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    listener = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, protocol=mqtt.MQTTv311,
                           reconnect_on_failure=False)
    subscribed, received = Event(), Event()
    reports = []

    def on_connect(connected_client, _userdata, _flags, reason, _properties):
        if not reason.is_failure:
            connected_client.subscribe("lab/sky/adc", qos=1)

    def on_subscribe(_client, _userdata, _mid, reasons, _properties):
        if reasons and all(not result.is_failure for result in reasons):
            subscribed.set()

    def on_message(_client, _userdata, message):
        reports.append((message.topic, message.qos, json.loads(message.payload)))
        if len(reports) == 2:
            received.set()

    listener.on_connect = on_connect
    listener.on_subscribe = on_subscribe
    listener.on_message = on_message
    try:
        broker_ready(broker, port)
        listener.connect("127.0.0.1", port)
        listener.loop_start()
        assert subscribed.wait(3)

        # 2. 实际收到两份 QoS 1 消息，发布方也收到两份 PUBACK。
        result = call(port, '{"adc_raw":837}\n{"adc_raw":4095}\n')
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert result.stdout.count("acknowledged") == 2, result.stdout
        assert received.wait(3)
        assert reports == [("lab/sky/adc", 1, {"adc_raw": 837}),
                           ("lab/sky/adc", 1, {"adc_raw": 4095})]
    finally:
        listener.disconnect()
        listener.loop_stop()
        broker.terminate()
        broker.wait(timeout=3)

print("Mosquitto loopback: empty, bad JSON data, offline result, QoS 1 delivery and PUBACK")
