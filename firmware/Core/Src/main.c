#include "stm32f103xe.h"
#include "stdint.h"

// prototypes 

void clockConfig(void);
void GPIOconfig(void);
void ADC_init(void);
void UART_init(void);
void UARTsendByte(uint8_t b);
void UARTsendString(const char *s);
void UARTsendUnit(uint16_t v);
uint16_t ADC_read_channel(uint8_t ch);

void clockConfig(void){
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY));
  
  FLASH->ACR |= FLASH_ACR_LATENCY_2;
  FLASH->ACR |= FLASH_ACR_PRFTBE;

  RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
  RCC->CFGR |= (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);

  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY));

  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
  RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1);

  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
} 

  void GPIOconfig(void) {
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN);

    // A0 (вода) - аналоговый 
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    
    // A1 (свет) — аналоговый
    GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
    
    // A2 (температура) - аналоговый
    GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);

    // B8, B9 (дальномер) - цифровой
    GPIOB->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8  |  GPIO_CRH_MODE9 | GPIO_CRH_CNF9);

    GPIOB->CRH |= (GPIO_CRH_MODE8_0 | GPIO_CRH_MODE8_1);
    GPIOB->CRH |= GPIO_CRH_CNF9_0;

    // B12, B13, B14 (часы) - цифровой 
    GPIOB->CRH &= ~(GPIO_CRH_MODE12|GPIO_CRH_CNF12  |  GPIO_CRH_MODE13|GPIO_CRH_CNF13  |  GPIO_CRH_MODE14|GPIO_CRH_CNF14);

    GPIOB->CRH |= (GPIO_CRH_MODE12_0 | GPIO_CRH_MODE12_1);
    GPIOB->CRH |= (GPIO_CRH_MODE13_0|GPIO_CRH_MODE13_1  |  GPIO_CRH_CNF13_0);
    GPIOB->CRH |= (GPIO_CRH_MODE14_0 | GPIO_CRH_MODE14_1);
}

void DS_WriteByte(uint8_t data){
    for (int i = 0; i < 8; i++){
      if (data & (1 << i)){
        GPIOB->BSRR = (1 << 13);
      } else {
        GPIOB->BRR = (1 << 13);
      }
      GPIOB->BSRR = (1 << 12);
      GPIOB->BRR = (1 << 12);
      }
    }  

uint8_t DS_Read(void){
  uint8_t data = 0;
  GPIOB->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
  GPIOB->CRH |= GPIO_CRH_CNF13_0;
  for (int i = 0; i < 8; i++){
    GPIOB->BSRR = (1 << 12);
    if (GPIOB->IDR & (1 << 13)){
      data |= (1 << i);
    }
   GPIOB->BRR = (1 << 12);
  }
  GPIOB->CRH |= (GPIO_CRH_MODE13_0 | GPIO_CRH_MODE13_1);

  return data;
}

void DS_WriteReg(uint8_t addr, uint8_t data){
  GPIOB->BRR = (1 << 14);
  GPIOB->BRR = (1 << 12);

  GPIOB->BSRR = (1 << 14);

  DS_WriteByte(addr);
  DS_WriteByte(data);

  GPIOB->BRR = (1 << 14);
}

uint8_t DS_ReadReg(uint8_t addr){
  GPIOB->BRR = (1 << 14);
  GPIOB->BRR = (1 << 12);

  GPIOB->BSRR = (1 << 14);

  DS_WriteByte(addr | 0x01);

  uint8_t data = DS_Read();
  GPIOB->BRR = (1 << 14);

  return data;
}

void DS_Init(void){
  DS_WriteReg(0x8E, 0x00);
  DS_WriteReg(0x80, 0x00);
}

uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

void DS_SetTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t month, uint8_t year){
  DS_Init();

  DS_WriteReg(0x80, dec_to_bcd(sec));
  DS_WriteReg(0x82, dec_to_bcd(min));
  DS_WriteReg(0x84, dec_to_bcd(hour));
  DS_WriteReg(0x86, dec_to_bcd(day));
  DS_WriteReg(0x88, dec_to_bcd(month));
  DS_WriteReg(0x8C, dec_to_bcd(year));

  DS_WriteReg(0x8E, 0x80);
}

void DS_GetTime(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *day, uint8_t *month, uint8_t *year){
  *sec = bcd_to_dec(DS_ReadReg(0x80));
  *min = bcd_to_dec(DS_ReadReg(0x82));
  *hour = bcd_to_dec(DS_ReadReg(0x84));
  *day = bcd_to_dec(DS_ReadReg(0x86));
  *month = bcd_to_dec(DS_ReadReg(0x88));
  *year = bcd_to_dec(DS_ReadReg(0x8C));
}

  void UARTsend2Digit(uint8_t value){
    if (value < 10) UARTsendByte('0');
    UARTsendUnit(value);
  }

