# 把 ILI9341 显示和 GT911 触摸接入 LVGL 9.6

天空星显示一个点击计数按钮。触摸抬起形成一次点击，标签由 Count: 0 更新为 Count: 1，LVGL 只提交需要重绘的区域。

## 板型与接线

使用 [ILI9341 并口与 GT911](ili9341-touch.md) 的同一组接线。先确认外设能独立点亮、读坐标，再加载这一版。

## 沿执行路径读代码

1. `main()` 初始化 LCD 和 GT911，再把 HAL 毫秒计数交给 LVGL。每圈调用 `lv_timer_handler()`，输入读取、事件和绘制均在主循环上下文执行。
2. `read_pointer()` 用新帧更新坐标与按下状态，未收到新帧时保留上一状态。点击事件更新标签。
3. `flush_region()` 接收 RGB565 区域，设置 LCD 窗口后同步写 FSMC。最晚在退出回调前调用 `lv_display_flush_ready()`，让 LVGL 可以复用这段绘图内存。

## 完整源码

<a href="../../../examples/stm32/fsmc/lvgl-touch/main.c" download="main.c">下载示例 main.c</a> · <a href="../../../examples/stm32/fsmc/lvgl-touch/lv_conf.h" download="lv_conf.h">下载 LVGL 配置</a>

<<< ../../../examples/stm32/fsmc/lvgl-touch/main.c

驱动与[裸机触摸条目](ili9341-touch.md)共用。12 行部分缓冲占 5,760 字节，LVGL 堆的配置为 40 KiB。

## 编译和观察

准备 GNU Arm、CMake、Ninja、STM32CubeF4 V1.28.3 和 LVGL 9.6 源码。将两个依赖路径换成本机路径后构建。

```sh
cmake -S examples/stm32/fsmc/lvgl-touch -B .work/sky-lvgl-touch -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3" -DLVGL_ROOT="D:/SDK/lvgl"
cmake --build .work/sky-lvgl-touch
```

下载 `.work/sky-lvgl-touch/sky-lvgl-touch.elf` 后复位，点击按钮应增加计数。调试器可观察 `stage`、`presses`、`flushes` 和 `input_errors`。正常操作时 `bad_windows` 应为零。

## 练习

**预测题**　为什么更新标签后，刷屏函数并不一定提交全屏？

<details><summary>核对预测</summary>

标签更新会使相关区域失效。部分缓冲模式只绘制需要更新的区域，并按可用缓冲分块交付。

</details>

**改动题**　把部分缓冲从 12 行改为 6 行，比较同一画面首次显示的 flushes。

<details><summary>核对改法</summary>

缓冲减为 2,880 字节。需要显示大区域时通常会分成更多批次，显示内容保持一致。

</details>

## 验证记录

已用实际 LVGL 9.6.0 源码和 GNU Arm 14.3.1 交叉编译链接。LCD 输出和点击事件待天空星实板核对。
