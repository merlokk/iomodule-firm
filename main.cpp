/*
 * Copyright (c) 2017 Dmytro Shestakov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <cstdlib>

#include "ch_extended.h"
#include "hal.h"
#include "pinlist.h"
#include "shell_impl.h"
#include "analogout.h"
#include "analogin.h"
#include "digitalin.h"
#include "digitalout.h"
#include "modbus_impl.h"
#include "at24_impl.h"

using namespace Rtos;
using namespace Mcudrv;

static constexpr auto& dout = Digital::output;
static constexpr auto& aout = Analog::output;
static constexpr auto& ain = Analog::input;
static constexpr auto& din = Digital::input;

static auto Init = [](auto&&... objs) {
  (objs.Init(), ...);
};

std::atomic_uint32_t uptimeCounter;

using namespace std::literals;
using nvram::eeprom;

const uint8_t test_string[] = "123";
static std::array<uint8_t, 100> buf;

int main(void) {
  halInit();
  System::init();
  Init(aout, dout, ain, din, modbus, eeprom);
  Shell sh;
  const char str[] = "ABCDEFGHIJK";
  eeprom.Write(nvram::Section::Modbus, str);
  buf.fill(0);
  eeprom.Read(nvram::Section::Modbus, buf, sizeof(str));
  nvram::log("%s", buf.data());
  systime_t time = chVTGetSystemTimeX();
  while(true) {
    time += S2ST(1);
    BaseThread::sleepUntil(time);
    ++uptimeCounter;
  }
}
