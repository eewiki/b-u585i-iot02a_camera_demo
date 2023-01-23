/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
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
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "nxd_dhcp_client.h"
#include "nx_web_http_server.h"
#include "ux_network_driver.h"
#include "app_filex.h"
#include "app_camera.h"
#include "app_tof.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* Define the ThreadX, NetX, and FileX object control blocks. */

/* Define Threadx global data structures. */
TX_THREAD Main_thread;
TX_SEMAPHORE Semaphore;

/* Define NetX global data structures. */
NX_PACKET_POOL EthPool;
NX_PACKET_POOL WebServerPool;
NX_PACKET_POOL StreamServerPool;
NX_IP  EthIP;
NX_DHCP DHCPClient;
NX_WEB_HTTP_SERVER HTTPServer;
NX_WEB_HTTP_SERVER StreamHTTPServer;
ULONG IPAddress;
ULONG NetMask;

/* Define FileX global data structures. */
FX_MEDIA FlashMedia;

/* Buffer for FileX FX_MEDIA sector cache. this should be 32-Bytes aligned to avoid
   cache maintenance issues */
ALIGN_32BYTES(uint32_t media_memory[512 / sizeof(uint32_t)]);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PART_BOUNDARY "123456789000000000000987654321"
#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=" PART_BOUNDARY
#define MAX_STREAM_PART_LEN 82
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

static uint8_t nx_web_server_pool[SERVER_POOL_SIZE];
static uint8_t nx_stream_server_pool[SERVER_POOL_SIZE];

static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nCache-Control: no-store\r\n\r\n";
static NX_WEB_HTTP_SERVER_MIME_MAP my_mime_maps[] =
{
  {"css", "text/css"},
  {"svg", "image/svg+xml"},
  {"png", "image/png"},
  {"jpg", "image/jpg"}
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* WEB HTTP server thread entry */
static void  nx_server_thread_entry(ULONG thread_input);

/* DHCP state change notify callback */
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);

/* Web Server callback when a new request from a web client is triggered */
static UINT webserver_request_notify_callback(NX_WEB_HTTP_SERVER *server_ptr,
                                              UINT request_type, CHAR *resource,
                                              NX_PACKET *packet_ptr);

/* Stream Server callback when a new request from a web client is triggered */
static UINT streamserver_request_notify_callback(NX_WEB_HTTP_SERVER *server_ptr,
                                              UINT request_type, CHAR *resource,
                                              NX_PACKET *packet_ptr);

