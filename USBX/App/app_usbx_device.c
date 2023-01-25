/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_device.c
  * @author  MCD Application Team
  * @brief   USBX Device applicative file
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

/* Includes ------------------------------------------------------------------*/
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_otg.h"
#include "ux_device_class_cdc_ecm.h"
#include "ux_dcd_stm32.h"
#include "ux_device_descriptors.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

TX_BYTE_POOL ux_byte_pool;
TX_THREAD ux_app_thread;
UX_SLAVE_CLASS_CDC_ECM_PARAMETER cdc_ecm_parameter;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

void  usbx_app_thread_entry(ULONG arg);

/* USER CODE END PFP */
/**
  * @brief  Application USBX Device Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
  UINT ret = UX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_USBX_Device_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_USBX_Device_MEM_POOL */

  /* USER CODE BEGIN MX_USBX_Device_Init */

  ULONG device_framework_fs_length;   /* Device framework full speed length */
  ULONG string_framework_length;      /* Device string framework length */
  ULONG languge_id_framework_length;  /* Device string framework length */
  UCHAR *device_framework_full_speed; /* Device framework full speed */
  UCHAR *string_framework;            /* Framework string */
  UCHAR *language_id_framework;       /* Language id framework */
  CHAR *ux_app_pointer;               /* USBX  app memory pointer. */

  /* Allocate the USBX_MEMORY_SIZE. */
  if (tx_byte_allocate(byte_pool, (VOID **) &ux_app_pointer,
                       USBX_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    printf("USBX_MEMORY_SIZE allocation failed: 0x%02x\n", ret);
    return TX_POOL_ERROR;
  }

  /* Initialize USBX Memory */
  ux_system_initialize(ux_app_pointer, USBX_MEMORY_SIZE, UX_NULL, 0);

  /* Get_Device_Framework_Full_Speed and get the length */
  device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED,
                                                                &device_framework_fs_length);

  /* Get_String_Framework and get the length */
  string_framework = USBD_Get_String_Framework(&string_framework_length);

  /* Get_Language_Id_Framework and get the length */
  language_id_framework = USBD_Get_Language_Id_Framework(&languge_id_framework_length);

  /* The code below is required for installing the device portion of USBX.
     In this application */
  if (ux_device_stack_initialize(NULL,
                                 0U,
                                 device_framework_full_speed,
                                 device_framework_fs_length,
                                 string_framework,
                                 string_framework_length,
                                 language_id_framework,
                                 languge_id_framework_length, UX_NULL) != UX_SUCCESS)
  {
    printf("Device stack init failed: 0x%02x\n", ret);
    return UX_ERROR;
  }

  /* Set the parameters for callback when insertion/extraction of a CDC device. Set to NULL.*/
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_activate = UX_NULL;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_deactivate = UX_NULL;

  /* Define a NODE ID. */
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[0] = 0x00;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[1] = 0x02;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[2] = 0x02;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[3] = 0x03;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[4] = 0x00;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[5] = 0x00;

  /* Define a remote NODE ID. */
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[0] = 0x00;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[1] = 0x02;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[2] = 0x02;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[3] = 0x03;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[4] = 0x00;
  cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[5] = 0x00;

  /* Initialize the device cdc_ecm class. */
  if (ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name,
                                       ux_device_class_cdc_ecm_entry, 1, 0,
                                       &cdc_ecm_parameter) != UX_SUCCESS)
  {
    printf("Device cdc_ecm class Init failed: 0x%02x\n", ret);
    return UX_ERROR;
  }

  /* Perform the initialization of the network driver. This will initialize the USBX network layer.*/
  ux_network_driver_init();

  /* Allocate the stack for main_usbx_app_thread_entry. */
  if (tx_byte_allocate(byte_pool, (VOID **) &ux_app_pointer,
                       USBX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    printf("USBX_APP_STACK_SIZE allocation failed: 0x%02x\n", ret);
    return TX_POOL_ERROR;
  }

  /* Create the usbx_app_thread_entry.  */
  if (tx_thread_create(&ux_app_thread, "main_usbx_app_thread_entry",
                       usbx_app_thread_entry, 0,
                       ux_app_pointer, USBX_APP_STACK_SIZE,
                       DEFAULT_THREAD_PRIO, DEFAULT_PREEMPTION_THRESHOLD,
                       TX_NO_TIME_SLICE,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    printf("ux_app_thread creation failed: 0x%02x\n", ret);
    return TX_THREAD_ERROR;
  }

  /* USER CODE END MX_USBX_Device_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

/**
  * @brief  Function implementing usbx_app_thread_entry.
  * @param arg: Not used
  * @retval None
  */
void usbx_app_thread_entry(ULONG arg)
{
  /* Initialization of USB device */
  if (MX_USB_Device_Init() != UX_SUCCESS)
  {
    while (1);
  }
}

UINT MX_USB_Device_Init(void)
{
  /* USER CODE BEGIN USB_Device_Init_PreTreatment_0 */
  /* USER CODE END USB_Device_Init_PreTreatment_0 */

//  /* Enable USB voltage detector */
//  HAL_PWREx_EnableUSBVoltageDetector();

  /* Initialize the device controller HAL driver */
  MX_USB_OTG_FS_PCD_Init();

  /* USER CODE BEGIN USB_Device_Init_PreTreatment_1 */
  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x100);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x10);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x20);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x10);
  /* USER CODE END USB_Device_Init_PreTreatment_1 */

  /* Initialize and link controller HAL driver to USBx */
  if (ux_dcd_stm32_initialize((ULONG)USB_OTG_FS, (ULONG)&hpcd_USB_OTG_FS) != TX_SUCCESS)
  {
    printf("Failed to initialize USB device controller");
    return UX_ERROR;
  }

  //if (nx_ip_interface_attach(&EthIP, )

  /* Start USB device by connecting the DP pullup */
  HAL_PCD_Start(&hpcd_USB_OTG_FS);

  /* USER CODE BEGIN USB_Device_Init_PostTreatment */

  return UX_SUCCESS;

  /* USER CODE END USB_Device_Init_PostTreatment */
}

/* USER CODE END 1 */
