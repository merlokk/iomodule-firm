/*
 * Copyright (c) 2018 Dmytro Shestakov
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

#include "modbus_impl.h"
#include "digitalin.h"
#include "analogin.h"
#include "order_conv.h"

#include <array>

using Utils::htons;
using Utils::ntohs;
using Utils::htonl;

Modbus modbus;

enum Range {
  R_AnalogInputStart = 32,
  R_AnalogInputSize = 10,
  R_CounterStart = 64,
  R_CounterSize = 14 * 2,
  R_DigitalInputStart = 96,
  R_DigitalInputSize = 1,
  R_AnalogOutputStart = 128,
  R_AnalogOutputSize = 4,
  R_DigitalOutputStart = 160,
  R_DigitalOutputSize = 4
};

extern "C" {

  /**
   * Modbus slave input register callback function.
   *
   * @param pucRegBuffer input register buffer
   * @param usAddress input register address
   * @param usNRegs input register number
   *
   * @return result
   */
  eMBErrorCode eMBRegInputCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs)
  {
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;
    uint16_t* regBuffer16 = (uint16_t*)pucRegBuffer;
    /* it already plus one in modbus function method. */
    --usAddress;
    //Digital inputs data
    if(usAddress == R_DigitalInputStart)
    {
      if(usNRegs == R_DigitalInputSize) {
        *regBuffer16 = htons(Digital::input.GetBinaryVal());
      }
      else {
        eStatus = MB_ENOREG;
      }
    }
    //Counters data, the number of registers must be even
    else if(usAddress >= R_CounterStart) {
      iRegIndex = (int)(usAddress - R_CounterStart);
      if((usNRegs + iRegIndex) <= R_CounterSize && (usNRegs & 0x01) == 0) {
        auto counters = Digital::input.GetCounters();
        while(usNRegs > 0) {
          uint32_t beVal = htonl(counters[(size_t)iRegIndex]);
          *regBuffer16++ = uint16_t(beVal & 0xFFFF);
          *regBuffer16++ = uint16_t(beVal >> 16);
          ++iRegIndex;
          usNRegs -= 2;
        }
      }
      else {
        eStatus = MB_ENOREG;
      }
    }
    //Analog inputs data
    else if(usAddress >= R_AnalogInputStart) {
      iRegIndex = (int)(usAddress - R_AnalogInputStart);
      if((usNRegs + iRegIndex) <= R_AnalogInputSize) {
        auto inputs = Analog::input.GetSamples();
        while(usNRegs > 0) {
          *regBuffer16++ = htons(inputs[(size_t)iRegIndex]);
          ++iRegIndex;
          --usNRegs;
        }
      }
      else {
        eStatus = MB_ENOREG;
      }
    }
    else {
      eStatus = MB_ENOREG;
    }
    return eStatus;
  }

  /**
   * Modbus slave holding register callback function.
   *
   * @param pucRegBuffer holding register buffer
   * @param usAddress holding register address
   * @param usNRegs holding register number
   * @param eMode read or write
   *
   * @return result
   */
  eMBErrorCode eMBRegHoldingCB(UCHAR * /*pucRegBuffer*/, USHORT /*usAddress*/,
                               USHORT /*usNRegs*/, eMBRegisterMode /*eMode*/)
  {

    return MB_ENOERR;
  }

}
