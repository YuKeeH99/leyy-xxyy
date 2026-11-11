#ifndef __VN_PLAT_PIN_DEF_H__
#define __VN_PLAT_PIN_DEF_H__ 

// BATTERY
#define VN_ADC_CHANNEL_BATTERY_VOLTAGE         ADC_CHANNEL_ADC0
#define VN_ADC_CHANNEL_BATTERY_TEMP            ADC_CHANNEL_ADC1

// CHARGE
#define VN_PIN_CHARGE_CHRG                     GPIO_PIN_NO_14
#define VN_PIN_CHARGE_STDBY                    GPIO_PIN_NO_15
#define VN_PIN_CHARGE_EN                       GPIO_PIN_NO_1

// UART DEBUG
#define VN_UART_PORT_DEBUG                     QL_DEBUG_UART_PORT

// WIFI
#define VN_UART_PORT_WIFI                      QL_MAIN_UART_PORT
#define VN_PIN_WIFI_CHIP_EN                    GPIO_PIN_NO_27
#define VN_PIN_WIFI_LDO_EN                     GPIO_PIN_NO_20

// RADAR
#define VN_UART_PORT_RADAR                     QL_BT_UART_PORT
#define VN_PIN_RADAR_LDO_EN                    GPIO_PIN_NO_23
#define VN_PIN_RADAR_OUT                       GPIO_PIN_NO_22

// WAKEUP CHIP
#define VN_UART_PORT_WAKEUP_CHIP               QL_U4_UART_PORT
#define VN_PIN_WAKEUP_CHIP_LDO_EN              GPIO_PIN_NO_21
#define VN_PIN_WAKEUP_CHIP_INT                 GPIO_PIN_NO_26

// LCD
#define VN_SPI_PORT_LCD                        QL_SPI_PORT1
#define VN_PIN_LCD_DC                          GPIO_PIN_NO_35
#define VN_PIN_LCD_RST                         GPIO_PIN_NO_32
#define VN_PIN_LCD_TE                          GPIO_PIN_NO_8
#define VN_PIN_OLED_LCD_LDO_EN                 GPIO_PIN_NO_25

// OLED
#define VN_I2C_PORT_OLED                       2

// FLASH
#define VN_SPI_PORT_FLASH                      QL_SPI_PORT2
#define VN_PIN_FLASH_SPI_CS                    GPIO_PIN_NO_5

// TOUCH
#define VN_PIN_TOUCH                           GPIO_PIN_NO_31

// PA
#define VN_PIN_PA_EN                           GPIO_PIN_NO_124

// CODEC
#define VN_PIN_CODEC_LDO_EN                    GPIO_PIN_NO_24
#define VN_I2C_PORT_CODEC                      0

// STATUS
#define VN_PIN_STATUS                          GPIO_PIN_NO_70

// DETECT POWER ON 
#define VN_PIN_DETECT_POWER_ON                 GPIO_PIN_NO_0


#endif // __VN_PLAT_PIN_DEF_H__