/* USER CODE END PFP */
/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN MX_NetXDuo_Init */

  /* Web App memory pointer. */
  UCHAR   *web_app_pointer;

  /* Initialize the NetXDuo system. */
  nx_system_initialize();

  /* Allocate the web stack instance. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         WEB_STACK_SIZE, TX_NO_WAIT);

  /* Check web stack allocation */
  if (ret != TX_SUCCESS)
  {
    printf("Web stack allocation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the main thread. */
  ret = tx_thread_create(&Main_thread, "Main_thread", nx_server_thread_entry,
                         ENTRY_INPUT, web_app_pointer, WEB_STACK_SIZE,
                         DEFAULT_PRIORITY, THREAD_PREEMPT_THRESHOLD,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Check Main Thread creation */
  if (ret != TX_SUCCESS)
  {
    printf("Main Thread creation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Allocate the packet pool. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         NX_PACKET_POOL_SIZE, TX_NO_WAIT);

  /* Check packet pool allocation */
  if (ret != TX_SUCCESS)
  {
    printf("Packet pool allocation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create a packet pool. */
  ret = nx_packet_pool_create(&EthPool, "NetX Main Packet Pool",
                              PACKET_PAYLOAD_SIZE, web_app_pointer,
                              NX_PACKET_POOL_SIZE);

  /* Check for packet pool creation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Packed creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the web server packet pool. */
  ret = nx_packet_pool_create(&WebServerPool, "Web Server Packet Pool",
                              SERVER_PACKET_SIZE, nx_web_server_pool,
                              SERVER_POOL_SIZE);

  /* Check for web server pool creation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Server pool creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the streaming server packet pool. */
  ret = nx_packet_pool_create(&StreamServerPool, "Stream Server Packet Pool",
                              SERVER_PACKET_SIZE, nx_stream_server_pool,
                              SERVER_POOL_SIZE);

  /* Check for streaming server pool creation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Server pool creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Allocate the IP instance. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         IP_MEMORY_SIZE, TX_NO_WAIT);

  /* Check for IP instance pool Allocation status. */
  if (ret != NX_SUCCESS)
  {
    printf("IP instance Allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create an IP instance by linking the ux network driver */
  ret = nx_ip_create(&EthIP, "NetX IP Instance 0", NULL_IP_ADDRESS,
                     NULL_IP_ADDRESS, &EthPool, _ux_network_driver_entry,
                     web_app_pointer, IP_MEMORY_SIZE, DEFAULT_PRIORITY);

  /* Check the IP instance creation */
  if (ret != NX_SUCCESS)
  {

    printf("IP Instance creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Allocate the arp cache memory. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         ARP_CACHE_SIZE, TX_NO_WAIT);

  /* Check the arp cache memory Allocation status. */
  if (ret != NX_SUCCESS)
  {
    printf("IP instance Allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Enable ARP and supply ARP cache memory for IP Instance 0. */
  ret =  nx_arp_enable(&EthIP, (void *) web_app_pointer, ARP_CACHE_SIZE);

  /* Check the ARP instance activation */
  if (ret != NX_SUCCESS)
  {
    printf("ARP Enable for IP error : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Enable the ICMP support to be able to ping the board */
  ret = nx_icmp_enable(&EthIP);

  /* Check the ICMP activation */
  if (ret != NX_SUCCESS)
  {
    printf("ICMP enable for IP error : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Enable UDP support needed  by DHCP client */
  ret =  nx_udp_enable(&EthIP);

  /* Check UDP support activation */
  if (ret != NX_SUCCESS)
  {
    printf("UDP enable for IP error : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Enable the TCP protocol */
  ret =  nx_tcp_enable(&EthIP);

  /* Check the TCP activation */
  if (ret != NX_SUCCESS)
  {
    printf("TCP enable for IP error : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Allocate the SERVER_STACK memory . */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         SERVER_STACK, TX_NO_WAIT);

  /* Check the SERVER_STACK memory Allocation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Server stack Allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the Web HTTP Server. */
  ret = nx_web_http_server_create(&HTTPServer, "WEB HTTP Server", &EthIP,
                                  CONNECTION_PORT, &FlashMedia, web_app_pointer,
                                  SERVER_STACK, &WebServerPool, NX_NULL,
                                  webserver_request_notify_callback);

  /* Check the web server creation */
  if (ret != NX_SUCCESS)
  {
    printf("HTTP WEB Server creation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Allocate the SERVER_STACK memory . */
  ret = tx_byte_allocate(byte_pool, (VOID **) &web_app_pointer,
                         SERVER_STACK, TX_NO_WAIT);

  /* Check the SERVER_STACK memory Allocation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Server stack Allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the Stream HTTP Server. */
  ret = nx_web_http_server_create(&StreamHTTPServer, "Stream HTTP Server", &EthIP,
                                  81, &FlashMedia, web_app_pointer,
                                  SERVER_STACK, &WebServerPool, NX_NULL,
                                  streamserver_request_notify_callback);

  /* Check the stream server creation */
  if (ret != NX_SUCCESS)
  {
    printf("HTTP WEB Server creation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create the DHCP instance. */
  ret = nx_dhcp_create(&DHCPClient, &EthIP, "dhcp_client");

  /* Check the DHCP instance creation */
  if (ret != NX_SUCCESS)
  {
    printf("DHCP Instance creation failed : 0x%02x\n", ret);
  }

  /* Register a callback function for ip notify */
  ret = nx_ip_address_change_notify(&EthIP, ip_address_change_notify_callback,
                                    NULL);

  /* Check the callback function creation*/
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }

  /* Create semaphore */
  ret = tx_semaphore_create(&Semaphore, "App Semaphore", 0);

  /* Check semaphore creation */
  if (ret != TX_SUCCESS)
  {
    printf("Semaphore creation failed : 0x%02x\n", ret);
    Error_Handler();
  }

  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

/**
* @brief  ip address change callback
* @param  ip_instance : NX_IP instance registered for this callback
* @param   ptr : VOID * optional data pointer
* @retval None
*/
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
  /* as soon as the IP address is ready, the semaphore is released to let the web server start */
  tx_semaphore_put(&Semaphore);
}


/**
* @brief  HTTP(S) server callback routine
*
* The server callback routine is the application request notify callback
* registered with the HTTP server in *nx_web_http_server_create*. It is invoked
* when the HTTP server responds to Client GET, PUT and DELETE requests which
* require an HTTP response.
* @author Matt Mielke
* @param  server_ptr HTTP Server control block
* @param  request_type (e.g., GET, PUT, etc.)
* @param  resource resource requested (e.g., /test.htm)
* @param  packet_ptr
* @retval None
*/
UINT webserver_request_notify_callback(NX_WEB_HTTP_SERVER *server_ptr,
                                       UINT request_type, CHAR *resource,
                                       NX_PACKET *packet_ptr)
{
  UINT status;
  NX_PACKET *resp_packet_ptr;
  char tmp_string[30];
  UINT string_len;
  char data[130];
  UINT data_len;
  ToF_Data_t currToFData;
  NX_PARAMETER_NOT_USED(request_type);
  NX_PARAMETER_NOT_USED(packet_ptr);

  data_len = 0;
  data[data_len] = '\0';

  /* Get the requested data from packet */
  if (strcmp(resource, "/LedOn") == 0)
  {
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  }
  else if (strcmp(resource, "/LedOff") == 0)
  {
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
  }
  else if (strcmp(resource, "/tofData") == 0)
  {
    if (ToF_getRangingData(&currToFData) != TX_SUCCESS)
    {
      return NX_SUCCESS; // TODO: the server will send 404?
    }

    data_len = currToFData.len * sizeof(currToFData.data[0]);
    memcpy(data, currToFData.data, data_len);
    data[data_len] = '\0';
  }
  else
  {
    return NX_SUCCESS;
  }

  /* Derive the client request type from the client request. */
  nx_web_http_server_type_get(server_ptr, server_ptr -> nx_web_http_server_request_resource, tmp_string, &string_len);

  /* Null terminate the string. */
  tmp_string[string_len] = '\0';

  /* Now build a response header with server status is OK and no additional header info. */
  status = nx_web_http_server_callback_generate_response_header(server_ptr, &resp_packet_ptr, NX_WEB_HTTP_STATUS_OK,
                                                                data_len, tmp_string, NX_NULL);

  status = nx_packet_data_append(resp_packet_ptr, data, data_len, server_ptr->nx_web_http_server_packet_pool_ptr, NX_WAIT_FOREVER);

  /* Now send the packet! */
  status = nx_web_http_server_callback_packet_send(server_ptr, resp_packet_ptr);
  if (status != NX_SUCCESS)
  {
    nx_packet_release(resp_packet_ptr);
    return status;
  }
  return(NX_WEB_HTTP_CALLBACK_COMPLETED);
}

/**
* @brief  HTTP(S) stream server callback routine
*
* The server callback routine is the application request notify callback
* registered with the HTTP server in *nx_web_http_server_create*. It is invoked
* when the HTTP server responds to Client GET, PUT and DELETE requests which
* require an HTTP response.
* @author Matt Mielke
* @param  server_ptr HTTP Server control block
* @param  request_type (e.g., GET, PUT, etc.)
* @param  resource resource requested (e.g., /test.htm)
* @param  packet_ptr
* @retval None
*/
UINT streamserver_request_notify_callback(NX_WEB_HTTP_SERVER *server_ptr,
                                       UINT request_type, CHAR *resource,
                                       NX_PACKET *packet_ptr)
{
  UINT status;
  NX_PACKET *resp_packet_ptr;
  CHAR part_hdr[MAX_STREAM_PART_LEN];
  UINT hdr_len;
  uint32_t tmp;
  NX_PARAMETER_NOT_USED(request_type);
  NX_PARAMETER_NOT_USED(packet_ptr);

  /* Get the requested data from packet */
  if (strcmp(resource, "/stream.jpg") == 0)
  {
    status = nx_web_http_server_callback_generate_response_header(server_ptr, &resp_packet_ptr, NX_WEB_HTTP_STATUS_OK, 0, (char *)_STREAM_CONTENT_TYPE, NX_NULL);
    status = nx_web_http_server_callback_packet_send(server_ptr, resp_packet_ptr);

    while (1)
    {
      status = nx_web_http_server_response_packet_allocate(server_ptr, &resp_packet_ptr, NX_WAIT_FOREVER);
      status = nx_packet_data_append(resp_packet_ptr, (char *)_STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY), server_ptr->nx_web_http_server_packet_pool_ptr, NX_WAIT_FOREVER);

      status = tx_semaphore_get(&ImgSemaphore, TX_WAIT_FOREVER); // wait until new image available
      status = tx_mutex_get(&ImgMutex, TX_WAIT_FOREVER); // wait until buffer not being updated
      HAL_GPIO_WritePin(DEBUG2_GPIO_Port, DEBUG2_Pin, GPIO_PIN_SET);

      hdr_len = snprintf(part_hdr, MAX_STREAM_PART_LEN, _STREAM_PART, currentImg.len);
      status = nx_packet_data_append(resp_packet_ptr, part_hdr, hdr_len, server_ptr->nx_web_http_server_packet_pool_ptr, NX_WAIT_FOREVER);

      tmp = resp_packet_ptr->nx_packet_length;
      while (currentImg.len != 0)
      {
        if (tmp == 0)
        {
          status = nx_web_http_server_response_packet_allocate(server_ptr, &resp_packet_ptr, NX_WAIT_FOREVER);
          /* Determine if an error is present.  */
          if (status != NX_SUCCESS)
          {
              /* Indicate an allocation error occurred.  */
              server_ptr->nx_web_http_server_allocation_errors++;

              /* Error, return to caller.  */
              break;
          }
        }

        /* Calculate the maximum length.  */
        tmp = ((ULONG)(resp_packet_ptr->nx_packet_data_end - resp_packet_ptr->nx_packet_append_ptr)) - NX_PHYSICAL_TRAILER;

        if (tmp > server_ptr->nx_web_http_server_current_session_ptr->nx_tcp_session_socket.nx_tcp_socket_connect_mss)
        {
          tmp = server_ptr->nx_web_http_server_current_session_ptr->nx_tcp_session_socket.nx_tcp_socket_connect_mss;
        }

        if (tmp > currentImg.len)
        {
          tmp = currentImg.len;
        }

        /* Copy the data into the response packet buffer.  */
        memcpy(resp_packet_ptr->nx_packet_append_ptr, currentImg.buf, tmp);
        currentImg.buf += tmp; // update source buffer pointer

        /* Update the packet information with the data read.  */
        resp_packet_ptr->nx_packet_length += tmp;
        resp_packet_ptr->nx_packet_append_ptr += tmp;

        status = nx_web_http_server_callback_packet_send(server_ptr, resp_packet_ptr);

        /* Check for success.  */
        if (status != NX_SUCCESS)
        {
            /* Release the packet.  */
            nx_packet_release(resp_packet_ptr);

            /* if connection lost, return */
            if (status == NX_NOT_CONNECTED)
            {
              tx_mutex_put(&ImgMutex);
              HAL_GPIO_WritePin(DEBUG2_GPIO_Port, DEBUG2_Pin, GPIO_PIN_RESET);
              return (NX_WEB_HTTP_CALLBACK_COMPLETED);
            }

            /* Return to caller.  */
            break;
        }

        /* Increment the bytes sent count.  */
        server_ptr->nx_web_http_server_total_bytes_sent += tmp;

        /* Adjust the file length based on what we have sent.  */
        currentImg.len -= tmp;

        /* Indicate new packet needed */
        tmp = 0;
      }

      tx_mutex_put(&ImgMutex);
      HAL_GPIO_WritePin(DEBUG2_GPIO_Port, DEBUG2_Pin, GPIO_PIN_RESET);
    }

    return(NX_WEB_HTTP_CALLBACK_COMPLETED);
  }

  return NX_SUCCESS;
}

/**
* @brief  Application thread for HTTP web server
* @param  thread_input : thread input
* @retval None
*/
void nx_server_thread_entry(ULONG thread_input)
{
  /* Sleep for 1s */
  tx_thread_sleep(NX_MS_TO_TICK(1000));

  /* HTTP WEB SERVER THREAD Entry */
  UINT    status;
  NX_PARAMETER_NOT_USED(thread_input);

  printf("Starting DHCP client...\n");

  /* Start the DHCP Client. */
  status = nx_dhcp_start(&DHCPClient);

  /* Check DHCP Client Starting status. */
  if (status != NX_SUCCESS)
  {
    /* Print DHCP Instance creation error. */
    printf("DHCP Instance Starting error : 0x%02x\n", status);
  }

  /* Check if the semaphore is released */
  if (tx_semaphore_get(&Semaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Checks for errors in the IP address */
  nx_ip_address_get(&EthIP, &IPAddress, &NetMask);

  /* Log IP address */
  PRINT_ADDRESS(IPAddress);
  /* Log NetMask */
  PRINT_ADDRESS(NetMask);

  /* Open the SD disk driver. */
  status = fx_media_open(&FlashMedia, "STM32_SDIO_DISK", fx_stm32_sram_driver,
                        NULL, media_memory, sizeof(media_memory));

  /* Check the media opening status. */
  if (status != FX_SUCCESS)
  {
    /* Print Media Opening error. */
    printf("FX media opening failed : 0x%02x\n", status);
    /* Error, call error handler.*/
    Error_Handler();
  }
  else
  {
    /* Print Media Opening Success. */
    printf("Fx media successfully opened.\n");
  }

  status = nx_web_http_server_mime_maps_additional_set(&HTTPServer,&my_mime_maps[0], 4);

  /* Start the WEB HTTP Server. */
  status = nx_web_http_server_start(&HTTPServer);

  /* Check the WEB HTTP Server starting status. */
  if (status != NX_SUCCESS)
  {
    /* Print HTTP WEB Server starting error. */
    printf("HTTP WEB Server Starting Failed, error: 0x%02x\n", status);
    /* Error, call error handler.*/
    Error_Handler();
  }
  else
  {
    /* Print HTTP WEB Server Starting success. */
    printf("HTTP WEB Server successfully started.\n");
  }

  /* Start the Stream HTTP Server. */
  status = nx_web_http_server_start(&StreamHTTPServer);

  /* Check the WEB HTTP Server starting status. */
  if (status != NX_SUCCESS)
  {
    /* Print HTTP WEB Server starting error. */
    printf("HTTP Stream Server Starting Failed, error: 0x%02x\n", status);
    /* Error, call error handler.*/
    Error_Handler();
  }
  else
  {
    /* Print HTTP WEB Server Starting success. */
    printf("HTTP Stream Server successfully started.\n");
  }
}

/* USER CODE END 1 */
