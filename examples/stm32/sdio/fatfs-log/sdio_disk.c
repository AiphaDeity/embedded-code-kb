#include "stm32f4xx_hal.h"
#include "sdio_disk.h"
#include "ff.h"
#include "diskio.h"

#define WAIT_MS 1000U
#define BLOCK_BYTES 512U

static SD_HandleTypeDef card;
static DSTATUS drive_status = STA_NOINIT;
volatile uint32_t sd_error;

void HAL_SD_MspInit(SD_HandleTypeDef *handle)
{
  if (handle->Instance == SDIO)
  {
    // 1. PC8 是数据，PC12 是时钟，PD2 是命令。三个信号均使用 AF12。
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SDIO_CLK_ENABLE();
    GPIO_InitTypeDef pin = {
      .Pin = GPIO_PIN_8, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP,
      .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF12_SDIO,
    };
    HAL_GPIO_Init(GPIOC, &pin);
    pin.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &pin);
    pin.Pin = GPIO_PIN_12;
    pin.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &pin);
  }
}

static DRESULT wait_ready(void)
{
  const uint32_t start = HAL_GetTick();
  do
  {
    // 1. 确认卡已经结束内部忙写，之后才能交给下一次扇区访问。
    const HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&card);
    if (state == HAL_SD_CARD_TRANSFER)
    {
      return RES_OK;
    }
    if (state == HAL_SD_CARD_ERROR)
    {
      break;
    }
    HAL_Delay(1U);
  } while ((uint32_t)(HAL_GetTick() - start) < WAIT_MS);
  sd_error = HAL_SD_GetError(&card);
  return RES_ERROR;
}

DSTATUS disk_initialize(BYTE drive)
{
  if (drive != 0U)
  {
    return STA_NOINIT;
  }

  // 1. HAL 以识别时钟初始化卡，然后切到这里配置的 1 位、标称 12 MHz 传输。
  card.Instance = SDIO;
  card.Init = (SD_InitTypeDef){
    .ClockEdge = SDIO_CLOCK_EDGE_RISING, .ClockBypass = SDIO_CLOCK_BYPASS_DISABLE,
    .ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE, .BusWide = SDIO_BUS_WIDE_1B,
    .HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE, .ClockDiv = 2U,
  };
  if (HAL_SD_Init(&card) == HAL_OK && wait_ready() == RES_OK)
  {
    drive_status = 0U;
  }
  else
  {
    sd_error = HAL_SD_GetError(&card);
    drive_status = STA_NOINIT;
  }
  return drive_status;
}

DSTATUS disk_status(BYTE drive)
{
  return drive == 0U ? drive_status : STA_NOINIT;
}

DRESULT disk_read(BYTE drive, BYTE *buffer, DWORD sector, UINT count)
{
  if (drive != 0U || count == 0U)
  {
    return RES_PARERR;
  }
  if (drive_status != 0U)
  {
    return RES_NOTRDY;
  }

  // 1. FatFs 给出逻辑扇区号。HAL 按卡类型换算寻址，本轮等待不成功就向上传错。
  if (HAL_SD_ReadBlocks(&card, buffer, sector, count, WAIT_MS) != HAL_OK)
  {
    sd_error = HAL_SD_GetError(&card);
    return RES_ERROR;
  }
  return wait_ready();
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, DWORD sector, UINT count)
{
  if (drive != 0U || count == 0U)
  {
    return RES_PARERR;
  }
  if (drive_status != 0U)
  {
    return RES_NOTRDY;
  }

  // 1. HAL 的阻塞写接口用非 const 指针，实际只读取源缓冲。返回后仍要等卡编程完成。
  if (HAL_SD_WriteBlocks(&card, (uint8_t *)buffer, sector, count, WAIT_MS) != HAL_OK)
  {
    sd_error = HAL_SD_GetError(&card);
    return RES_ERROR;
  }
  return wait_ready();
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
  if (drive != 0U)
  {
    return RES_PARERR;
  }
  if (drive_status != 0U)
  {
    return RES_NOTRDY;
  }
  if (command == CTRL_SYNC)
  {
    return wait_ready();
  }
  if (command == GET_SECTOR_COUNT)
  {
    HAL_SD_CardInfoTypeDef info;
    if (HAL_SD_GetCardInfo(&card, &info) != HAL_OK)
    {
      return RES_ERROR;
    }
    *(DWORD *)buffer = info.LogBlockNbr;
  }
  else if (command == GET_SECTOR_SIZE)
  {
    *(WORD *)buffer = BLOCK_BYTES;
  }
  else
  {
    return RES_PARERR;
  }
  return RES_OK;
}
