#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"

#include "hardware/uart.h"
#include "floppy/floppy.h"
#include "intra_uart.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"


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