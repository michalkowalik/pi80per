//
// Created by Michal Kowalik on 11.02.24.
//

#ifndef PI80PER_CONFIG_H
#define PI80PER_CONFIG_H

#define UART_IO uart0                // Console
#define BAUD_RATE 115200
#define UART_IO_TX_PIN 0
#define UART_IO_RX_PIN 1

#define UART_INTRA uart1              // intra-pi communication
#define INTRA_BAUD_RATE 921600
//#define INTRA_BAUD_RATE 115200
#define UART_INTRA_TX_PIN 8
#define UART_INTRA_RX_PIN 9

#define LED_PIN 25


#endif //PI80PER_CONFIG_H
