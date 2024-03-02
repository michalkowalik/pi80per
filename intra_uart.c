//
// Created by Michal Kowalik on 11.02.24.
//
#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "floppy/floppy.h"
#include "debug.h"

void on_uart_intra_rx() {
    int command;
    int length;
    uint8_t buffer[256];
    uint8_t index = 0;

    while (uart_is_readable(UART_INTRA)) {
        command = uart_getc(UART_INTRA);

        if (command == 0) {
            length = uart_getc(UART_INTRA);
            debug_printf("Command: 0x00, length: %02x \r\n", length);

            while (index < length) {
                buffer[index++] = uart_getc(UART_INTRA);
            }
            // TODO: make sure it doesn't cause any issues
            // if it does, enqueue the request, and process it in the main loop
            uart_write_blocking(UART_IO, buffer, index);
        } else if (command == 6) { // write sector to floppy
            length = uart_getc(UART_INTRA);

            while (index < length) {
                buffer[index++] = uart_getc(UART_INTRA);
            }
            process_floppy_write(buffer);

        } else if ((command > 1 && command < 6) || command == 7) {
            process_floppy_command(command, uart_getc(UART_INTRA));
        }
    }
}

// send confirmation of a command
void send_confirmation(uint8_t command, uint8_t status) {
    uint8_t uart_data[] = {command, status};
    uart_write_blocking(UART_INTRA, uart_data, 2);
}

void send_data(uint8_t command, uint8_t *data, uint8_t length) {
    uint8_t uart_data[] = {command, length};
    uart_write_blocking(UART_INTRA, uart_data, 2);
    uart_write_blocking(UART_INTRA, data, length);
}
