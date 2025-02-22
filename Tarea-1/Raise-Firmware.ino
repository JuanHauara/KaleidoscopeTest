// -*- mode: c++ -*-
/* Raise-Firmware -- Factory firmware for the Dygma Raise
 * Copyright (C) 2019, 2020  DygmaLab, SE.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUILD_INFORMATION
#define BUILD_INFORMATION "locally built"
#endif

#include "Kaleidoscope.h"

#include "Kaleidoscope-MouseKeys.h"
#include "Kaleidoscope-LEDControl.h"
#include "Kaleidoscope-PersistentLEDMode.h"
#include "Kaleidoscope-FocusSerial.h"
#include "Kaleidoscope-EEPROM-Settings.h"
#include "Kaleidoscope-EEPROM-Keymap.h"
#include "EEPROMPadding.h"
#include "EEPROMUpgrade.h"
#include "Kaleidoscope-IdleLEDs.h"
#include "Kaleidoscope-Colormap.h"
#include "Kaleidoscope-LED-Palette-Theme.h"
#include "Kaleidoscope-LEDEffect-Rainbow.h"
#include "Kaleidoscope-LED-Stalker.h"
#include "Kaleidoscope-LED-Wavepool.h"
#include "Kaleidoscope-Heatmap.h"
#include "Kaleidoscope-LEDEffect-DigitalRain.h"
#include "Kaleidoscope-LEDEffect-BootGreeting.h"
#include "Kaleidoscope-DynamicSuperKeys.h"
// Support for host power management (suspend & wakeup)
#include "Kaleidoscope-HostPowerManagement.h"
#include "Kaleidoscope-DynamicMacros.h"
#include "Kaleidoscope-MagicCombo.h"
#include "Kaleidoscope-USB-Quirks.h"
#include "Kaleidoscope-LayerFocus.h"
#include "RaiseIdleLEDs.h"
#include "RaiseFirmwareVersion.h"
#include "kaleidoscope/device/dygma/raise/Focus.h"
#include "kaleidoscope/device/dygma/raise/SideFlash.h"
#include "Kaleidoscope-OneShot.h"
#include "Kaleidoscope-Qukeys.h"
#include "Kaleidoscope-Escape-OneShot.h"
#include "LED-CapsLockLight.h"
#include "attiny_firmware.h"

enum { QWERTY, NUMPAD, _LAYER_MAX }; // layers

/* This comment temporarily turns off astyle's indent enforcement so we can make
 * the keymaps actually resemble the physical key layout better
 */
// *INDENT-OFF*

KEYMAPS(
[QWERTY] = KEYMAP_STACKED
(
    Key_Escape      ,Key_1         ,Key_2       ,Key_3         ,Key_4     ,Key_5 ,Key_6
   ,Key_Tab         ,Key_Q         ,Key_W       ,Key_E         ,Key_R     ,Key_T
   ,Key_CapsLock    ,Key_A         ,Key_S       ,Key_D         ,Key_F     ,Key_G
   ,Key_LeftShift   ,Key_Backslash ,Key_Z       ,Key_X         ,Key_C     ,Key_V ,Key_B
   ,Key_LeftControl ,Key_LeftGui   ,Key_LeftAlt ,Key_Space     ,Key_Space
                                                ,Key_Backspace ,Key_Enter

   ,Key_7               ,Key_8      ,Key_9        ,Key_0        ,Key_Minus         ,Key_Equals       ,Key_Backspace
   ,Key_Y               ,Key_U      ,Key_I        ,Key_O        ,Key_P             ,Key_LeftBracket  ,Key_RightBracket ,Key_Enter
   ,Key_H               ,Key_J      ,Key_K        ,Key_L        ,Key_Semicolon     ,Key_Quote        ,Key_Backslash
   ,Key_N               ,Key_M      ,Key_Comma    ,Key_Period   ,Key_Slash         ,Key_RightShift
   ,Key_Space           ,Key_Space  ,Key_RightAlt ,Key_RightGui ,Key_LEDEffectNext ,Key_RightControl
   ,MoveToLayer(NUMPAD) ,Key_Delete
),

[NUMPAD] = KEYMAP_STACKED
(
    Key_Escape      ,Key_F1        ,Key_F2        ,Key_F3         ,Key_F4 ,Key_F5 ,Key_F6
   ,Key_Tab         ,XXX           ,Key_UpArrow   ,XXX            ,XXX    ,XXX
   ,Key_CapsLock    ,Key_LeftArrow ,Key_DownArrow ,Key_RightArrow ,XXX    ,XXX
   ,Key_LeftShift   ,Key_Backslash ,XXX           ,XXX            ,XXX    ,XXX    ,XXX
   ,Key_LeftControl ,Key_LeftGui   ,Key_LeftAlt   ,Key_Space      ,Key_Space
                                                  ,Key_Backspace  ,Key_Enter

   ,Key_F7              ,Key_F8    ,Key_F9        ,Key_F10       ,Key_F11            ,Key_F12 ,Key_Backspace
   ,Key_KeypadSubtract  ,Key_7     ,Key_8         ,Key_9         ,Key_KeypadDivide   ,XXX     ,XXX, Key_Enter
   ,Key_KeypadAdd       ,Key_4     ,Key_5         ,Key_6         ,Key_KeypadMultiply ,XXX     ,Key_Backslash
   ,Key_KeypadDot       ,Key_1     ,Key_2         ,Key_3         ,Key_UpArrow        ,Key_RightShift
   ,Key_0               ,Key_Space ,Key_LeftArrow ,Key_DownArrow ,Key_RightArrow     ,Key_RightControl
   ,MoveToLayer(QWERTY) ,Key_Delete
 )
);

