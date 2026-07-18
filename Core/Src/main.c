/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "usb_device.h"
#include "gpio.h"
#include "CS43131.h"
#include "DSP.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
// store 2 biquad filters and states for left and right channels (coefficients + state)

//store 3 biquad filters to be sent to the CS43131 (no states/seperate)
// store 2 biquad coefficients and states for left and right channels
static const BiquadCoefficients filtersL_coeffs[2] = {
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}, // Biquad coeffs 1
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}  // Biquad coeffs 2
};

static BiquadFilter filtersL[2] = {
  { &filtersL_coeffs[0], {0,0} },
  { &filtersL_coeffs[1], {0,0} }
};

static const BiquadCoefficients filtersR_coeffs[2] = {
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}, // Biquad coeffs 1
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}  // Biquad coeffs 2
};

static BiquadFilter filtersR[2] = {
  { &filtersR_coeffs[0], {0,0} },
  { &filtersR_coeffs[1], {0,0} }
};

// store 3 biquad coefficients to be sent to the CS43131 (no states/separate)
static const BiquadCoefficients cs43131_filters[3] = {
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}, // Biquad coeffs 1
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000}, // Biquad coeffs 2
  {0x20000000, 0x40000000, 0x20000000, 0x00000000, 0x00000000} // Biquad coeffs 3
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_I2S1_Init();
  MX_USB_DEVICE_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  CS43131_Init();

  
  int32_t result = biquad_process(&filtersL[0], 2);
  result = biquad_process(&filtersL[1], result);
  int32_t resultR = biquad_process(&filtersR[0], 2);
  resultR = biquad_process(&filtersR[1], resultR);


  CS43131_setFilterCoefficients(1, &cs43131_filters[0]);
  CS43131_setFilterCoefficients(2, &cs43131_filters[1]);
  CS43131_setFilterCoefficients(3, &cs43131_filters[2]);
  /*
  CS43131:
  ASP_SPRATE = ASP_SPRATE_48KHZ // set sample rate to 48kHz
  ASP_SPSIZE = ASP_SPSIZE_16BIT // set sample size to 16 bits
  PDN_CTRL &= ~(PDN_ASP | PDN_XTAL | PDN_HP) // power up ASP, XTAL, and HP
  
  */
  /* USER CODE END 2 */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
