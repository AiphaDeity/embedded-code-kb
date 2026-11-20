# Linux 用 MQTT 逐条上报 ADC 采样

把每行 `{"adc_raw":837}` 发到 `lab/sky/adc`。收到 broker 的 QoS 1 `PUBACK` 后才继续下一行。输入可由[天空星 ADC 到 JSON](../projects/sensor-to-json.md)的主机程序生成。

## 沿执行路径读代码

1. `main()` 先从标准输入读取一行。`readings()` 只交付有效的 12 位整数，空输入直接退出，首行数据错误会在建立连接前报告。
2. `publish()` 建立连接，网络线程把 `CONNACK` 交给等待中的主线程。只在 broker 接受会话后开始发布。
3. 主线程提交一条 QoS 1 消息并等待该条的 `PUBACK`。超时就退出，接收方此时仍可能已经见到该消息。需要以业务处理结果为确认并跨进程补传时，见 [SQLite 缓存补传](tcp-outbox.md)。

## 完整源码

<a href="../../examples/linux/mqtt-publish/main.py" download="main.py">下载 main.py</a> · <a href="../../examples/linux/mqtt-publish/check.py" download="check.py">下载检查脚本</a> · <a href="../../examples/linux/mqtt-publish/requirements.txt" download="requirements.txt">下载依赖清单</a>

<<< ../../examples/linux/mqtt-publish/main.py

## 在 Linux 回环网络复现

安装 Python 3.8 及以上、Mosquitto 和 `mosquitto_sub`。从仓库根目录创建虚拟环境，从同一环境运行发布脚本。

```sh
mkdir -p .work
python3 -m venv .work/mqtt-venv
.work/mqtt-venv/bin/pip install -r examples/linux/mqtt-publish/requirements.txt
printf 'listener 18884 127.0.0.1\nallow_anonymous true\npersistence false\n' > .work/mqtt-local.conf
mosquitto -c .work/mqtt-local.conf
```

另开终端，先启动订阅。

```sh
mosquitto_sub -h 127.0.0.1 -p 18884 -t lab/sky/adc -q 1 -C 2
```

再用第三个终端逐条发布。

```sh
printf '{"adc_raw":837}\n{"adc_raw":1024}\n' | \
  .work/mqtt-venv/bin/python examples/linux/mqtt-publish/main.py --port 18884
```

发布端打印两行 `acknowledged`。订阅端输出 837 和 1024 两条 JSON 后退出。

## 练习

**预测题**　第一行是 `{"adc_raw":837}`，第二行是 `{"adc_raw":4096}`。broker 在线时会收到几条？

<details><summary>核对预测</summary>

第一行收到 `PUBACK` 后才进入第二行。第二行超出 12 位范围，程序打印 `line 2` 并以退出码 2 结束。第一条已经发布。

</details>

**改动题**　让订阅和发布都使用 `lab/sky/adc/test`，再停掉 broker 重发一条合法数据。

<details><summary>核对改法</summary>

订阅命令改 `-t lab/sky/adc/test`，发布命令加 `--topic lab/sky/adc/test`。broker 停止后发布端退出码为 1，终端没有 `acknowledged`。

</details>

## 验证记录

在 Alpine WSL 使用 Paho 2.1.0 和真实 Mosquitto 2.0.21，核对空输入、无效数据、broker 离线、订阅收到两条 QoS 1 消息、发布端等待两次确认。运行 `.work/mqtt-venv/bin/python examples/linux/mqtt-publish/check.py` 可用临时回环端口复查。

接口参考见 [Paho 客户端 API](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html)和 [Mosquitto 配置手册](https://mosquitto.org/man/mosquitto-conf-5.html)。

