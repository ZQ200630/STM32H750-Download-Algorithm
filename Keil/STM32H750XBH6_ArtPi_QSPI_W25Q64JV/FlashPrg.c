/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for New Device Flash
 * @version  V1.0.0
 * @date     28. October 2020
 ******************************************************************************/
/*
 * Copyright (c) 2010-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "FlashOS.h"        // FlashOS Structures

/* 
   Mandatory Flash Programming Functions (Called by FlashOS):
                int Init        (unsigned long adr,   // Initialize Flash
                                 unsigned long clk,
                                 unsigned long fnc);
                int UnInit      (unsigned long fnc);  // De-initialize Flash
                int EraseSector (unsigned long adr);  // Erase Sector Function
                int ProgramPage (unsigned long adr,   // Program Page Function
                                 unsigned long sz,
                                 unsigned char *buf);

   Optional  Flash Programming Functions (Called by FlashOS):
                int BlankCheck  (unsigned long adr,   // Blank Check
                                 unsigned long sz,
                                 unsigned char pat);
                int EraseChip   (void);               // Erase complete Device
      unsigned long Verify      (unsigned long adr,   // Verify Function
                                 unsigned long sz,
                                 unsigned char *buf);

       - BlanckCheck  is necessary if Flash space is not mapped into CPU memory space
       - Verify       is necessary if Flash space is not mapped into CPU memory space
       - if EraseChip is not provided than EraseSector for all sectors is called
*/

#include "main.h"
#include "quadspi.h"

extern QSPI_HandleTypeDef hqspi;
unsigned int DevAddr;

/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */
int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
  int ret = 0;
  volatile int i;
  volatile unsigned char * ptr = (volatile unsigned char * )&hqspi;

  for (i = 0; i < sizeof(hqspi); i++) {
    *ptr++ = 0U;
  }

  DevAddr = adr;

  __disable_irq();

  HAL_Init();
  SystemInit();

  SystemClock_Config();
  SystemCoreClockUpdate();
    MX_GPIO_Init();
  if (CSP_QUADSPI_Init() != HAL_OK) {
    ret = 1;
  }
        //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
    //HAL_GPIO_WritePin(GPIOI, GPIO_PIN_8, GPIO_PIN_RESET);
    
    //HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_8);
    //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15);
  return (ret);
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {
  int ret = 0;
   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);
   HAL_GPIO_WritePin(GPIOI, GPIO_PIN_8, GPIO_PIN_SET);
  if (hqspi.State != HAL_QSPI_STATE_BUSY_MEM_MAPPED) {
    if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) {
      ret = 1;
    }
  }

  return (ret);
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseChip (void) {
  int ret = 0;
    HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_8);
  if (CSP_QSPI_Erase_Chip () != HAL_OK) {
    ret = 1;
  }
  HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_8);
  return (ret);
}


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr) {
  int ret = 1;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15);
  if (adr >= DevAddr) {
    adr -= DevAddr;

    if (CSP_QSPI_EraseSector (adr, adr + MEMORY_BLOCK_SIZE) == HAL_OK) {
      ret = 0;
    }
  }

  return (ret);
}


/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {
  int ret = 1;
    HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_8);
  if (adr >= DevAddr) {
    adr -= DevAddr;

    if (CSP_QSPI_WriteMemory (buf, adr, sz) == HAL_OK) {
      ret = 0;
    }
  }

  return (ret);
}

unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf){
  volatile unsigned long ret = adr + sz;
  volatile int i;


  if (hqspi.State != HAL_QSPI_STATE_BUSY_MEM_MAPPED) {
    if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) {
      ret = 0U;
    }
  }
  
  if (ret != 0U) {
    for (i = 0; i < sz; i++) {
      if ((*((volatile unsigned char  *) (adr+i))) != buf[i]) {
        // Verification Failed (return address)
        ret = adr + i;
        break;
      }
      HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_8);
       HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15);
    }
  }

  return (ret);
}

int  BlankCheck  (unsigned long adr, unsigned long sz, unsigned char pat) {
  int ret = 0;
  volatile int i;

  if (hqspi.State != HAL_QSPI_STATE_BUSY_MEM_MAPPED) {
    if (CSP_QSPI_EnableMemoryMappedMode() != HAL_OK) {
      ret = 1;
    }
  }
  
  if (ret == 0) {
    for (i = 0; i < sz; i++) {
      if ((*((volatile unsigned char  *) (adr+i))) != pat) {
        ret = 1;
        break;
      }
    }
  }

  if (CSP_QUADSPI_Init() != HAL_OK) {
    HAL_Delay (100);
    if (CSP_QUADSPI_Init() != HAL_OK) {
      ret = 1;
    }
  }

  return (ret);
}
