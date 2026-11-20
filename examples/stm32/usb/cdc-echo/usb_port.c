#include "usbd_core.h"

static PCD_HandleTypeDef controller;
enum { ENDPOINT_COUNT = 4 };

static USBD_StatusTypeDef usb_status(HAL_StatusTypeDef status)
{
  if (status == HAL_OK) { return USBD_OK; }
  if (status == HAL_BUSY) { return USBD_BUSY; }
  return USBD_FAIL;
}

void OTG_FS_IRQHandler(void) { HAL_PCD_IRQHandler(&controller); }

void HAL_PCD_MspInit(PCD_HandleTypeDef *handle)
{
  if (handle->Instance != USB_OTG_FS) { return; }
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_11 | GPIO_PIN_12, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Alternate = GPIO_AF10_OTG_FS,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  HAL_NVIC_SetPriority(OTG_FS_IRQn, 5U, 0U);
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *handle)
{
  if (handle->Instance != USB_OTG_FS) { return; }
  HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
  __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
}

// 1. PCD 回调把控制、数据和总线状态传给中间件，USB 类负责解释端点用途。
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *handle)
{
  USBD_LL_SetupStage(handle->pData, (uint8_t *)handle->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *handle, uint8_t ep)
{
  USBD_LL_DataOutStage(handle->pData, ep, handle->OUT_ep[ep].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *handle, uint8_t ep)
{
  USBD_LL_DataInStage(handle->pData, ep, handle->IN_ep[ep].xfer_buff);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *handle)
{
  USBD_LL_SetSpeed(handle->pData, USBD_SPEED_FULL);
  USBD_LL_Reset(handle->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *handle) { USBD_LL_Suspend(handle->pData); }
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *handle) { USBD_LL_Resume(handle->pData); }
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *handle) { USBD_LL_DevConnected(handle->pData); }
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *handle) { USBD_LL_DevDisconnected(handle->pData); }

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
  // 1. 设备和 PCD 互持上下文。核心板 Type-C 接口作总线供电的 FS 设备。
  controller.Instance = USB_OTG_FS;
  controller.Init = (PCD_InitTypeDef){
    .dev_endpoints = ENDPOINT_COUNT, .speed = PCD_SPEED_FULL, .ep0_mps = USB_MAX_EP0_SIZE,
    .phy_itface = USB_OTG_EMBEDDED_PHY, .vbus_sensing_enable = DISABLE,
  };
  controller.pData = pdev;
  pdev->pData = &controller;
  if (HAL_PCD_Init(&controller) != HAL_OK)
  {
    return USBD_FAIL;
  }

  // 2. FIFO 以 32 位字为单位。OUT 共用接收区，IN 给 EP0/EP1/EP2 分别留空间。
  if (HAL_PCDEx_SetRxFiFo(&controller, 128U) != HAL_OK ||
      HAL_PCDEx_SetTxFiFo(&controller, 0U, 64U) != HAL_OK ||
      HAL_PCDEx_SetTxFiFo(&controller, 1U, 64U) != HAL_OK ||
      HAL_PCDEx_SetTxFiFo(&controller, 2U, 32U) != HAL_OK)
  {
    return USBD_FAIL;
  }
  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  return usb_status(HAL_PCD_DeInit(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  return usb_status(HAL_PCD_Start(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  return usb_status(HAL_PCD_Stop(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t address, uint8_t type, uint16_t size)
{
  return usb_status(HAL_PCD_EP_Open(pdev->pData, address, size, type));
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return usb_status(HAL_PCD_EP_Close(pdev->pData, address));
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return usb_status(HAL_PCD_EP_Flush(pdev->pData, address));
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return usb_status(HAL_PCD_EP_SetStall(pdev->pData, address));
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return usb_status(HAL_PCD_EP_ClrStall(pdev->pData, address));
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return usb_status(HAL_PCD_SetAddress(pdev->pData, address));
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t address, uint8_t *data, uint32_t size)
{
  return usb_status(HAL_PCD_EP_Transmit(pdev->pData, address, data, size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t address, uint8_t *data, uint32_t size)
{
  return usb_status(HAL_PCD_EP_Receive(pdev->pData, address, data, size));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t address)
{
  const PCD_HandleTypeDef *handle = pdev->pData;
  const uint8_t index = address & 0x7FU;
  if (index >= ENDPOINT_COUNT) { return 1U; }
  return (address & 0x80U) != 0U ? handle->IN_ep[index].is_stall : handle->OUT_ep[index].is_stall;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t address)
{
  return HAL_PCD_EP_GetRxCount(pdev->pData, address);
}

void USBD_LL_Delay(uint32_t ms) { HAL_Delay(ms); }
