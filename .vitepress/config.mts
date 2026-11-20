import { cp } from 'node:fs/promises'
import { join } from 'node:path'
import { gzipSync } from 'node:zlib'
import { defineConfig } from 'vitepress'
import { checkSourceDownloads } from './check-source-downloads.mts'
import { cReadingStages } from './c-reading-sidebar.mts'

export default defineConfig({
  lang: 'zh-CN',
  title: '嵌入式代码模板库',
  description: '按运行环境和功能查找嵌入式代码示例，沿源码执行路径阅读与练习。',
  base: '/embedded-code-kb/',
  srcExclude: ['templates/**', 'agent.md', '实施计划.md', '.work/**'],
  rewrites: { 'README.md': 'index.md' },
  vite: {
    plugins: [{
      name: 'gzip-local-search-index',
      apply: 'build',
      enforce: 'post',
      transform(code, id) {
        // VitePress 1.6 生成的根语言模块默认导出 MiniSearch 的 JSON 字符串。
        if (id !== '/@localSearchIndexroot') return
        const serialized = /^export default (".*")\s*;?\s*$/s.exec(code)?.[1]
        if (!serialized) return
        const compressed = gzipSync(JSON.parse(serialized), { level: 9 }).toString('base64')
        return {
          code: `import { gunzipSync, strFromU8 } from 'fflate'
const compressed = Uint8Array.from(atob(${JSON.stringify(compressed)}), character => character.charCodeAt(0))
export default strFromU8(gunzipSync(compressed))
`,
          map: null,
        }
      },
    }],
  },
  themeConfig: {
    nav: [
      { text: '首页', link: '/' },
      { text: '总目录', link: '/嵌入式代码模板库-总目录' },
      { text: 'C 代码跟读', link: '/docs/c-reading/' },
    ],
    sidebar: [
      {
        text: '嵌入式 C 代码跟读',
        link: '/docs/c-reading/',
        collapsed: true,
        items: cReadingStages,
      },
      {
        text: '通用 C / C++',
        collapsed: true,
        items: [
          { text: '从字节数组读取整数', link: '/docs/c/byte-array-parsing' },
          { text: 'C 位域掩码读取与改写', link: '/docs/c/bit-field' },
          { text: 'C 有符号定点换算与限幅', link: '/docs/c/fixed-point-scale' },
          { text: '环形缓冲区', link: '/docs/c/ring-buffer' },
          { text: '字节流解析', link: '/docs/c/stream-frame' },
          { text: 'Modbus RTU CRC-16', link: '/docs/c/crc16-modbus' },
          { text: '计数回绕与超时重试', link: '/docs/c/timed-retry' },
          { text: '按键抖动与稳定时间', link: '/docs/c/debounce' },
          { text: '滑动平均与滞回报警', link: '/docs/c/sample-filter' },
          { text: 'C 配置结构体与状态变化回调', link: '/docs/c/health-callback' },
          { text: 'C++ 作用域管理 SPI 片选', link: '/docs/c/spi-chip-select-cpp' },
        ],
      },
      {
        text: 'MCU 裸机 · STM32',
        collapsed: true,
        items: [
          { text: 'STM32 从复位走到最小主循环', link: '/docs/stm32/startup/minimal' },
          { text: '01 GPIO · 天空星按键点灯', link: '/docs/stm32/gpio/button-led' },
          { text: '01 GPIO · 寄存器版按键点灯', link: '/docs/stm32/gpio/register-button-led' },
          { text: '01 GPIO · 开漏释放与实际电平回读', link: '/docs/stm32/gpio/open-drain-readback' },
          { text: '01 GPIO · 非阻塞按键消抖', link: '/docs/stm32/gpio/debounced-button' },
          { text: '02 RCC · GPIO 时钟与 AHB 分频', link: '/docs/stm32/rcc/clock-gates' },
          { text: '02 RCC · HSE、PLL 与 MCO 测频', link: '/docs/stm32/rcc/hse-pll-mco' },
          { text: '03 UART · USART1 轮询收发', link: '/docs/stm32/uart/polling-echo' },
          { text: '03 USART1 · 寄存器版回显', link: '/docs/stm32/uart/register-echo' },
          { text: '03 USART1 中断收发与回显', link: '/docs/stm32/uart/interrupt-echo' },
          { text: '03 USART1 中断与环形缓冲', link: '/docs/stm32/uart/ring-echo' },
          { text: '天空星 UART DMA 接收', link: '/docs/stm32/uart-dma-rx-idle' },
          { text: '04 NVIC / EXTI 按键边沿计数', link: '/docs/stm32/exti/button-irq' },
          { text: '04 EXTI9_5 共享中断分派', link: '/docs/stm32/exti/shared-lines' },
          { text: '05 DMA2 搬运内存并比对', link: '/docs/stm32/dma/memory-copy' },
          { text: '05 DMA 半传输与缓冲区交接', link: '/docs/stm32/dma/half-transfer' },
          { text: '06 ADC1 轮询采样电位器', link: '/docs/stm32/adc/single-channel' },
          { text: '06 TIM2 触发 ADC1 与循环 DMA', link: '/docs/stm32/adc/timer-dma' },
          { text: '06 ADC1 双通道扫描', link: '/docs/stm32/adc/scan-two' },
          { text: '07 I²C 读取 MPU6050 器件 ID', link: '/docs/stm32/i2c/mpu6050-id' },
          { text: '07 MPU6050 连续读取三轴加速度', link: '/docs/stm32/i2c/mpu6050-acceleration' },
          { text: '08 SPI 读取 W25Q128 JEDEC ID', link: '/docs/stm32/spi/jedec-id' },
          { text: '08 SPI 双 DMA 连续读取', link: '/docs/stm32/spi/dma-read' },
          { text: '08 SPI NOR 双槽参数存储', link: '/docs/stm32/spi/nor-settings' },
          { text: '09 TIM2 定时中断翻转 LED', link: '/docs/stm32/tim/periodic-led' },
          { text: '09 TIM2 · 寄存器版定时中断', link: '/docs/stm32/tim/register-periodic-led' },
          { text: '09 PWM 输出与输入捕获测频', link: '/docs/stm32/tim/pwm-capture' },
          { text: '09 TIM3 读取两相编码器', link: '/docs/stm32/tim/encoder' },
          { text: '10 FSMC 给 ILI9341 并口屏写入像素', link: '/docs/stm32/fsmc/ili9341-pixel' },
          { text: '10 FSMC 复用地址并读写外部 SRAM', link: '/docs/stm32/fsmc/external-sram' },
          { text: '10 GT911 触摸与 LCD 区域刷新', link: '/docs/stm32/fsmc/ili9341-touch' },
          { text: '10 LVGL 9.6 触摸点击计数', link: '/docs/stm32/fsmc/lvgl-touch' },
          { text: '11 DAC1 循环输出三档电压', link: '/docs/stm32/dac/voltage-steps' },
          { text: '11 TIM6 与 DMA 输出查表波形', link: '/docs/stm32/dac/dma-wave' },
          { text: '12 IWDG 停喂后读取复位原因', link: '/docs/stm32/iwdg/reset-cause' },
          { text: '12 Monitor 汇总进展再喂狗', link: '/docs/stm32/iwdg/health-window' },
          { text: '13a RS-485 半双工两节点回显', link: '/docs/stm32/rs485/half-duplex-echo' },
          { text: '13a Modbus RTU 请求与应答', link: '/docs/stm32/rs485/modbus-rtu' },
          { text: '13b CAN1 内部回环收发一帧', link: '/docs/stm32/can/internal-loopback' },
          { text: '13b CAN 双节点请求应答', link: '/docs/stm32/can/two-node' },
          { text: 'Boot 校验镜像与 App 启动交接', link: '/docs/stm32/boot-app/verify-and-jump' },
          { text: 'Boot A/B 试跑、确认与回滚', link: '/docs/stm32/boot-app/trial-rollback' },
          { text: 'HardFault 快照与 PC 定位', link: '/docs/stm32/fault/hardfault-snapshot' },
          { text: 'RTC 闹钟从 STOP 唤醒', link: '/docs/stm32/rtc/stop-alarm' },
          { text: '片内 Flash 参数恢复', link: '/docs/stm32/flash/settings-journal' },
          { text: 'SDIO 与 FatFs 追加日志', link: '/docs/stm32/sdio/fatfs-log' },
          { text: 'USB CDC 数据回显', link: '/docs/stm32/usb/cdc-echo' },
          { text: '以太网 LAN8720A 与 UDP 回显', link: '/docs/stm32/ethernet/udp-echo' },
          { text: 'ESP-AT 串口查询 Wi-Fi 状态', link: '/docs/stm32/wireless/esp-at-status' },
        ],
      },
      {
        text: 'MCU + FreeRTOS',
        collapsed: true,
        items: [
          { text: '队列交接串口数据', link: '/docs/freertos/queue-message' },
          { text: 'FreeRTOS 串口任务解帧与丢字节恢复', link: '/docs/freertos/uart-parser' },
          { text: 'FreeRTOS 从 EXTI 中断直接通知任务', link: '/docs/freertos/exti-notify' },
          { text: 'FreeRTOS ADC 采样与串口日志', link: '/docs/freertos/adc-logger' },
          { text: 'FreeRTOS 任务通知与周期采样', link: '/docs/freertos/task-notification' },
          { text: 'FreeRTOS 互斥锁保护共享状态', link: '/docs/freertos/mutex-state' },
          { text: 'FreeRTOS 互斥锁保护共享 SPI 总线', link: '/docs/freertos/spi-mutex' },
          { text: 'FreeRTOS 软件定时器交接周期事件', link: '/docs/freertos/software-timer' },
          { text: 'FreeRTOS 用总期限等待命令应答', link: '/docs/freertos/command-deadline' },
          { text: 'FreeRTOS 事件组等待就绪', link: '/docs/freertos/event-group-ready' },
          { text: 'FreeRTOS 静态队列与任务栈余量', link: '/docs/freertos/static-queue' },
          { text: 'FreeRTOS 用 TIM2 记录任务运行时间', link: '/docs/freertos/run-time-stats' },
          { text: 'FreeRTOS 指针队列与静态缓冲池', link: '/docs/freertos/buffer-pool' },
          { text: 'FreeRTOS 任务进展与看门狗', link: '/docs/freertos/watchdog-monitor' },
        ],
      },
      {
        text: '嵌入式 Linux',
        collapsed: true,
        items: [
          { text: 'Linux 串口限时读取', link: '/docs/linux/serial-timeout' },
          { text: 'Linux poll 监听管道与 Unix 域套接字', link: '/docs/linux/poll-ipc' },
          { text: 'Linux I2C_RDWR 读取 MPU6050 温度', link: '/docs/linux/i2c-mpu6050' },
          { text: 'Linux 设备树匹配和只读 platform 驱动', link: '/docs/linux/platform-reading' },
          { text: '交叉构建、部署和日志', link: '/docs/linux/deploy' },
          { text: 'Linux 断线重试与 SQLite 缓存补传', link: '/docs/linux/tcp-outbox' },
          { text: 'Linux MQTT 上报与 PUBACK', link: '/docs/linux/mqtt-publish' },
          { text: 'Buildroot 与 Yocto 集成串口工具', link: '/docs/linux/distro-package' },
        ],
      },
      {
        text: '项目与排障',
        collapsed: true,
        items: [
          { text: '从天空星 ADC 到 JSON', link: '/docs/projects/sensor-to-json' },
          { text: 'Modbus–MQTT 网关阅读路线', link: '/docs/projects/modbus-mqtt-reading' },
          { text: 'CAN 收帧到 SD 卡日志', link: '/docs/projects/can-to-sd' },
          { text: 'SHT30 温湿度采集与 LCD 显示', link: '/docs/projects/sht30-display' },
          { text: '调试和排障索引', link: '/docs/guides/troubleshooting' },
        ],
      },
    ],
    outline: { label: '本页目录' },
    sidebarMenuLabel: '目录',
    returnToTopLabel: '回到顶部',
    docFooter: { prev: '上一篇', next: '下一篇' },
    search: {
      provider: 'local',
      options: {
        translations: {
          button: { buttonText: '搜索', buttonAriaLabel: '搜索知识库' },
          modal: {
            noResultsText: '没有找到相关条目',
            resetButtonTitle: '清空搜索',
            footer: { selectText: '选择', navigateText: '切换', closeText: '关闭' },
          },
        },
        miniSearch: {
          options: {
            tokenize(text) {
              const words = new Intl.Segmenter('zh-CN', { granularity: 'word' }).segment(text)
              return Array.from(words).filter(word => word.isWordLike).map(word => word.segment)
            },
          },
        },
      },
    },
  },
  async buildEnd(site) {
    // 保留与仓库相同的示例路径，使正文中的相对源码链接也能在静态站点打开。
    await cp(join(site.srcDir, 'examples'), join(site.outDir, 'examples'), { recursive: true })
    await checkSourceDownloads(site.srcDir, site.outDir, site.site.base)
  },
})
