#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "B-FRAM-FileSystem.h"
#include "fram_driver.h"

#define FIRST_RUN

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);

/*Need to declare FS struct as a global variable*/

file_system_t BFFS;

/*Function to facilitate debugging through uart2, adapt to your needs */
int stm32printf(const char *format, ...)
{
    char str[200];
    va_list args;
    va_start(args, format);
    int n = vsprintf(str, format, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t *)str, n, HAL_MAX_DELAY);
    return n;
}

int main(void)
{
	/*The way this example should be run implies that:
	 * There is a first run, in which first_run is set to 0, where most file functionality is shown
	 * There is a second run, in which first_run is set to 1, where the functionality of loading the file system struct
	 * (including the files in which data was stored in the previous run) from FRAM and handling it.
	 */
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_SPI1_Init();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

	stm32printf("FRAM Size: %d\n",FRAM_SIZE);
	stm32printf("File Struct Size: %d\n",FILE_STRCT_SIZE);
	stm32printf("Max Files: %d\n",MAX_FILES);
	stm32printf("FS Struct Size =(Max Files*File Size)+8 = FS_OFFSET: %d\n",FS_STRCT_SIZE);
	stm32printf("Usable Size: FRAM Size - FS Size = %d \n",USABLE_SIZE);
	stm32printf("--------------------------------\n");

	bffs_st status;

	file_t* myfile1;
	file_t* myfile2;

	uint8_t data_w[3] = {100,200,130};
	uint8_t data_r;
	uint8_t data_r2[3];

	char myfilename1[10] = "file1.txt";
	char myfilename2[10] = "file2.txt";

#ifdef FIRST_RUN //To run first

		stm32printf("Mounting file system \n");
		if ((status = mount_fs()) != MOUNT_FS_SUCCESS)
			while(1);

		stm32printf("Reset file system \n"); //for this example we want a cleared file system
		if ((status = reset_fs()) != RESET_FS_SUCCESS)
			while(1);

		stm32printf(" FS Free Bytes: %d \n FS Size: %d\n FS Free File Slots: %d\n FS Total File Slots: %d\n FS Total Files: %d\n",
				get_fs_free_bytes(),
				get_fs_size(),
				get_fs_free_file_slots(),
				get_fs_total_file_slots(),
				get_fs_total_files());

		stm32printf("Creating file 1... \n");

		if((status = create_file(myfilename1,10,&myfile1)) != CREATE_FILE_SUCCESS)
			while(1);

		stm32printf("Creating file 2... \n");

		if((status = create_file(myfilename2,10,&myfile2)) != CREATE_FILE_SUCCESS)
			while(1);

		stm32printf(" File 1 Free Bytes: %d \n File Taken Bytes: %d\n Total File Size: %d\n File Read pointer at :%d\n",
				get_file_free_bytes(myfile1),
				get_file_used_bytes(myfile1),
				get_file_size(myfile1),
				tell_file(myfile1));

		stm32printf("Writing [%d,%d,%d] to file 1...\n",data_w[0],data_w[1],data_w[2]);
		if ((status = write_file(myfile1,3,data_w)) != WRITE_FILE_SUCCESS)
			while(1);
		stm32printf("Seeking byte 2 of file 1...\n");
		if ((status = seek_file(myfile1,2)) != SEEK_FILE_SUCCESS)
			while(1);
		stm32printf(" File 1 Free Bytes: %d \n File Taken Bytes: %d\n Total File Size: %d\n File Read pointer at: %d\n",
				get_file_free_bytes(myfile1),
				get_file_used_bytes(myfile1),
				get_file_size(myfile1),
				tell_file(myfile1));

		if ((status = read_file(myfile1,1,&data_r,READ_FILE_RESET_READ_PTR)) != READ_FILE_SUCCESS)
			while(1);
		stm32printf("On file 1: have read 1 byte at sought position, resetting read pointer, data: %d \n",data_r);

		if ((status = read_file(myfile1,3,data_r2,READ_FILE_RESET_READ_PTR)) != READ_FILE_SUCCESS)
			while(1);
		stm32printf("On file 1: have read 3 bytes at start, resetting read pointer, data: [%d,%d,%d] \n",data_r2[0],data_r2[1],data_r2[2]);

		stm32printf("Clearing file 1\n");
		if ((status = clear_file(myfile1)) != CLEAR_FILE_SUCCESS)
			while(1);

		if ((status = read_file(myfile1,3,data_r2,READ_FILE_RESET_READ_PTR)) != READ_FILE_SUCCESS)
			while(1);
		stm32printf("On file 1: have read 3 bytes at start, resetting read pointer, data: [%d,%d,%d] \n",data_r2[0],data_r2[1],data_r2[2]);

		stm32printf("Writing [%d,%d,%d] to file 1 AGAIN...\n",data_w[0],data_w[1],data_w[2]);
		if ((status = write_file(myfile1,3,data_w)) != WRITE_FILE_SUCCESS)
			while(1);

		stm32printf(" FS Free Bytes: %d \n FS Size: %d\n FS Free File Slots: %d\n FS Total File Slots: %d\n FS Total Files: %d\n",
				get_fs_free_bytes(),
				get_fs_size(),
				get_fs_free_file_slots(),
				get_fs_total_file_slots(),
				get_fs_total_files());

#else //to run second

		stm32printf("Mounting file system \n");
		if ((status = mount_fs()) != MOUNT_FS_SUCCESS)
			while(1);

		stm32printf(" FS Free Bytes: %d \n FS Size: %d\n FS Free File Slots: %d\n FS Total File Slots: %d\n FS Total Files: %d\n",
				get_fs_free_bytes(),
				get_fs_size(),
				get_fs_free_file_slots(),
				get_fs_total_file_slots(),
				get_fs_total_files());

		stm32printf("Opening %s...\n",myfilename1);
		if((status = open_file(myfilename1,&myfile1)) != OPEN_FILE_SUCCESS)
			while(1);

		stm32printf(" File 1 Free Bytes: %d \n File Taken Bytes: %d\n Total File Size: %d\n File Read pointer at: %d\n",
				get_file_free_bytes(myfile1),
				get_file_used_bytes(myfile1),
				get_file_size(myfile1),
				tell_file(myfile1));
		if ((status = read_file(myfile1,3,data_r2,READ_FILE_RESET_READ_PTR)) != READ_FILE_SUCCESS)
			while(1);
		stm32printf("On file 1: have read 3 bytes at start, resetting read pointer, data: [%d,%d,%d] \n",data_r2[0],data_r2[1],data_r2[2]);

		if ((status = reset_fs()) != RESET_FS_SUCCESS)
			while(1);
		stm32printf("Reset File System since its the only way of deleting files at the moment \n");

		stm32printf(" FS Free Bytes: %d \n FS Size: %d\n FS Free File Slots: %d\n FS Total File Slots: %d\n FS Total Files: %d\n",
				get_fs_free_bytes(),
				get_fs_size(),
				get_fs_free_file_slots(),
				get_fs_total_file_slots(),
				get_fs_total_files());

#endif
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{


  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
