/**
  ******************************************************************************
  * @file    app_tof.c
  * @author  Matt Mielke
  * @brief   ToF sensor control file
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
#include "app_tof.h"
#include "b_u585i_iot02a_ranging_sensor.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
TX_THREAD ToF_thread;
TX_SEMAPHORE IntSemaphore;
static RANGING_SENSOR_ProfileConfig_t Profile;
static RANGING_SENSOR_Result_t distance;

/* Exported variables --------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
/* ToF sensor control thread entry */
void tof_thread_entry(ULONG thread_input);


/* Exported function definitions ---------------------------------------------*/
UINT ToF_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* ToF App memory pointer. */
  UCHAR   *tof_app_pointer;

  /* Allocate the tof stack instance. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &tof_app_pointer,
                         TOF_STACK_SIZE, TX_NO_WAIT);

  /* Check tof stack allocation */
  if (ret != TX_SUCCESS)
  {
    printf("ToF stack allocation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  ret = tx_thread_create(&ToF_thread, "ToF thread", tof_thread_entry,
                         ENTRY_INPUT, tof_app_pointer, TOF_STACK_SIZE,
                         DEFAULT_PRIORITY, THREAD_PREEMPT_THRESHOLD,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Check ToF Thread creation */
  if (ret != TX_SUCCESS)
  {
    printf("ToF Thread creation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create semaphore */
  ret = tx_semaphore_create(&IntSemaphore, "ToF Int Semaphore", 0);

  /* Check semaphore creation */
  if (ret != TX_SUCCESS)
  {
    printf("IntSemaphore creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  return ret;
}

/**
* @brief  Get Time-of-Flight ranging data
* @param  data ranging data
* @retval status
*/
UINT ToF_getRangingData(ToF_Data_t *results)
{
  int i;
  UINT ret;

  ret = TX_SUCCESS;

  /* Check if the semaphore is released (new measurement available) */
  if (tx_semaphore_get(&IntSemaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
  {
    Error_Handler();
  }

  HAL_GPIO_WritePin(DEBUG4_GPIO_Port, DEBUG4_Pin, GPIO_PIN_SET);

  ret = BSP_RANGING_SENSOR_GetDistance(VL53L5A1_DEV_CENTER, &distance);
  results->len = distance.NumberOfZones;
  for (i = 0; i < results->len; i++)
  {
    results->data[i] = distance.ZoneResult[i].Distance[0];
  }
  HAL_GPIO_WritePin(DEBUG4_GPIO_Port, DEBUG4_Pin, GPIO_PIN_RESET);

  return ret;
}


/* Private function definitions ----------------------------------------------*/
/**
* @brief  Application thread Time-of-Flight sensor
* @param  thread_input : thread input
* @retval None
*/
void tof_thread_entry(ULONG thread_input)
{
  if (BSP_RANGING_SENSOR_Init(VL53L5A1_DEV_CENTER) != BSP_ERROR_NONE)
  {
    printf("Failed to initialize ToF Sensor\n");
    Error_Handler();
  }

  Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
  Profile.TimingBudget = 30; /* 5 ms < TimingBudget < 100 ms */
  Profile.Frequency = 7; /* Hz */
  Profile.EnableAmbient = 0; /* Enable: 1, Disable: 0 */
  Profile.EnableSignal = 0; /* Enable: 1, Disable: 0 */

  /* set the profile if different from default one */
  BSP_RANGING_SENSOR_ConfigProfile(VL53L5A1_DEV_CENTER, &Profile);
  BSP_RANGING_SENSOR_Start(VL53L5A1_DEV_CENTER, RS_MODE_ASYNC_CONTINUOUS);

//  while (1)
//  {
//    /* Check if the semaphore is released (new measurement received) */
//    if (tx_semaphore_get(&IntSemaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
//    {
//      Error_Handler();
//    }
//
//    HAL_GPIO_WritePin(DEBUG4_GPIO_Port, DEBUG4_Pin, GPIO_PIN_SET);
//
//    BSP_RANGING_SENSOR_GetDistance(VL53L5A1_DEV_CENTER, &distance);
//    for (i = 0; i < RANGING_SENSOR_MAX_NB_ZONES; i++)
//    {
//      ToF_currentDistance[i] = distance.ZoneResult[i].Distance[0];
//    }
//    printf("%u\n", ToF_currentDistance[20]);
//    HAL_GPIO_WritePin(DEBUG4_GPIO_Port, DEBUG4_Pin, GPIO_PIN_RESET);
//  }
}

/**
* @brief  Ranging Sensor external interrupt callback
* @retval None
*/
void BSP_RANGING_SENSOR_EXTI_Callback(void)
{
  HAL_GPIO_TogglePin(DEBUG3_GPIO_Port, DEBUG3_Pin);
  tx_semaphore_ceiling_put(&IntSemaphore, 1);
}
