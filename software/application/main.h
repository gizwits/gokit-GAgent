#ifndef __MAIN_H
#define __MAIN_H

/* Definition for USARTx resources **********************************************/
#define GPIO_CLK_INIT		RCC_AHB1PeriphClockCmd

#define USARTx_CLK          	RCC_APB1Periph_USART2
#define USARTx_CLK_INIT		    RCC_APB1PeriphClockCmd

#define USARTx_RX_PIN         GPIO_Pin_3
#define USARTx_RX_SOURCE      GPIO_PinSource3
#define USARTx_RX_GPIO_PORT   GPIOA
#define USARTx_RX_GPIO_CLK    RCC_AHB1Periph_GPIOA
#define USARTx_RX_AF          GPIO_AF_USART2

#define USARTx_TX_PIN         GPIO_Pin_2
#define USARTx_TX_SOURCE      GPIO_PinSource2
#define USARTx_TX_GPIO_PORT   GPIOA
#define USARTx_TX_GPIO_CLK    RCC_AHB1Periph_GPIOA
#define USARTx_TX_AF          GPIO_AF_USART2

#define USARTx_CTS_PIN        GPIO_Pin_0
#define USARTx_CTS_SOURCE     GPIO_PinSource0
#define USARTx_CTS_GPIO_PORT  GPIOA
#define USARTx_CTS_GPIO_CLK   RCC_AHB1Periph_GPIOA
#define USARTx_CTS_AF         GPIO_AF_USART2

#define USARTx_RTS_PIN        GPIO_Pin_1
#define USARTx_RTS_SOURCE     GPIO_PinSource1
#define USARTx_RTS_GPIO_PORT  GPIOA
#define USARTx_RTS_GPIO_CLK   RCC_AHB1Periph_GPIOA
#define USARTx_RTS_AF         GPIO_AF_USART2

#define USARTx_IRQn           USART2_IRQn
#define USARTx	              USART2
#define USARTx_IRQHandler     USART2_IRQHandler

/*********************Add by alex**************************/


/**********************************************************/
#endif /* __MAIN_H */
