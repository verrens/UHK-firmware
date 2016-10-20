#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "test_led.h"
#include "key_matrix.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbKeyboardEndpoints[USB_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_KEYBOARD_ENDPOINT_COUNT, UsbKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbKeyboardInterfaces[USB_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_KEYBOARD_INTERFACE_INDEX,
    UsbKeyboardInterface,
    sizeof(UsbKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_KEYBOARD_INTERFACE_COUNT,
    UsbKeyboardInterfaces,
}};

usb_device_class_struct_t UsbKeyboardClass = {
    UsbKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static usb_keyboard_report_t UsbKeyboardReport;

#define KEYBOARD_MATRIX_COLS_NUM 7
#define KEYBOARD_MATRIX_ROWS_NUM 5

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
    .cols = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTB, GPIOB, kCLOCK_PortB, 3},
        {PORTB, GPIOB, kCLOCK_PortB, 16},
        {PORTB, GPIOB, kCLOCK_PortB, 17},
        {PORTB, GPIOB, kCLOCK_PortB, 18},
        {PORTA, GPIOA, kCLOCK_PortA, 1},
        {PORTB, GPIOB, kCLOCK_PortB, 0}
        },
    .rows = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 12},
        {PORTA, GPIOA, kCLOCK_PortA, 13},
        {PORTC, GPIOC, kCLOCK_PortC, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 19},
        {PORTD, GPIOD, kCLOCK_PortD, 6}
    }
};

#define KEY_STATE_COUNT (5*7)
uint8_t leftKeyStates[KEY_STATE_COUNT] = {0};

static usb_status_t UsbKeyboardAction(void)
{
    UsbKeyboardReport.modifiers = 0;
    UsbKeyboardReport.reserved = 0;

    KeyMatrix_Init(&keyMatrix);
    KeyMatrix_Scan(&keyMatrix);

    uint8_t scancodeIdx;
    for (uint8_t scancodeIdx=0; scancodeIdx<USB_KEYBOARD_MAX_KEYS; scancodeIdx++) {
        UsbKeyboardReport.scancodes[scancodeIdx] = 0;
    }

    uint8_t data[] = {0};
    I2cWrite(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, data, sizeof(data));

    I2cRead(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, leftKeyStates, KEY_STATE_COUNT);

    scancodeIdx = 0;
    for (uint8_t keyId=0; keyId<KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM; keyId++) {
        if (keyMatrix.keyStates[keyId] && scancodeIdx<USB_KEYBOARD_MAX_KEYS) {
            UsbKeyboardReport.scancodes[scancodeIdx++] = keyId + HID_KEYBOARD_SC_A;
        }
    }

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (leftKeyStates[keyId] && scancodeIdx<USB_KEYBOARD_MAX_KEYS) {
            UsbKeyboardReport.scancodes[scancodeIdx++] = keyId + HID_KEYBOARD_SC_A;
        }
    }

    return USB_DeviceHidSend(UsbCompositeDevice.keyboardHandle, USB_KEYBOARD_ENDPOINT_INDEX,
                             (uint8_t*)&UsbKeyboardReport, USB_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbKeyboardAction();
            }
            break;
        case kUSB_DeviceHidEventGetReport:
        case kUSB_DeviceHidEventSetReport:
        case kUSB_DeviceHidEventRequestReportBuffer:
            error = kStatus_USB_InvalidRequest;
            break;
        case kUSB_DeviceHidEventGetIdle:
        case kUSB_DeviceHidEventGetProtocol:
        case kUSB_DeviceHidEventSetIdle:
        case kUSB_DeviceHidEventSetProtocol:
            break;
        default:
            break;
    }

    return error;
}

usb_status_t UsbKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}