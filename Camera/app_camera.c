/**
  ******************************************************************************
  * @file    app_camera.c
  * @author  Matt Mielke
  * @brief   Camera control file
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

/* Includes ------------------------------------------------------------------*/
#include "app_camera.h"
#include "b_u585i_iot02a_camera.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define CAMERA_BUF_LENGTH 0xFFFC

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
TX_THREAD Camera_thread;
TX_SEMAPHORE VsyncSemaphore;
TX_SEMAPHORE ImgSemaphore;
TX_MUTEX ImgMutex;
uint32_t CameraBuf[2][CAMERA_BUF_LENGTH >> 2];
volatile uint32_t CameraBufLen;
volatile uint8_t CameraError;

/* Exported variables --------------------------------------------------------*/
JPEGImg_t currentImg;


/* Private function prototypes -----------------------------------------------*/
/* Camera control thread entry */
void camera_thread_entry(ULONG thread_input);


/* Exported function definitions ---------------------------------------------*/
UINT Camera_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* Camera App memory pointer. */
  UCHAR   *camera_app_pointer;

  /* Allocate the camera stack instance. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &camera_app_pointer,
                         CAMERA_STACK_SIZE, TX_NO_WAIT);

  /* Check camera stack allocation */
  if (ret != TX_SUCCESS)
  {
    printf("Camera stack allocation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  ret = tx_thread_create(&Camera_thread, "Camera thread", camera_thread_entry,
                         ENTRY_INPUT, camera_app_pointer, CAMERA_STACK_SIZE,
                         DEFAULT_PRIORITY, THREAD_PREEMPT_THRESHOLD,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Check Camera Thread creation */
  if (ret != TX_SUCCESS)
  {
    printf("Camera Thread creation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create semaphore */
  ret = tx_semaphore_create(&VsyncSemaphore, "Camera Vsync Semaphore", 0);

  /* Check semaphore creation */
  if (ret != TX_SUCCESS)
  {
    printf("VsyncSemaphore creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create semaphore */
  ret = tx_semaphore_create(&ImgSemaphore, "Image Updated Semaphore", 0);

  /* Check semaphore creation */
  if (ret != TX_SUCCESS)
  {
    printf("ImgSemaphore creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create Mutex */
  ret = tx_mutex_create(&ImgMutex, "Image Buffer Mutex", TX_NO_INHERIT);

  /* Check Mutex creation */
  if (ret != TX_SUCCESS)
  {
    printf("ImgMutex creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  return ret;
}


/* Private function definitions ----------------------------------------------*/
/**
* @brief  Application thread camera
* @param  thread_input : thread input
* @retval None
*/
void camera_thread_entry(ULONG thread_input)
{
  UINT status;
  uint8_t bufIdx;

  BSP_CAMERA_Init(0, CAMERA_R640x480, CAMERA_PF_JPEG);
  tx_thread_sleep(100); // about 1 sec.

  HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET);
  bufIdx = 0;
  CameraError = 0;
  BSP_CAMERA_Start(0, (uint8_t *)CameraBuf[bufIdx], CAMERA_BUF_LENGTH, CAMERA_MODE_CONTINUOUS);

  while (1)
  {
    /* Check if the semaphore is released (new image received) */
    if (tx_semaphore_get(&VsyncSemaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
      Error_Handler();
    }

    HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET);

    /* Update current image buffer pointer and length when consumer not using them */
    if (CameraError == 0) // If BSP_CAMERA_ErrorCallback() wasn't triggered (i.e., no overflow)
    {
      status = tx_mutex_get(&ImgMutex, TX_WAIT_FOREVER);
      currentImg.buf = (uint8_t *)CameraBuf[bufIdx];
      currentImg.len = CameraBufLen;
      status = tx_mutex_put(&ImgMutex);

      status = tx_semaphore_ceiling_put(&ImgSemaphore, 1); // notify consumer of new image
      bufIdx ^= 0x1; // toggle between 0 and 1
    }

    CameraError = 0;
    HAL_DMA_Start_IT(hcamera_dcmi->DMA_Handle, (uint32_t)&hcamera_dcmi->Instance->DR, (uint32_t)CameraBuf[bufIdx], CAMERA_BUF_LENGTH);
    HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET);
//    printf("%d\n", CameraBufLen);
  }
}

/**
* @brief  Synchronization frame (VSYNC) interrupt handler
* @param  Instance : camera instance
* @retval None
*/
void BSP_CAMERA_VsyncEventCallback(uint32_t Instance)
{
  HAL_GPIO_TogglePin(DEBUG1_GPIO_Port, DEBUG1_Pin);
//  printf("V");

  CameraBufLen = CAMERA_BUF_LENGTH - __HAL_DMA_GET_COUNTER(hcamera_dcmi->DMA_Handle);

  if (CameraBufLen != 0)
  {
    HAL_DMA_Abort(hcamera_dcmi->DMA_Handle); // TODO: Use HAL_DMA_Abort_IT
    tx_semaphore_put(&VsyncSemaphore);
  }

}

void BSP_CAMERA_LineEventCallback(uint32_t Instance)
{
//  printf("L");
}

/**
  * @brief  Frame Event callback.
  * @param  Instance Camera instance.
  * @retval None
  */
void BSP_CAMERA_FrameEventCallback(uint32_t Instance)
{
//  printf("F");
}

/**
  * @brief  Error callback.
  * @param  Instance Camera instance.
  * @retval None
  */
void BSP_CAMERA_ErrorCallback(uint32_t Instance)
{
//  printf("E");
  CameraError = 1;
}
