/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.h
  * @author  MCD Application Team
  * @brief   NetXDuo applicative header file
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
#ifndef __APP_NETXDUO_H__
#define __APP_NETXDUO_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "nx_api.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define PRINT_ADDRESS(addr) do { \
                                    printf("%s: %lu.%lu.%lu.%lu \n", #addr, \
                                    (addr >> 24) & 0xff, \
                                    (addr >> 16) & 0xff, \
                                    (addr >> 8) & 0xff, \
                                     addr& 0xff);\
                                  }while(0)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_NetXDuo_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define IP_MEMORY_SIZE           2048  /* IP Memory size */
#define DEFAULT_PRIORITY         10  /* Pirority IP creation */
#define PACKET_PAYLOAD_SIZE      1536  /*Packet payload size */
#define ARP_CACHE_SIZE           1024  /* APP Cache size  */
#define NX_PACKET_POOL_SIZE      ((PACKET_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 60)  /* Packet pool size */
#define WAIT_OPTION              1000  /* Wait option for getting @IP */
#define ENTRY_INPUT              0  /* Entry input for Main thread */
#define THREAD_PRIO              4  /* Main Thread priority */
#define THREAD_PREEMPT_THRESHOLD 10  /* Main Thread preemption threshold */
#define WEB_STACK_SIZE           2048  /* Web application size */
#define CONNECTION_PORT          80  /* HTTP connection port */
#define SERVER_PACKET_SIZE       (NX_WEB_HTTP_SERVER_MIN_PACKET_SIZE * 2)  /* Server packet size */
#define SERVER_STACK             4096  /* Server stack */
#define SERVER_POOL_SIZE         (SERVER_PACKET_SIZE * 4)  /* Server pool size */
#define NETX_APP_BYTE_POOL_SIZE  ((1024 * 10) + (ARP_CACHE_SIZE + IP_MEMORY_SIZE + WEB_STACK_SIZE))  /* Netx byte pool */
#define NULL_IP_ADDRESS          IP_ADDRESS(0,0,0,0)  /* define the NULL ip address */

/* Convert from millisecond to ThreadX Tick value (from ux_api.h). */
#define NX_MS_TO_TICK(ms)        ((ms) * (NX_IP_PERIODIC_RATE) / 1000)

/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_NETXDUO_H__ */
