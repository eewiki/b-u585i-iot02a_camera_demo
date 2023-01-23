/**
  ******************************************************************************
  * @file    app_camera.h
  * @author  Matt Mielke
  * @brief   Camera control header file
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
#ifndef __APP_CAMERA_H__
#define __APP_CAMERA_H__

/* Includes ------------------------------------------------------------------*/
#include "tx_api.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint8_t *buf;
  uint32_t len;
} JPEGImg_t;

/* Exported constants --------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/
UINT Camera_Init(VOID *memory_ptr);

/* External variables --------------------------------------------------------*/
extern JPEGImg_t currentImg;
extern TX_MUTEX ImgMutex;
extern TX_SEMAPHORE ImgSemaphore;

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DEFAULT_PRIORITY         10  /* Pirority IP creation */
#define ENTRY_INPUT              0  /* Entry input for Main thread */
#define THREAD_PRIO              4  /* Main Thread priority */
#define THREAD_PREEMPT_THRESHOLD 10  /* Main Thread preemption threshold */
#define CAMERA_STACK_SIZE        1024  /* Web application size */

#endif /* __APP_CAMERA_H__ */