/* Re-enable astyle's indent enforcement */
// *INDENT-ON*

kaleidoscope::device::keyboardio::raise::SideFlash<ATTinyFirmware> SideFlash;

enum {
  COMBO_TOGGLE_NKRO_MODE
};

static uint32_t protocol_toggle_start = 0;

static void toggleKeyboardProtocol(uint8_t combo_index)
{
  USBQuirks.toggleKeyboardProtocol();
  protocol_toggle_start = Kaleidoscope.millisAtCycleStart();
}

static void protocolBreathe()
{
  if (Kaleidoscope.hasTimeExpired(protocol_toggle_start, uint16_t(10000)))
  {
    protocol_toggle_start = 0;
  }
  if (protocol_toggle_start == 0)
    return;

  uint8_t hue = 120;
  if (Kaleidoscope.hid().keyboard().getProtocol() == HID_BOOT_PROTOCOL)
  {
    hue = 0;
  }

  cRGB color = breath_compute(hue);
  ::LEDControl.setCrgbAt(KeyAddr(4, 0), color);
  ::LEDControl.setCrgbAt(KeyAddr(3, 0), color);
  ::LEDControl.setCrgbAt(KeyAddr(4, 2), color);
  ::LEDControl.setCrgbAt(KeyAddr(0, 6), color);
  ::LEDControl.syncLeds();
}

USE_MAGIC_COMBOS(
    {.action = toggleKeyboardProtocol,
     // Left Ctrl + Left Shift + Left Alt + 6
     .keys = {R4C0, R3C0, R4C2, R0C6}});

kaleidoscope::plugin::EEPROMPadding JointPadding(8);

cRGB heat_colors[] = {
  {  0,   0,   0}, // black
  {255,  25,  25}, // blue
  { 25, 255,  25}, // green
  { 25,  25, 255}  // red
};

void toggleLedsOnSuspendResume(kaleidoscope::plugin::HostPowerManagement::Event event) {
  switch (event) {
  case kaleidoscope::plugin::HostPowerManagement::Suspend:
    LEDControl.disable();
    break;
  case kaleidoscope::plugin::HostPowerManagement::Resume:
    LEDControl.enable();
    break;
  case kaleidoscope::plugin::HostPowerManagement::Sleep:
    break;
  }
}

/** hostPowerManagementEventHandler dispatches power management events (suspend,
 * resume, and sleep) to other functions that perform action based on these
 * events.
 */
void hostPowerManagementEventHandler(kaleidoscope::plugin::HostPowerManagement::Event event) {
  toggleLedsOnSuspendResume(event);
}

