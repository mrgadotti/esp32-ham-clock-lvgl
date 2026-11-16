#pragma once
// Pin map per board, selected by -DBOARD_CYD or -DBOARD_38PIN.
// TFT pins live in platformio.ini (TFT_eSPI build flags).

#define PIN_BL 21
#define PIN_TOUCH_CLK 25
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_CS 33

// Panel inversion at boot; flip if colors look negative.
#define DISPLAY_INVERT_DEFAULT true

#if defined(BOARD_CYD)
#define PIN_TOUCH_MISO 39
#define PIN_TOUCH_IRQ 36
#define PIN_UART2_RX 27
#define PIN_UART2_TX 22
#define HAS_RGB_LED 1
#define PIN_LED_R 4
#define PIN_LED_G 16
#define PIN_LED_B 17  // active LOW
#elif defined(BOARD_38PIN)
#define PIN_TOUCH_MISO 27
#define PIN_TOUCH_IRQ 35
#define PIN_UART2_RX 16
#define PIN_UART2_TX 17
#else
#error "Define BOARD_CYD or BOARD_38PIN"
#endif
