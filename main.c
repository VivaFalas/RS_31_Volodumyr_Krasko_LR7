/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* Private variables ---------------------------------------------------------*/
// Глобальна змінна тривалості імпульсу для PB12 (Завдання 1)
uint32_t pb12_pulse_duration = 200;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Error_Handler(void);

// Прототипи задач (FreeRTOS Tasks)
void Task_PB12(void *argument);
void Task_PB13(void *argument);
void Task_PB15(void *argument);
void Task_Button(void *argument);

/**
  * @brief  Головна функція програми
  */
int main(void)
{
  /* Ініціалізація периферії */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* Створення задач через нативний API FreeRTOS */

  // Задача для PB12: 1 Гц (Період 1000 мс)
  xTaskCreate(Task_PB12, "LED12", 128, NULL, 1, NULL);

  // Задача для PB13: 2 Гц (Період 500 мс)
  xTaskCreate(Task_PB13, "LED13", 128, NULL, 1, NULL);

  // Задача для PB15: 5 Гц (Період 200 мс)
  xTaskCreate(Task_PB15, "LED15", 128, NULL, 1, NULL);

  // Задача для Кнопки PA0: Пріоритет 2 (вищий для швидкої реакції)
  xTaskCreate(Task_Button, "BTN", 128, NULL, 2, NULL);

  /* Запуск планувальника задач */
  vTaskStartScheduler();

  /* Сюди програма ніколи не дійде */
  while (1) {}
}

/* --- Реалізація задач (Tasks) --- */

// Задача 1: PB12 (1 Гц, тривалість спалаху змінюється кнопкою)
void Task_PB12(void *argument) {
  for(;;) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(pb12_pulse_duration));
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(1000 - pb12_pulse_duration));
  }
}

// Задача 2: PB13 (2 Гц, стабільне миготіння)
void Task_PB13(void *argument) {
  for(;;) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(100)); // Імпульс 100 мс
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(400)); // Пауза 400 мс
  }
}

// Задача 3: PB15 (5 Гц, дуже швидке миготіння)
void Task_PB15(void *argument) {
  for(;;) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(50));  // Імпульс 50 мс
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(150)); // Пауза 150 мс
  }
}

// Задача 4: Опитування кнопки PA0 та зміна параметрів PB12
void Task_Button(void *argument) {
  uint8_t last_state = GPIO_PIN_RESET;
  for(;;) {
    uint8_t current_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    // Виявлення моменту натискання (front edge)
    if (current_state == GPIO_PIN_SET && last_state == GPIO_PIN_RESET) {
      vTaskDelay(pdMS_TO_TICKS(50)); // Програмний антибрязк

      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        // Перемикаємо тривалість імпульсу між 200 мс та 500 мс
        pb12_pulse_duration = (pb12_pulse_duration == 200) ? 500 : 200;

        // Очікуємо відпускання кнопки, щоб не перемикати по колу
        while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
          vTaskDelay(pdMS_TO_TICKS(20));
        }
      }
    }
    last_state = current_state;
    vTaskDelay(pdMS_TO_TICKS(20)); // Частота опитування кнопки
  }
}

/* --- Системні налаштування --- */

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Увімкнення тактування */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Початковий стан виходів */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET);

  /* Конфігурація LED на Порту B */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Конфігурація кнопки на PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Стягуючий резистор для стабільності
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void) {
  __disable_irq();
  while (1) {}
}

// Забезпечує роботу HAL Tick на базі TIM2 для сумісності з FreeRTOS
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
}
