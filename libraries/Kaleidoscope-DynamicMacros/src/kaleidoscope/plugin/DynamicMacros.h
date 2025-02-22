/* DynamicMacros - Dynamic macro support for Kaleidoscope.
 * Copyright (C) 2019, 2021  Keyboard.io, Inc.
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

#pragma once

#include "kaleidoscope/Runtime.h"
#include <Kaleidoscope-EEPROM-Settings.h>
#include <Kaleidoscope-Ranges.h>

#include "kaleidoscope/plugin/Macros/MacroSteps.h"

#define DM(n) Key(kaleidoscope::ranges::DYNAMIC_MACRO_FIRST + n)

#define MAX_CONCURRENT_DYNAMIC_MACRO_KEYS 8

namespace kaleidoscope
{
  namespace plugin
  {

    class DynamicMacros : public kaleidoscope::Plugin
    {
    public:

      static void reserve_storage(uint16_t size);

      /// Play a macro sequence of key events
      void play(uint8_t seq_id);

      // ---------------------------------------------------------------------------
      // Event handlers
      EventHandlerResult onNameQuery();
      EventHandlerResult onKeyEvent(KeyEvent &event);
      EventHandlerResult beforeReportingState(const KeyEvent &event);
      EventHandlerResult onFocusEvent(const char *command);

      uint16_t minRInterval;
      uint16_t maxRInterval;

    private:
      static uint16_t storage_base_;
      static uint16_t storage_size_;
      static uint16_t map_[63];
      static void updateDynamicMacroCache();
      static Key active_macro_keys_[MAX_CONCURRENT_DYNAMIC_MACRO_KEYS];
      static void press(Key key);
      static void release(Key key);
      static void tap(Key key);
      void randomDelay(void);
    };

  } // namespace plugin
} // namespace kaleidoscope

extern kaleidoscope::plugin::DynamicMacros DynamicMacros;
