//
// Created by Michal Kowalik on 11.02.24.
//
#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "floppy/floppy.h"

void on_uart_intra_rx() {

    int command;
    int length;
    uint8_t buffer[256];
    uint8_t index = 0;

    while (uart_is_readable(UART_INTRA)) {
        command = uart_getc(UART_INTRA);


        if (command == 0) {
            length = uart_getc(UART_INTRA);

            while (index < length) {
                buffer[index++] = uart_getc(UART_INTRA);
            }

            printf("Command: 0x00, length: %02x \r\n", length);
            buffer[index++] = '\r';
            buffer[index++] = '\n';
            uart_write_blocking(UART_IO, buffer, index);
        } else if (command == 2) {
            // set active Drive
            uint8_t drive = uart_getc(UART_INTRA);
            printf("Command: 0x02, drive: %02x \r\n", drive);
            active_drive = drive;
        } else if (command == 3) {
            // set active sector on active drive
            uint8_t sector = uart_getc(UART_INTRA);
            printf("Command: 0x03, sector: %02x \r\n", sector);
            floppy_drives[active_drive].sector = sector;
        } else if (command == 4){
            // set active track
            uint8_t track = uart_getc(UART_INTRA);
            printf("Command: 0x04, track: %02x \r\n", track);
            floppy_drives[active_drive].track = track;
        } else if (command == 6) {
            // write data to active drive
            length = uart_getc(UART_INTRA);
            printf("Command: 0x06, length: %02x \r\n", length);
            while (index < length) {
                buffer[index++] = uart_getc(UART_INTRA);
            }
            floppy_write_sector(buffer);
        } else if (command == 7) {
            // read sector from active drive
            length = uart_getc(UART_INTRA);
            printf("Command: 0x07, length: %02x \r\n", length);
            floppy_read_sector();

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