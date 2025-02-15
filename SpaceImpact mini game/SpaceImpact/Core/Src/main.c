/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "lcd.h"
#include "font.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BULLETS_MAX_NUM 30
#define MOBS_MAX_NUM 3
#define TERMINAL_XLINE 112
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define POSITION(_adc) (((_adc % 4096) * 140) / 4095)
#define FPS_TO_TICKS(_fps) (1000 / _fps)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim9;

/* Definitions for Draw */
osThreadId_t DrawHandle;
const osThreadAttr_t Draw_attributes = {
  .name = "Draw",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Control */
osThreadId_t ControlHandle;
const osThreadAttr_t Control_attributes = {
  .name = "Control",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for MTX1 */
osMutexId_t MTX1Handle;
const osMutexAttr_t MTX1_attributes = {
  .name = "MTX1"
};
/* USER CODE BEGIN PV */

typedef struct {
    uint8_t x;
    uint8_t y;
    struct queue *nextElem;
    struct queue *prevElem;
} queue;

typedef struct {
	queue *startPtr;
	queue *endPtr;
	int8_t cntrElem;
	int8_t maxItemNum;
} item_t;
item_t bullets;
item_t mobs;


uint16_t adc_value;
uint16_t adc_prev_value;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM9_Init(void);
void StartDraw(void *argument);
void StartControl(void *argument);

/* USER CODE BEGIN PFP */
uint8_t addQueueFrame();
void delQueueFrame();
void protagonistDraw();
void bulletDraw();
void mobDraw();
void collisionFinder();
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_TIM9_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_IT(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  srand(HAL_ADC_GetValue(&hadc1));						// Getting seed for RNG from ADC

  bullets.startPtr = NULL;
  bullets.endPtr = NULL;
  bullets.cntrElem = 0;
  bullets.maxItemNum = BULLETS_MAX_NUM;

  mobs.startPtr = NULL;
  mobs.endPtr = NULL;
  mobs.cntrElem = 0;
  mobs.maxItemNum = MOBS_MAX_NUM;

  HAL_TIM_Base_Start_IT(&htim9);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of MTX1 */
  MTX1Handle = osMutexNew(&MTX1_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Draw */
  DrawHandle = osThreadNew(StartDraw, NULL, &Draw_attributes);

  /* creation of Control */
  ControlHandle = osThreadNew(StartControl, NULL, &Control_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 42000;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 6000;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_CS_Pin|LCD_A0_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LCD_RESET_Pin LCD_CS_Pin LCD_A0_Pin */
  GPIO_InitStruct.Pin = LCD_RESET_Pin|LCD_CS_Pin|LCD_A0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
uint8_t addQueueFrame(item_t *Item)
{
    if (Item->cntrElem < Item->maxItemNum)
    {
        queue *nextBuf = (queue *)malloc(sizeof(queue));
        if(nextBuf) {
          queue *prevBuf = Item->endPtr;
          Item->endPtr = (Item->endPtr == NULL)  ?  (Item->startPtr = nextBuf)  :  (queue *)(Item->endPtr->nextElem = (struct queue *)nextBuf);
          Item->endPtr->nextElem = NULL;
          Item->endPtr->prevElem = (struct queue *)prevBuf;
          Item->cntrElem++;
          return 0;
        }
    }
    return 1;
}

void delQueueFrame(item_t *Item, queue *frame)
{
    if (Item->cntrElem > 0 && frame != NULL)
    {
        if (Item->startPtr == frame && Item->endPtr == frame) {     // 1 - element queue
            Item->startPtr = NULL;
            Item->endPtr = NULL;
        } else if (Item->startPtr == frame) {                       // frame == start
            Item->startPtr = (queue*)frame->nextElem;
            Item->startPtr->prevElem = NULL;
        } else if (Item->endPtr == frame) {                         // frame == end
            Item->endPtr = (queue*)frame->prevElem;
            Item->endPtr->nextElem = NULL;
        } else {                                                    // frame is between start and end
            ((queue*)frame->prevElem)->nextElem = frame->nextElem;
            ((queue*)frame->nextElem)->prevElem = frame->prevElem;
        }

        free(frame);
        --(Item->cntrElem);
    }
}

void protagonistDraw()
{
	if(POSITION(adc_value) != POSITION(adc_prev_value)) {					// motionless protagonist figure flicker reduction
			osMutexAcquire(MTX1Handle, osWaitForever);
			lcd_fill_rect( 3, POSITION(adc_prev_value), 14, 20, ST7735_BLACK);
			lcd_fill_rect( 3, POSITION(adc_value),       5, 20, ST7735_CYAN);
			lcd_fill_rect( 8, POSITION(adc_value) + 5,   3, 10, ST7735_CYAN);
			lcd_fill_rect(11, POSITION(adc_value) + 8,   3,  4, ST7735_CYAN);
			adc_prev_value = adc_value;
			osMutexRelease(MTX1Handle);
	}
}

void bulletDraw()
{
/* Bullet dimensions:
 * X: 5px.
 * Y: 2px.
 * */
	for(queue *buffer = bullets.startPtr; buffer != NULL; buffer = (queue*)(buffer->nextElem)) {
		if((buffer->x + 5) < 127) {
			lcd_fill_rect(buffer->x++, buffer->y, 5, 2, ST7735_BLACK);
			lcd_fill_rect(buffer->x,   buffer->y, 5, 2, ST7735_GREEN);
		} else {
			lcd_fill_rect(buffer->x, buffer->y, 5, 2, ST7735_BLACK);
			delQueueFrame(&bullets, buffer);
		}
	}
}

void mobDraw()
{
/* Mob dimensions:
 * X: 10px.
 * Y: 10px.
 * */
	for(queue *buffer = mobs.startPtr; buffer != NULL; buffer = (queue*)(buffer->nextElem)) {
			lcd_line(buffer->x - 5, buffer->y - 5, buffer->x + 5, buffer->y + 5, ST7735_RED);
			lcd_line(buffer->x + 5, buffer->y - 5, buffer->x - 5, buffer->y + 5, ST7735_RED);
			lcd_fill_circle(buffer->x, buffer->y, 5, ST7735_RED);
	}
}

void collisionFinder()
{
	for(queue *bullet = bullets.startPtr; bullet != NULL && bullet->x > TERMINAL_XLINE && bullet->x < TERMINAL_XLINE + 10; bullet = (queue*)(bullet->nextElem))
		for (queue *mob = mobs.startPtr; mob != NULL; mob = (queue*)(mob->nextElem))
			if ((bullet->y < mob->y + 5 && bullet->y > mob->y - 5) || (bullet->y + 2 > mob->y - 5 && bullet->y + 2 < mob->y + 5)) {
				lcd_fill_rect(bullet->x, bullet->y, 5, 2, ST7735_BLACK);
				delQueueFrame(&bullets, bullet);
				lcd_fill_rect(mob->x - 5, mob->y - 5, 11, 11, ST7735_BLACK);
				delQueueFrame(&mobs, mob);
			}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(!addQueueFrame(&bullets)) {
		bullets.endPtr->x = 14;
		bullets.endPtr->y = POSITION(adc_prev_value) + 9;		// ?
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	if(htim != &htim9)
			return;

	if(!addQueueFrame(&mobs)) {
		mobs.endPtr->x = 117;				// x,y - mob`s center coordinates
		mobs.endPtr->y = rand() % 139 + 10;

		for(queue *buffer = mobs.startPtr; buffer->nextElem != NULL; buffer = (queue*)(buffer->nextElem)) {
			uint8_t newMobL = mobs.endPtr->y - 6;		// new mob`s left  side Y-coordinate + 1px. gap
			uint8_t newMobR = mobs.endPtr->y + 6;		// new mob`s right side Y-coordinate + 1px. gap
			uint8_t oldMobL = buffer->y - 6;
			uint8_t oldMobR = buffer->y + 6;
			if((newMobL >= oldMobL && newMobL <= oldMobR) || (newMobR >= oldMobL && newMobR <= oldMobR)) {
				mobs.endPtr->y = rand() % 139 + 10;
				buffer = mobs.startPtr;
			}
		}

	}
	__HAL_TIM_SET_AUTORELOAD(&htim9, rand() % 8000 + 2000);		// spawn of the next mob in the time range of 1-5 seconds
	__HAL_TIM_SET_COUNTER(&htim9, 0);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDraw */
/**
  * @brief  Function implementing the Draw thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDraw */
void StartDraw(void *argument)
{
  /* USER CODE BEGIN 5 */
	if(lcd_init())
	  while(1){}
	lcd_fill(ST7735_BLACK);
	adc_value = 2048;
	adc_prev_value = adc_value + 1024;		// adc_prev_value != adc_value at first
  /* Infinite loop */
  for(;;)
  {
	protagonistDraw();
	bulletDraw();
	collisionFinder();
	mobDraw();

    osDelay(FPS_TO_TICKS(60));
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartControl */
/**
* @brief Function implementing the Control thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartControl */
void StartControl(void *argument)
{
  /* USER CODE BEGIN StartControl */
  /* Infinite loop */
  for(;;)
  {
	HAL_ADC_Start_IT(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

	osMutexAcquire(MTX1Handle, osWaitForever);
	adc_value = HAL_ADC_GetValue(&hadc1);
	osMutexRelease(MTX1Handle);

    osDelay(FPS_TO_TICKS(60));
  }
  /* USER CODE END StartControl */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
