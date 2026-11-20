#include "usbd_ctlreq.h"

static uint8_t device[] = {
  18U, USB_DESC_TYPE_DEVICE, 0x00U, 0x02U,
  0x02U, 0x02U, 0x00U, USB_MAX_EP0_SIZE,
  LOBYTE(SKY_USB_VID), HIBYTE(SKY_USB_VID), LOBYTE(SKY_USB_PID), HIBYTE(SKY_USB_PID),
  0x00U, 0x01U, USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, 0U, 1U,
};
static uint8_t language[] = {4U, USB_DESC_TYPE_STRING, 0x09U, 0x04U};
static uint8_t text[USBD_MAX_STR_DESC_SIZ];
static uint8_t vendor_name[] = "Embedded Code KB";
static uint8_t product_name[] = "Sky CDC Echo";

static uint8_t *get_device(USBD_SpeedTypeDef speed, uint16_t *length)
{
  (void)speed;
  *length = sizeof(device);
  return device;
}

static uint8_t *get_language(USBD_SpeedTypeDef speed, uint16_t *length)
{
  (void)speed;
  *length = sizeof(language);
  return language;
}

static uint8_t *get_manufacturer(USBD_SpeedTypeDef speed, uint16_t *length)
{
  (void)speed;
  USBD_GetString(vendor_name, text, length);
  return text;
}

static uint8_t *get_product(USBD_SpeedTypeDef speed, uint16_t *length)
{
  (void)speed;
  USBD_GetString(product_name, text, length);
  return text;
}

USBD_DescriptorsTypeDef sky_usb_descriptors = {
  .GetDeviceDescriptor = get_device, .GetLangIDStrDescriptor = get_language,
  .GetManufacturerStrDescriptor = get_manufacturer, .GetProductStrDescriptor = get_product,
};
