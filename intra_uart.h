//
// Created by Michal Kowalik on 11.02.24.
//

#ifndef PI80PER_INTRA_UART_H
#define PI80PER_INTRA_UART_H

#include "pico/stdlib.h"

void on_uart_intra_rx();

void send_confirmation(uint8_t command, uint8_t status);
void send_data(uint8_t command, uint8_t *data, uint8_t length);


#endif //PI80PER_INTRA_UART_H
