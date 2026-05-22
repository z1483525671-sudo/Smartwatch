/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <string.h>
#include <stdlib.h> 
#include "rtc.h"

extern UART_HandleTypeDef huart2;
extern void MPU6050_Read_Accel(int16_t *ax, int16_t *ay, int16_t *az);
void Sensor_Task(void *pvParameters);
void Process_Task(void *pvParameters);
void Receive_Task(void *pvParameters);
void MedicineReminder_Task(void *pvParameters);

void WIFI_Task(void *pvParameters);
QueueHandle_t xAccelQueue;
QueueHandle_t xUartRxQueue;  

uint8_t rx_byte; 
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
/* USER CODE BEGIN Variables */
 uint8_t rx_byte;

/* USER CODE END Variables */
osThreadId UIHandle;
osThreadId CommHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
xAccelQueue = xQueueCreate(10, sizeof(int16_t) * 3);
if (xAccelQueue == NULL) {
    while (1);
}

// ????????(??256??)
xUartRxQueue = xQueueCreate(256, sizeof(uint8_t));
if (xUartRxQueue == NULL) {
    while (1);
}

// ?? USART2 ????,????1???
HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_byte, 1);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  osThreadDef(SensorTask, Sensor_Task, osPriorityHigh, 0, 512);
    osThreadCreate(osThread(SensorTask), NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  osThreadDef(ProcessTask, Process_Task, osPriorityNormal, 0, 1024);
    osThreadCreate(osThread(ProcessTask), NULL);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */


osThreadDef(WiFiTask, WIFI_Task, osPriorityNormal, 0, 1024);
osThreadCreate(osThread(WiFiTask), NULL);
/*
  osThreadDef(ReceiveTask, Receive_Task, osPriorityNormal, 0, 1024);
    osThreadCreate(osThread(ReceiveTask), NULL);
*/
osThreadDef(MedicineReminder, MedicineReminder_Task, osPriorityNormal, 0, 256);
osThreadCreate(osThread(MedicineReminder), NULL);

  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of UI */
  osThreadDef(UI, StartDefaultTask, osPriorityNormal, 0, 128);
  UIHandle = osThreadCreate(osThread(UI), NULL);

  /* definition and creation of Comm */
  osThreadDef(Comm, StartTask02, osPriorityBelowNormal, 0, 128);
  CommHandle = osThreadCreate(osThread(Comm), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the UI thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Comm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE BEGIN 4 */








// ?? AT ??
static void ESP8266_Send(const char *cmd) {
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 1000);
}

// ?????????(????)
// ????????,???? 0
static int ESP8266_RecvFrame(char *buf, int maxlen, uint32_t timeout_ms) {
    int len = 0;
    uint8_t ch;
    TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < end) {
        if (xQueueReceive(xUartRxQueue, &ch, pdMS_TO_TICKS(20)) == pdPASS) {
            if (len < maxlen - 1) {
                buf[len++] = ch;
            }
            // ??????:????????,????
            end = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
        }
    }
    if (len > 0) buf[len] = '\0';
    return len;
}


static int WiFi_WaitFor(const char *keyword, uint32_t total_timeout_ms) {
    char frame[256];
    TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(total_timeout_ms);
    while (xTaskGetTickCount() < end) {
        if (ESP8266_RecvFrame(frame, sizeof(frame), 200) > 0) {
            if (strstr(frame, keyword) != NULL) {
                return 1;
            }
        }
    }
    return 0;
}