void ADC_init(void){
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

  RCC->CFGR &= ~RCC_CFGR_ADCPRE;
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;
  
  ADC1->CR2 |= ADC_CR2_ADON;
  for(volatile uint32_t i = 0; i < 100000; i++);
  
  ADC1->CR2|= ADC_CR2_RSTCAL;
  while (ADC1->CR2 & ADC_CR2_RSTCAL);

  ADC1->CR2 |= ADC_CR2_CAL;
  while (ADC1->CR2 & ADC_CR2_CAL);

  ADC1->SMPR2 &= ~(ADC_SMPR2_SMP0 | ADC_SMPR2_SMP2 | ADC_SMPR2_SMP9);
  ADC1->SMPR2 |= (ADC_SMPR2_SMP0_2 | ADC_SMPR2_SMP2_2 | ADC_SMPR2_SMP9_2);
  
  ADC1->SMPR1 &= ~ADC_SMPR1_SMP12;
  ADC1->SMPR1 |= ADC_SMPR1_SMP12_2;
  
  ADC1->SQR1 &= ~ADC_SQR1_L;

  ADC1->CR2 &= ~ADC_CR2_EXTSEL;
  ADC1->CR2 |= ADC_CR2_EXTSEL;
  ADC1->CR2 |= ADC_CR2_EXTTRIG;
}

void UART3_init(void){
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

  GPIOB->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);  
  GPIOB->CRH |= (GPIO_CRH_MODE10_0 | GPIO_CRH_MODE10_1 | GPIO_CRH_CNF10_1);

  GPIOB->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11);
  GPIOB->CRH |= GPIO_CRH_CNF11_0;

  USART3->BRR = (19 << 4) | 8;

  USART3->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void UART3sendByte(uint8_t b){
  while (!(USART3->SR & USART_SR_TXE));
  USART3->DR = b;
}

void UART3sendString(const char *s){
  while (*s) UART3sendByte((uint8_t)*s++);
}

void UART3sendUnit (uint16_t v){
  char buf[10];
  int i = 0;

  if (v == 0){
    UART3sendByte('0');
    return;
  }

  while (v) {
    buf[i++] = '0' + (v % 10);
    v /= 10;
  } 
  while (--i >= 0) UART3sendByte(buf[i]);
}

void ESP_sendCmd(const char *cmd){
  UART3sendString(cmd);
  UART3sendString("\r\n");
}

void ESP_readResponse(void){
  uint32_t timeout = 1000000;
  while (timeout > 0) {
    if (USART3->SR & USART_SR_RXNE){
      uint8_t b = USART3->DR;
      UARTsendByte(b);
      timeout = 1000000;
    } else {
      timeout--;
    }
  }
}

// Простая задержка в миллисекундах (busy-wait, ~72 МГц)
void delay_ms(uint32_t ms){
  while(ms--){
    for(volatile uint32_t i = 0; i < 16000; i++);
  }
}

// Очистка приёмного буфера USART3 (убрать "мусор" стартового сообщения ESP)
void UART3_flush(void){
  while(USART3->SR & USART_SR_RXNE){ (void)USART3->DR; }
}

// Чтение ответа ESP заданное время (мс), с эхо в USART1 (ПК)
void ESP_readResponseMs(uint32_t ms){
  uint32_t timeout = ms * 8000;
  while(timeout > 0){
    if(USART3->SR & USART_SR_RXNE){
      UARTsendByte((uint8_t)USART3->DR);
      timeout = ms * 8000;      // сброс таймаута при каждом новом байте
    } else {
      timeout--;
    }
  }
}

// Чтение ответа ESP в буфер за заданное время (мс). Эхо в USART1 (ПК).
// Возвращает количество принятых байт.
int ESP_readBuf(char *buf, int maxlen, uint32_t ms){
  uint32_t timeout = ms * 8000;
  int n = 0;
  while(timeout > 0 && n < maxlen - 1){
    if(USART3->SR & USART_SR_RXNE){
      uint8_t b = (uint8_t)USART3->DR;
      buf[n++] = b;
      UARTsendByte(b);          // эхо в ПК
      timeout = ms * 8000;      // сброс таймаута при новом байте
    } else {
      timeout--;
    }
  }
  buf[n] = '\0';
  return n;
}

// Поиск подстроки в буфере. Возвращает 1, если найдено.
int ESP_contains(const char *buf, const char *sub){
  int i = 0;
  while(buf[i]){
    int j = 0;
    while(sub[j] && buf[i + j] == sub[j]) j++;
    if(sub[j] == '\0') return 1;
    i++;
  }
  return 0;
}

