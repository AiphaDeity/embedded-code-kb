import argparse
from ipaddress import ip_address
from itertools import chain
import json
import sys
from threading import Event

import paho.mqtt.client as mqtt


def readings(stream):
    """每行一条 JSON。只有范围有效的 ADC 结果才交给 MQTT。"""
    index = 0
    while line := stream.readline(257):
        index += 1
        try:
            if len(line) > 256:
                raise ValueError
            record = json.loads(line)
            raw = record.get("adc_raw") if isinstance(record, dict) else None
            if type(raw) is not int or not 0 <= raw <= 4095:
                raise ValueError
        except (ValueError, TypeError) as error:
            raise ValueError(f"line {index}: expected adc_raw in 0..4095") from error
        yield index, json.dumps({"adc_raw": raw}, separators=(",", ":"))


def publish(args, rows):
    # 1. 连接与 MQTT 确认各自有期限。收到成功的 CONNACK 才开始发布输入。
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, protocol=mqtt.MQTTv311,
                         reconnect_on_failure=False)
    client.connect_timeout = args.timeout
    connected = Event()
    response = None

    def on_connect(_client, _userdata, _flags, reason, _properties):
        nonlocal response
        response = reason
        connected.set()

    client.on_connect = on_connect
    if client.connect(str(args.host), args.port, keepalive=15) != mqtt.MQTT_ERR_SUCCESS:
        raise ConnectionError("connect failed")

    try:
        if client.loop_start() != mqtt.MQTT_ERR_SUCCESS:
            raise ConnectionError("network loop failed")
        if not connected.wait(args.timeout):
            raise TimeoutError("CONNACK timeout")
        if response is None or response.is_failure:
            raise ConnectionError(f"broker refused connection: {response}")

        # 2. 每次只提交一条 QoS 1 消息。等待该条的 PUBACK 后才读取下一条。
        for index, payload in rows:
            sent = client.publish(args.topic, payload, qos=1, retain=False)
            if sent.rc != mqtt.MQTT_ERR_SUCCESS:
                raise ConnectionError(f"line {index}: {mqtt.error_string(sent.rc)}")
            sent.wait_for_publish(timeout=args.timeout)
            if not sent.is_published():
                raise TimeoutError(f"line {index}: PUBACK timeout")
            print(f"line {index}: acknowledged, mid={sent.mid}", flush=True)
    finally:
        # 3. 结束或报错都关闭会话并回收网络线程。
        client.disconnect()
        client.loop_stop()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", type=ip_address, default=ip_address("127.0.0.1"))
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--topic", default="lab/sky/adc")
    parser.add_argument("--timeout", type=float, default=3.0, metavar="SECONDS")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 0.1 <= args.timeout <= 60.0:
        parser.error("port must be in 1..65535 and timeout in 0.1..60 seconds")
    if not 1 <= len(args.topic.encode("utf-8")) <= 65535 or any(ch in args.topic for ch in "+#\0"):
        parser.error("topic must be a concrete MQTT topic")

    try:
        # 1. 无输入就直接结束。先确认首行有效，再连到 broker。
        rows = readings(sys.stdin)
        first = next(rows, None)
        if first is None:
            return 0
        publish(args, chain((first,), rows))
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    except (OSError, RuntimeError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
