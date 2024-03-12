//
// Created by Michal Kowalik on 11.02.24.
//
#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "floppy/floppy.h"
#include "debug.h"

#define UART_BUFFER_SIZE 256

uint8_t uart_buffer[UART_BUFFER_SIZE];
uint8_t uart_read_index = 0;
uint8_t uart_write_index = 0;

void print_uart_buffer() {
    if (uart_read_index != uart_write_index) {
        while (uart_read_index != uart_write_index) {
            uart_putc(UART_IO, uart_buffer[uart_read_index]);
            uart_read_index = (uart_read_index + 1) % UART_BUFFER_SIZE;
        }
    }
}

void on_uart_intra_rx() {
    int command;
    int length;
    uint8_t buffer[256];
    uint8_t index = 0;

    while (uart_is_readable(UART_INTRA)) {
        command = uart_getc(UART_INTRA);

        if (command == 0) {
            length = uart_getc(UART_INTRA);
//            debug_printf("Command: 0x00, length: %02x \r\n", length);

            while (index < length) {
                uart_buffer[uart_write_index++] = uart_getc(UART_INTRA);
                index++;
            }
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
    printf("Sending confirmation: %02x %02x\r\n", command, status);
    uint8_t uart_data[] = {command, status};
    uart_write_blocking(UART_INTRA, uart_data, 2);
}

void send_data(uint8_t command, uint8_t *data, uint8_t length) {
    uint8_t uart_data[] = {command, length};
    uart_write_blocking(UART_INTRA, uart_data, 2);
    uart_write_blocking(UART_INTRA, data, length);
}