// Отправка строки по установленному TCP-соединению (AT+CIPSEND).
// Возвращает 1 при успехе (SEND OK), иначе 0.
int ESP_sendData(const char *data){
  char resp[80];
  int len = 0;
  while(data[len]) len++;

  // 1) объявляем длину
  UART3sendString("AT+CIPSEND=");
  UART3sendUnit((uint16_t)len);
  UART3sendString("\r\n");

  // 2) ждём приглашение '>'
  ESP_readBuf(resp, sizeof(resp), 1000);
  if(!ESP_contains(resp, ">")) return 0;

  // 3) отправляем сырые данные
  UART3sendString(data);

  // 4) ждём подтверждение
  ESP_readBuf(resp, sizeof(resp), 2000);
  if(ESP_contains(resp, "SEND OK")) return 1;
  return 0;
}

void UART_init(void){
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
  GPIOA->CRH |= (GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1);

  GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
  GPIOA->CRH |= GPIO_CRH_CNF10_0;

  USART1->BRR = (39 << 4) | 1;

  USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void UARTsendByte(uint8_t b){
  while (!(USART1->SR & USART_SR_TXE));
  USART1->DR = b;
}

void UARTsendString(const char *s){
  while (*s) UARTsendByte((uint8_t)*s++);
}

void UARTsendUnit (uint16_t v){
  char buf[10];
  int i = 0;

  if (v == 0){
    UARTsendByte('0');
    return;
  }

  while (v) {
    buf[i++] = '0' + (v % 10);
    v /= 10;
  } 
  while (--i >= 0) UARTsendByte(buf[i]);
}

uint16_t ADC_read_channel(uint8_t ch){
  ADC1->SQR3 &= ~ADC_SQR3_SQ1;
  ADC1->SQR3 |= (ch << ADC_SQR3_SQ1_Pos);

  ADC1->SR &= ~ADC_SR_EOC;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  
  uint32_t timeout = 1000000;
  while(!(ADC1->SR & ADC_SR_EOC) && timeout > 0) timeout--;

  if (timeout == 0) return 0xFFFF;
  
  return (uint16_t)(ADC1->DR & 0xFFFF);
}

void delay_us(uint32_t us){
  TIM2->CNT = 0;
  while (TIM2->CNT < us);
}

void TIM2_Init(void){
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  TIM2->PSC = 71;
  TIM2->ARR = 65535;
  TIM2->CNT = 0;
  TIM2->CR1 |= TIM_CR1_CEN;
}

void HC_Trig(void){   
  GPIOB->BRR = (1 << 8);
  GPIOB->BSRR = (1 << 8);
  delay_us(10);
  GPIOB->BRR = (1 << 8);
}

uint8_t HC_Echo(void){
  uint32_t timeout = 1000000;
  while (!(GPIOB->IDR & GPIO_IDR_IDR9) && timeout > 0){
    timeout--;
  } if (timeout == 0) return 0; 
  return 1;
}

uint32_t HC_MeasureEcho(void){
  TIM2->CNT = 0; 
  while (GPIOB->IDR & GPIO_IDR_IDR9);
  uint32_t meas = TIM2->CNT;
  return meas; 
}

uint32_t HC_Read(void){
  HC_Trig();
  uint32_t distance = 0;
  uint32_t res = HC_Echo();
  if (res) {
    uint32_t duration = HC_MeasureEcho();
    distance = duration / 58;
  } else return 0;
  return distance;
}

int main(void) {
    clockConfig();
    GPIOconfig(); 
    UART_init();
    UART3_init();

    // ESP-01 нужно время на загрузку после подачи питания.
    // AT-команды, посланные раньше, молча теряются.
    delay_ms(2000);
    UART3_flush();                 // убрать стартовый "мусор" из буфера USART3

    // ---- Инициализация TCP-соединения (однократно) ----
    // ВАЖНО: на ПК (192.168.4.2) должен слушать порт 8080 TCP-сервер.
    UARTsendString("CIPSTART:\r\n");
    ESP_sendCmd("AT+CIPMUX=0");                        // одиночное соединение
    ESP_readResponseMs(1000);

    ESP_sendCmd("AT+CIPSTART=\"TCP\",\"192.168.4.2\",8080");
    ESP_readResponseMs(5000);                          // соединению даём 5 с
    delay_ms(1000);

    // Тестовая отправка — проверка канала end-to-end
    UARTsendString("SEND test:\r\n");
    if(ESP_sendData("Hello from STM32 weather station!\r\n")){
        UARTsendString(">>> SEND OK\r\n");
    } else {
        UARTsendString(">>> SEND FAIL\r\n");
    }

    while (1){
        UARTsendString("--- loop ---\r\n");

        ESP_sendCmd("AT");
        ESP_readResponseMs(1000);

        ESP_sendCmd("AT+CWMODE?");
        ESP_readResponseMs(1000);

        ESP_sendCmd("AT+CIPSTATUS");
        ESP_readResponseMs(1000);

        ESP_sendCmd("AT+CIPMUX?");                     // '?' — иначе ERROR
        ESP_readResponseMs(1000);

        delay_ms(2000);
    }
}