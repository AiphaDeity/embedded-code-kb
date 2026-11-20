# GT911 读触点，ILI9341 只刷新标记区域

按住或拖动时在 ILI9341 上显示一个白色标记，松开后擦掉。读取触摸点、坐标换算和 LCD 区域写入可沿调用顺序分别观察。

## 板型与接线

LCD 接线沿用 [FSMC 点屏示例](ili9341-pixel.md)。GT911 使用独立的 3.3 V I²C 模块，PB6/PB7 接 SCL/SDA，PC7 接触摸 RESET，PB5 接 INT。I²C 需要外部上拉。接线以本示例引脚为准，使用扩展底板时对照排针复用。

## 沿执行路径读代码

1. 复位时 INT 保持低，与 GT911 的 0x5D 地址相配。核对产品 ID，再读原始分辨率。
2. 状态 ready 时读第一个触点，成功读取并清状态后才把数据交给画点。I²C 失败时保留上一帧的显示。
3. 把原始坐标映射到 240×320，擦掉旧标记和绘制新区域。屏幕边缘裁掉越界像素。

## 完整源码

<a href="../../../examples/stm32/fsmc/ili9341-touch/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/fsmc/ili9341-touch/main.c

<a href="../../../examples/stm32/fsmc/common/ili9341.h" download="ili9341.h">下载 LCD 区域驱动</a> · <a href="../../../examples/stm32/fsmc/ili9341-touch/coordinates.h" download="coordinates.h">下载坐标和裁剪</a> · <a href="../../../examples/stm32/fsmc/ili9341-touch/check.c" download="check.c">下载主机检查</a>

<<< ../../../examples/stm32/fsmc/ili9341-touch/coordinates.h{c}

<a href="../../../examples/stm32/fsmc/ili9341-touch/gt911.h" download="gt911.h">下载 GT911 通信驱动</a>

<<< ../../../examples/stm32/fsmc/ili9341-touch/gt911.h{c}

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包目录换成本机路径后，在仓库根目录运行以下命令。

```sh
cmake -S examples/stm32/fsmc/ili9341-touch -B .work/sky-ili9341-touch -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-ili9341-touch
```

按住和松开时观察白色标记出现与消失。若拖动方向反了，结合 last_point 确认轴是否需要交换或镜像。屏上一直无标记时先核对 stage，再看 io_errors。

## 练习

**预测题**　未置 ready 的一次轮询会让白点消失吗？

<details><summary>核对预测</summary>

未发布新状态时保留原显示，新帧报告零触点才擦除。

</details>

**改动题**　把标记左右各 4 像素改为 6。

<details><summary>核对改法</summary>

中间区域变成 13×13，边缘仍裁剪。更新主机检查中的边界预期。

</details>

## 验证记录

窗口区域的新旧示例均已交叉链接。主机验证了四角映射、超界拒绝和矩形裁剪，屏幕和触摸实测待补。 [Goodix GT911 编程指南](https://www.orientdisplay.com/pdf/GT911.pdf)

运行独立主机检查。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/stm32/fsmc/ili9341-touch/check.c -o .work/ili9341-touch-check.exe
& ./.work/ili9341-touch-check.exe
```