void MedicineReminder_Task(void *pvParameters)
{
    
    typedef struct {
        uint8_t hour;
        uint8_t minute;
    } MedTime_t;

    const MedTime_t med_times[] = {
        {8, 0},   
        {12, 0}, 
        {18, 0}   
    };
    const int num_times = sizeof(med_times) / sizeof(med_times[0]);

  
    int last_alert[num_times];
    for (int i = 0; i < num_times; i++) {
        last_alert[i] = 0;
    }

    uint8_t last_minute = 0xFF;  

    for (;;)
    {
        uint8_t hour, minute, second;
       
        RTC_TimeTypeDef sTime;
        if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
					 printf("Time: %02d:%02d:%02d\r\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        hour = sTime.Hours;
        minute = sTime.Minutes;
        second = sTime.Seconds;

        if (minute != last_minute)
        {
            last_minute = minute;
   
            for (int i = 0; i < num_times; i++)
            {
                if (hour == med_times[i].hour && minute == med_times[i].minute)
                {
                    if (!last_alert[i])
                    {
                      
                        printf("[REMINDER] Take medicine now! Time: %02d:%02d\r\n", hour, minute);
                     
                        last_alert[i] = 1; 
                    }
                }
                else
                {
                
                    if (last_alert[i] == 1 && (hour != med_times[i].hour || minute != med_times[i].minute))
                    {
                        last_alert[i] = 0;
                    }
                }
            }
        }

       
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}




void WIFI_Task(void *pvParameters) {
    // ---- WiFi ?? ----
    const char *ssid = "??WiFi?";
    const char *pwd  = "??WiFi??";
    char cmd[128];

    // ? ESP8266 ??
    vTaskDelay(pdMS_TO_TICKS(3000));

    // ?? AT
    ESP8266_Send("AT\r\n");
    if (!WiFi_WaitFor("OK", 2000)) goto error;

    // ?? Station ??
    ESP8266_Send("AT+CWMODE=1\r\n");
    if (!WiFi_WaitFor("OK", 2000)) goto error;

    // ?? WiFi
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    ESP8266_Send(cmd);
    if (!WiFi_WaitFor("OK", 15000)) goto error;

    // ?? IP
    ESP8266_Send("AT+CIFSR\r\n");
    if (WiFi_WaitFor("STAIP", 3000)) {
        // ????:PC13 LED ??
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        while (1) { vTaskDelay(1000); }   // ????
    }

error:
    // ??:PC13 LED ??
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}





/*
void Receive_Task(void *pvParameters)
{
    printf("start\r\n");

    char *test_msg = "Hello UART!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)test_msg, strlen(test_msg), 100);

    uint8_t rx_buf[128];
    uint16_t rx_len = 0;
    uint8_t ch;
    const TickType_t xByteTimeout = pdMS_TO_TICKS(20);   // ?????,???

    for (;;)
    {
        // ????????(???????)
        if (xQueueReceive(xUartRxQueue, &ch, portMAX_DELAY) == pdPASS)
        {
            rx_buf[rx_len++] = ch;
            TickType_t xLastByteTime = xTaskGetTickCount();

            // ????,???? 20ms ?????
            while ((xTaskGetTickCount() - xLastByteTime) < xByteTimeout)
            {
                if (xQueueReceive(xUartRxQueue, &ch, 0) == pdPASS)
                {
                    if (rx_len < sizeof(rx_buf) - 1)
                        rx_buf[rx_len++] = ch;
                    xLastByteTime = xTaskGetTickCount();  // ????
                }
                else
                {
                    vTaskDelay(1);   // ?? CPU,????
                }
            }

            // ????,????
            rx_buf[rx_len] = '\0';
            printf("Received %d bytes: %s\r\n", rx_len, rx_buf);
            rx_len = 0;
        }
    }
}

*/







void Process_Task(void *pvParameters)
{
    int16_t data[3];
    // ?????????(??,????)
    static enum {
        STATE_NORMAL,
        STATE_IMPACT,
        STATE_FREE_FALL,
        STATE_STATIONARY
    } fall_state = STATE_NORMAL;
    static uint32_t state_start_time = 0;
    float fx, fy, fz, total_g;

    for (;;)
    {
        if (xQueueReceive(xAccelQueue, data, portMAX_DELAY) == pdTRUE)
        {
            int16_t ax = data[0], ay = data[1], az = data[2];

            // ??? g(?? ?8g,LSB = 4096)
         
            int32_t sum_sq =(int32_t)ax*ax + (int32_t)ay*ay + (int32_t)az*az;

            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;  // ????
					
            // ? switch ???????(??????????)
const int32_t IMPACT_THRESHOLD_SQ    = (int32_t)(2.5f * 4096) * (int32_t)(2.5f * 4096);   // ? 104857600
const int32_t FREE_FALL_THRESHOLD_SQ = (int32_t)(0.6f * 4096) * (int32_t)(0.6f * 4096);   // ?  6029312
const int32_t STATIONARY_LOW_SQ      = (int32_t)(0.8f * 4096) * (int32_t)(0.8f * 4096);   // ? 10737418
const int32_t STATIONARY_HIGH_SQ     = (int32_t)(1.2f * 4096) * (int32_t)(1.2f * 4096);   // ? 24159191
const int32_t CANCEL_THRESHOLD_SQ    = (int32_t)(1.5f * 4096) * (int32_t)(1.5f * 4096);   // ? 37748736

switch (fall_state)
{
    case STATE_NORMAL:
        if (sum_sq > IMPACT_THRESHOLD_SQ)   // ???? ( > 2.5g )
        {
            fall_state = STATE_IMPACT;
            state_start_time = now;
            printf("Impact detected! sum_sq=%ld\r\n", sum_sq);
        }
        break;

    case STATE_IMPACT:
        if (sum_sq < FREE_FALL_THRESHOLD_SQ)   // ???? ( < 0.6g )
        {
            fall_state = STATE_FREE_FALL;
            state_start_time = now;
            printf("Free fall detected!\r\n");
        }
        else if (now - state_start_time > 500)  // ??? 500ms ????,??
        {
            fall_state = STATE_NORMAL;
            printf("Reset from impact (timeout)\r\n");
        }
        break;

    case STATE_FREE_FALL:
        if (sum_sq > STATIONARY_LOW_SQ && sum_sq < STATIONARY_HIGH_SQ)   // ???? (0.8g ~ 1.2g)
        {
            // ???? 2 ?????
            if (now - state_start_time > 2000)
            {
                fall_state = STATE_NORMAL;
                printf("*** FALL DETECTED! *** Send alert.\r\n");
                // ???????????????
            }
        }
        else if (sum_sq > CANCEL_THRESHOLD_SQ)   // ???? (>1.5g),????
        {
            fall_state = STATE_NORMAL;
            printf("Fall cancelled (user moved)\r\n");
        }
        break;

    default:
        fall_state = STATE_NORMAL;
        break;
}

						
						
          
        }
    }
}

void Sensor_Task(void *pvParameters)
{
    int16_t ax, ay, az;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int16_t data[3];

    for (;;)
    {
        MPU6050_Read_Accel(&ax, &ay, &az);
        data[0] = ax; data[1] = ay; data[2] = az;
        // ?????,??????? 0 ??(???,??)
        xQueueSend(xAccelQueue, data, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}
/* USER CODE END 4 */



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
     if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(xUartRxQueue, &rx_byte, &xHigherPriorityTaskWoken);
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
/* USER CODE END Application */
