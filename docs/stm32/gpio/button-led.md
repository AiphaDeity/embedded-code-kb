# 天空星 GPIO：按住按键点亮 LED

同一组引脚也有一份 [CMSIS 寄存器版](register-button-led.md)，可对照 HAL 配置和实际寄存器写入。
输出值和线上的实际电平有差异时，可做[开漏释放与回读](open-drain-readback.md)实验。

在天空星核心板上按住用户键，板载 LED 应亮起，松手后熄灭。主循环每次读取 PA0 的输入电平，再用这次读到的值控制 PB2 的输出。

## 板型和引脚

参考板为立创·天空星 STM32F407VET6 核心板。实验使用核心板自带的用户键和受 PB2 控制的 LED，接上供电和 SWD 调试器即可。

| 信号 | 引脚 | 模式 | 高电平 | 低电平 |
|---|---|---|---|---|
| 用户按键 | PA0 | 输入，使用板载下拉 | 按下 | 松开 |
| 核心板 LED | PB2 | 推挽输出 | 点亮 | 熄灭 |

原理图中的 PA0 通过电阻下拉，按下时接到 3.3 V。认准用户键和 PB2 对应的 LED，可用板卡原理图与丝印核对。[立创独立按键原理](https://wiki.lckfb.com/zh-hans/tkx/tkx-stm32f407vxt6/beginner/key-principle.html) · [立创天空星 HAL 点灯教程](https://wiki.lckfb.com/zh-hans/fdb/basic/new-lighting-project-hal.html#_5-2-明确写出亮灭状态)

## 完整示例

<a href="../../../examples/stm32/gpio/button-led/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/gpio/button-led/main.c

## 顺着 main() 读

1. **初始化。** `HAL_Init()` 建立默认时基。进入 `gpio_init()` 后先给 GPIOA、GPIOB 打开时钟，再配置按键输入和 LED 输出。先写低电平再切换输出模式，用来设定灯的初态。
2. **读取。** `HAL_GPIO_ReadPin()` 采样 PA0。按住时得到 `GPIO_PIN_SET`，松开时得到 `GPIO_PIN_RESET`。找引脚时，端口和掩码要一起看。
3. **更新。** PA0 和 PB2 都是高电平有效。本例把读到的电平直接交给 `HAL_GPIO_WritePin()`，所以灯随按键电平变化。
4. **等待下一轮。** `HAL_Delay(1U)` 使用 HAL 毫秒时基。每次 SysTick 中断调用 `HAL_IncTick()`，延时结束后回到第 2 步。这段短延时用于轮询，按键可能在按下和松开时抖动。

## 构建并观察

准备 GNU Arm 编译器、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径改成本机实际位置，在仓库根目录运行：

```sh
cmake -S examples/stm32/gpio/button-led -B .work/sky-gpio -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-gpio
```

在用于实验的板上通过 SWD 下载 `.work/sky-gpio/sky-button-led.elf`，复位后按以下顺序观察。

| 动作 | 预期 |
|---|---|
| 上电后松开按键 | LED 熄灭 |
| 按住用户键 | LED 保持点亮 |
| 松手 | LED 熄灭 |

若现象不符，在调试器中从主循环看 `button` 是否随按下、松开变化。输入值正确时，再核对 `LED_PORT`、`LED_PIN` 和实际观察的灯。

## 练习

**预测题**　一直按住按键时，第 3 步每轮都执行。LED 会怎样变化？

<details><summary>核对预测</summary>

每轮都把高电平写给 PB2，LED 保持点亮。重复写同一个电平只会维持原来的状态。

</details>

**改动题**　改成「每按一次切换灯的状态」。按住两秒只切换一次，稳定松开后再按可以切换第二次。

<details><summary>核对改法</summary>

在 `gpio_init();` 后、原主循环的位置放入以下代码。候选电平变化时重新计时。等候选值稳定至少 20 ms，再更新确认值，并且只在确认按下的那一刻切换 LED。

```c
// 1. 从松开状态开始，读一次引脚作为待确认的电平。
GPIO_PinState stable = GPIO_PIN_RESET;
GPIO_PinState candidate = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
uint32_t changed_at = HAL_GetTick();
for (;;)
{
  // 2. 原始采样一旦变化，重新开始计算持续时间。
  const GPIO_PinState sample = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
  const uint32_t now = HAL_GetTick();
  if (sample != candidate)
  {
    candidate = sample;
    changed_at = now;
  }

  // 3. 候选值稳定达到 20 ms 后，确认新状态。只有确认按下时才切换灯。
  if (candidate != stable && now - changed_at >= 20U)
  {
    stable = candidate;
    // 松开只更新状态，供下一次按下判断。
    if (stable == GPIO_PIN_SET)
    {
      HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
  }
  // 4. 等待下一轮采样。这里的短延时不改变 20 ms 的稳定判定条件。
  HAL_Delay(1U);
}
```

</details>

## 验证记录

按键和 LED 极性已对照天空星官方资料。使用 STM32CubeF4 V1.28.3 与 GNU Arm 14.3.1 编译链接成功，生成 `sky-button-led.elf`。上电、按住和松开的可见现象待实板核对。
