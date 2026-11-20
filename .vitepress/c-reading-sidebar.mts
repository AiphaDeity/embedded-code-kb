export const cReadingStages = [
  {
    "text": "导读 · 第一次读 C",
    "collapsed": true,
    "items": [
      {
        "text": "00　第一次顺着 main 读 C 程序",
        "link": "/docs/c-reading/00"
      }
    ]
  },
  {
    "text": "阶段 1 · 表达式与控制流",
    "collapsed": true,
    "items": [
      {
        "text": "01　整数换算 · 毫伏拆成伏和毫伏",
        "link": "/docs/c-reading/01"
      },
      {
        "text": "02　条件分支 · 温度分档",
        "link": "/docs/c-reading/02"
      },
      {
        "text": "03　for 循环 · 累加四次读数",
        "link": "/docs/c-reading/03"
      },
      {
        "text": "04　while 循环 · 拆出十进制各位",
        "link": "/docs/c-reading/04"
      },
      {
        "text": "05　函数 · 把 PWM 占空比限制在范围内",
        "link": "/docs/c-reading/05"
      },
      {
        "text": "06　switch 与枚举 · 把工作模式变成文本",
        "link": "/docs/c-reading/06"
      }
    ]
  },
  {
    "text": "阶段 2 · 数组、字符串与指针",
    "collapsed": true,
    "items": [
      {
        "text": "07　一维数组 · 查找第一次越限的位置",
        "link": "/docs/c-reading/07"
      },
      {
        "text": "07A　数组与指针对照 · 下标、地址和元素个数",
        "link": "/docs/c-reading/07A"
      },
      {
        "text": "08　二维数组 · 按通道统计采样值",
        "link": "/docs/c-reading/08"
      },
      {
        "text": "09　字符串 · 找到命令中的分隔符",
        "link": "/docs/c-reading/09"
      },
      {
        "text": "10　字符数组 · 识别格式化输出截断",
        "link": "/docs/c-reading/10"
      },
      {
        "text": "11　指针与输出参数 · 一次取到最小值和最大值",
        "link": "/docs/c-reading/11"
      },
      {
        "text": "12　指针与长度 · 递归折半查找有序表",
        "link": "/docs/c-reading/12"
      }
    ]
  },
  {
    "text": "阶段 3 · 位操作与二进制数据",
    "collapsed": true,
    "items": [
      {
        "text": "13　整型提升 · 两个字节相减",
        "link": "/docs/c-reading/13"
      },
      {
        "text": "14　位掩码 · 置位、清位和查询",
        "link": "/docs/c-reading/14"
      },
      {
        "text": "15　连续位域 · 更新配置字中的模式",
        "link": "/docs/c-reading/15"
      },
      {
        "text": "16　字节序 · 读取大端 16 位数",
        "link": "/docs/c-reading/16"
      },
      {
        "text": "17　有符号采样 · 解释 12 位补码",
        "link": "/docs/c-reading/17"
      },
      {
        "text": "18　校验 · 按位计算 CRC-8",
        "link": "/docs/c-reading/18"
      }
    ]
  },
  {
    "text": "阶段 4 · 对象、内存与模块接口",
    "collapsed": true,
    "items": [
      {
        "text": "19　结构体与生命周期 · 两路独立计数",
        "link": "/docs/c-reading/19"
      },
      {
        "text": "20　枚举与联合体 · 表示不同种类的事件",
        "link": "/docs/c-reading/20"
      },
      {
        "text": "21　静态存储期 · 跨调用保存计数",
        "link": "/docs/c-reading/21"
      },
      {
        "text": "22　堆内存 · 复制一批采样并交付所有权",
        "link": "/docs/c-reading/22"
      },
      {
        "text": "22A　调用者提供缓冲区 · 容量、写入长度与失败约定",
        "link": "/docs/c-reading/22A"
      },
      {
        "text": "23　函数指针 · 把数据交给回调",
        "link": "/docs/c-reading/23"
      },
      {
        "text": "24　多文件模块 · 头文件、实现与调用方",
        "link": "/docs/c-reading/24"
      }
    ]
  },
  {
    "text": "阶段 5 · 有状态的嵌入式模块",
    "collapsed": true,
    "items": [
      {
        "text": "25　计时器回绕 · 判断是否超时",
        "link": "/docs/c-reading/25"
      },
      {
        "text": "26　按键消抖 · 连续三次一致才切换",
        "link": "/docs/c-reading/26"
      },
      {
        "text": "27　滑动平均 · 固定窗口和运行和",
        "link": "/docs/c-reading/27"
      },
      {
        "text": "28　环形队列 · 回绕、判空与判满",
        "link": "/docs/c-reading/28"
      },
      {
        "text": "29　零复制解析 · 返回报文数据区视图",
        "link": "/docs/c-reading/29"
      },
      {
        "text": "30　逐字节状态机 · 拼接分批到达的帧",
        "link": "/docs/c-reading/30"
      }
    ]
  },
  {
    "text": "阶段 6 · 资源约束、协作与故障分析",
    "collapsed": true,
    "items": [
      {
        "text": "31　固定块内存池 · 耗尽与归还",
        "link": "/docs/c-reading/31"
      },
      {
        "text": "32　寄存器语义 · 写 1 清除事件标志",
        "link": "/docs/c-reading/32"
      },
      {
        "text": "33　C11 原子操作 · 单槽生产者与消费者",
        "link": "/docs/c-reading/33"
      },
      {
        "text": "34　双缓冲 · 观察生产速度超过消费速度",
        "link": "/docs/c-reading/34"
      },
      {
        "text": "35　故障定位 · 给队列添加高水位和丢弃计数",
        "link": "/docs/c-reading/35"
      },
      {
        "text": "36　综合 · 可注入故障的传感器轮询模块",
        "link": "/docs/c-reading/36"
      }
    ]
  },
  {
    "text": "阶段 7 · 从 C 源码读到编译产物",
    "collapsed": true,
    "items": [
      {
        "text": "37　宏与内联函数 · 参数何时求值",
        "link": "/docs/c-reading/37"
      },
      {
        "text": "38　对象布局 · 对齐、填充与线上格式",
        "link": "/docs/c-reading/38"
      },
      {
        "text": "39　有符号边界 · 先检查再加",
        "link": "/docs/c-reading/39"
      },
      {
        "text": "40　优化与调试 · 追踪一次循环",
        "link": "/docs/c-reading/40"
      },
      {
        "text": "41　存储期与链接 · 在 map 中找对象",
        "link": "/docs/c-reading/41"
      },
      {
        "text": "42　综合验收 · 分块解码与故障恢复",
        "link": "/docs/c-reading/42"
      }
    ]
  },
  {
    "text": "结课 · 从主机代码到 STM32",
    "collapsed": true,
    "items": [
      {
        "text": "43　结课项目 · STM32 串口中断接收与故障观察",
        "link": "/docs/c-reading/43"
      }
    ]
  }
]
