/**
  ******************************************************************************
  * @file    app_tof.h
  * @author  Matt Mielke
  * @brief   ToF sensor control header file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_TOF_H__
#define __APP_TOF_H__

/* Includes ------------------------------------------------------------------*/
#include "tx_api.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define DEFAULT_PRIORITY         10  /* Pirority IP creation */
#define ENTRY_INPUT              0  /* Entry input for Main thread */
#define THREAD_PRIO              4  /* Main Thread priority */
#define THREAD_PREEMPT_THRESHOLD 10  /* Main Thread preemption threshold */
#define TOF_STACK_SIZE           2048  /* Web application size */

#define TOF_MAX_RESULTS_SIZE     64

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint16_t data[TOF_MAX_RESULTS_SIZE];
  uint8_t len;
} ToF_Data_t;

/* Exported constants --------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/
UINT ToF_Init(VOID *memory_ptr);
UINT ToF_getRangingData(ToF_Data_t *data);

/* External variables --------------------------------------------------------*/



#endif /* __APP_TOF_H_ */
