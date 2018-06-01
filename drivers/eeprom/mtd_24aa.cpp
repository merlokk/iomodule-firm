/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

    This file is part of 24AA lib.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <cstring>
#include <cstdlib>

#include "ch.hpp"

#include "mtd_24aa.hpp"

namespace nvram {

  /*
   ******************************************************************************
   * DEFINES
   ******************************************************************************
   */

  /*
   ******************************************************************************
   * EXTERNS
   ******************************************************************************
   */

  /*
   ******************************************************************************
   * PROTOTYPES
   ******************************************************************************
   */

  /*
   ******************************************************************************
   * GLOBAL VARIABLES
   ******************************************************************************
   */

  /*
   ******************************************************************************
   ******************************************************************************
   * LOCAL FUNCTIONS
   ******************************************************************************
   ******************************************************************************
   */
  /**
   * @brief     Calculates requred timeout.
   */
  static systime_t calc_timeout(size_t bytes, uint32_t clock)
  {
    const uint32_t bitsinbyte = 10;
    uint32_t tmo_ms;
    tmo_ms = ((bytes + 1) * bitsinbyte * 1000);
    tmo_ms /= clock;
    tmo_ms += 10; /* some additional milliseconds to be safer */
    return MS2ST(tmo_ms);
  }

  msg_t Mtd24aa::i2c_read(uint8_t* rxbuf, size_t len,
                          uint8_t* writebuf, size_t preamble_len)
  {
#if defined(STM32F1XX_I2C)
    bool rewind = false;
    if(1 == len) {
      len = 2;
      //if last byte requested
      if(*(uint16_t*)writebuf == capacity() - 1) {
        --writebuf[0];
        rewind = true;
      }
    }
#endif /* defined(STM32F1XX_I2C) */
    msg_t status;
    systime_t tmo = calc_timeout(len + preamble_len, i2cp->config->clock_speed);
    uint8_t addrSuffix = (preamble_len == 1 ? writebuf[1] : 0);
    osalDbgCheck((nullptr != rxbuf) && (0 != len));
#if I2C_USE_MUTUAL_EXCLUSION
    i2cAcquireBus(this->i2cp);
#endif
    status = i2cMasterTransmitTimeout(this->i2cp, this->addr | addrSuffix,
                                      writebuf, preamble_len, rxbuf, len, tmo);
    if(MSG_OK != status) {
      i2cflags = i2cGetErrors(this->i2cp);
    }
#if I2C_USE_MUTUAL_EXCLUSION
    i2cReleaseBus(this->i2cp);
#endif
#if defined(STM32F1XX_I2C)
    if(rewind) {
      rxbuf[0] = rxbuf[1];
    }
#endif /* defined(STM32F1XX_I2C) */
    return status;
  }

  msg_t Mtd24aa::i2c_write(const uint8_t* txdata, size_t len,
                           uint8_t* writebuf, size_t preamble_len)
  {
    msg_t status;
    systime_t tmo = calc_timeout(len + preamble_len, i2cp->config->clock_speed);
    uint8_t addrSuffix = (preamble_len == 1 ? writebuf[1] : 0);
    if((nullptr != txdata) && (0 != len)) {
      memcpy(&writebuf[preamble_len], txdata, len);
    }
#if I2C_USE_MUTUAL_EXCLUSION
    i2cAcquireBus(this->i2cp);
#endif
    status = i2cMasterTransmitTimeout(this->i2cp, this->addr | addrSuffix,
                                      writebuf, preamble_len + len, nullptr, 0, tmo);
    if(MSG_OK != status) {
      i2cflags = i2cGetErrors(this->i2cp);
    }
#if I2C_USE_MUTUAL_EXCLUSION
    i2cReleaseBus(this->i2cp);
#endif
    return status;
  }

  bool Mtd24aa::wait_op_complete(void)
  {
    if(0 != cfg.programtime) {
      osalThreadSleep(cfg.programtime);
    }
    return OSAL_SUCCESS;
  }

  /**
   * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
   *          or can be placed in write buffer (for FRAM)
   */
  size_t Mtd24aa::bus_write(const uint8_t* txdata, size_t len, uint32_t offset)
  {
    msg_t status;
    osalDbgCheck((this->writebuf_size - cfg.addr_len) >= len);
    osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
    this->acquire();
    *(uint32_t*)writebuf = offset;
    /* write preamble. Only address bytes for this memory type */
    status = i2c_write(txdata, len, writebuf, cfg.addr_len);
    wait_op_complete();
    this->release();
    if(status == MSG_OK) {
      return len;
    }
    else {
      return 0;
    }
  }

  size_t Mtd24aa::bus_read(uint8_t* rxbuf, size_t len, uint32_t offset)
  {
    msg_t status;
    osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
    osalDbgCheck(this->writebuf_size >= cfg.addr_len);
    this->acquire();
    addr2buf(writebuf, offset, cfg.addr_len);
    status = i2c_read(rxbuf, len, writebuf, cfg.addr_len);
    this->release();
    if(MSG_OK == status) {
      return len;
    }
    else {
      return 0;
    }
  }

  /*
   ******************************************************************************
   * EXPORTED FUNCTIONS
   ******************************************************************************
   */

  Mtd24aa::Mtd24aa(const MtdConfig& cfg, uint8_t* writebuf, size_t writebuf_size,
                   I2CDriver* i2cp, i2caddr_t addr) :
    MtdBase(cfg, writebuf,  writebuf_size),
    i2cp(i2cp),
    addr(addr),
    i2cflags{}
  { }

} //nvram