KALEIDOSCOPE_INIT_PLUGINS(
    FirmwareVersion,
    USBQuirks,
    MagicCombo,
    RaiseIdleLEDs,
    EEPROMSettings,
    EEPROMKeymap,
    FocusSettingsCommand,
    FocusEEPROMCommand,
    LEDCapsLockLight,
    LEDControl,
    PersistentLEDMode,
    FocusLEDCommand,
    LEDPaletteTheme,
    JointPadding,
    ColormapEffect,
    LEDRainbowWaveEffect, LEDRainbowEffect, StalkerEffect, HeatmapEffect, LEDDigitalRainEffect, WavepoolEffect,
    BootGreetingEffect,
    PersistentIdleLEDs,
    RaiseFocus,
    Qukeys,
    // DynamicSuperKeys,
    DynamicMacros,
    SideFlash,
    Focus,
    MouseKeys,
    OneShot,
    EscapeOneShot,
    LayerFocus,
    EEPROMUpgrade,
    HostPowerManagement
  );

///////////////////////////////////////////////////////////////////////////
#include "wiring_private.h"	// Necesaria para función pinPeripheral()

/*
 * Configuración módulo SERCOM0 para comunicación USART en pines PA04->UART RX y PA05->UART TX.
 * 
 * Arduino Zero utiliza el módulo SERCOM0 para la instancia Serial1, esta es creada en el archivo 
 * "../hardware/dygma/samd/variants/arduino_zero/variant.cpp". Comento esa definición ya que no 
 * usaremos Serial1, en su lugar configuraremos el módulo SERCOM0 para trabajar con la instancia 
 * uartDebug y los pines 9 y 10 del chip.
 * Dado que Arduino Zero utiliza interrupciones para manejar el puerto UART, también comento la 
 * definición de la función "void SERCOM0_Handler()" en el mismo archivo variant.cpp ya que la 
 * utilizaremos para llamar al manejador IrqHandler() de nuestra instancia uartDebug.
 * 
 * ATmel SAMD21G18A-AUT, package TQFP48 -> Arduino Zero
 * Chip Pin 9  -> PA04 -> SERCOM 0 -> Arduino Zero pin A3/D17: UART RX
 * Chip Pin 10 -> PA05 -> SERCOM 0 -> Arduino Zero pin A4/D18: UART TX
 * Datasheet, pag. 29 y pag. 432
 * 
 * Constructor clase Uart ():
 * Uart(SERCOM *_s, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX);
 */
Uart uartDebug(&sercom0, A3, A4, SERCOM_RX_PAD_1, UART_TX_PAD_0);
//Uart uartDebug(&sercom0, 17, 18, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void SERCOM0_Handler() {
  uartDebug.IrqHandler();
}
///////////////////////////////////////////////////////////////////////////

void setup()
{
  // First start the serial communications to avoid restarting unnecesarily
  Kaleidoscope.serialPort().begin(9600);

  // Configuración módulo SERCOM0 para comunicación USART en pines PA04->UART RX y PA05->UART TX. 
  uartDebug.begin(9600);			// Comunicación a 9600 baudios / 8bits / No Parity / 1 Stop bit.
  pinPeripheral(A3, PIO_SERCOM);	// Asigna el pin A3 al módulo SERCOM.
  pinPeripheral(A4, PIO_SERCOM);	// Asigna el pin A4 al módulo SERCOM.
  uartDebug.println("Hola desde puerto uartDebug");

  Kaleidoscope.setup();

  // Reserve space in the keyboard's EEPROM for the keymaps
  EEPROMKeymap.setup(10);

  // Reserve space for the number of Colormap layers we will use
  ColormapEffect.max_layers(10);
  LEDRainbowEffect.brightness(255);
  LEDRainbowWaveEffect.brightness(255);
  StalkerEffect.variant = STALKER(BlazingTrail);
  HeatmapEffect.heat_colors = heat_colors;
  HeatmapEffect.heat_colors_length = 4;
  LEDDigitalRainEffect.DROP_MS = 100; // Make the rain fall faster
  LEDDigitalRainEffect.activate();
  WavepoolEffect.activate();

  // DynamicSuperKeys.setup(0, 1024);
  DynamicMacros.reserve_storage(2048);

  EEPROMUpgrade.reserveStorage();
  EEPROMUpgrade.upgrade();
}
 
void loop()
{
  Kaleidoscope.loop();
  protocolBreathe();
}
