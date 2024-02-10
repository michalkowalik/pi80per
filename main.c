#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "floppy/floppy.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define UART_IO uart0                // Console
#define BAUD_RATE 115200
#define UART_IO_TX_PIN 0
#define UART_IO_RX_PIN 1

#define UART_INTRA uart1              // intra-pi communication
#define INTRA_BAUD_RATE 921600
#define UART_INTRA_TX_PIN 8
#define UART_INTRA_RX_PIN 9

#define LED_PIN 25


// RX interrupt handler:
void on_uart_rx() {
    while (uart_is_readable(UART_IO)) {
        uint8_t ch = uart_getc(UART_IO);
        printf("%c", ch);

        // send it to the other pi
        uint8_t uart_data[] = {0x01, ch};
        uart_write_blocking(UART_INTRA, uart_data, 2);
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

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uart_init(UART_IO, BAUD_RATE);
    gpio_set_function(UART_IO_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_IO_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);

    // for now -> for now it has no TX data!
    uart_set_irq_enables(UART_IO, true, false);


    uart_init(UART_INTRA, INTRA_BAUD_RATE);
    gpio_set_function(UART_INTRA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_INTRA_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART1_IRQ, on_uart_intra_rx);
    irq_set_enabled(UART1_IRQ, true);

    // for now -> for now it has no TX data!
    uart_set_irq_enables(UART_INTRA, true, false);

    sleep_ms(1000);
    printf("\r\nUARTs enabled\r\n");

    // init floppies
    floppy_init();


    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);


        gpio_put(LED_PIN, 0);
        sleep_ms(250);

        //
    }
}

#pragma clang diagnostic pop