#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_IO uart0                // Console
#define BAUD_RATE 115200
#define UART_IO_TX_PIN 0
#define UART_IO_RX_PIN 1

#define UART_INTRA uart1              // intra-pi communication
#define INTRA_BAUD_RATE 921600
#define UART_INTRA_TX_PIN 8
#define UART_INTRA_RX_PIN 9

#define LED_PIN 25

bool data_expected = false;

// RX interrupt handler:
void on_uart_rx() {
    while (uart_is_readable(UART_IO)) {
        uint8_t ch = uart_getc(UART_IO);
        printf("%c", ch);
    }
}

void on_uart_intra_rx() {

    char buffer[256];
    uint8_t index = 0;

    while (uart_is_readable(UART_INTRA)) {
        buffer[index++] = uart_getc(UART_INTRA);
    }
    
    printf("Received %d bytes \n\r", index);
    if (data_expected) {
        buffer[index++] = '\r'; // just for test
        buffer[index++] = '\n'; // just for test
        uart_write_blocking(UART_IO, buffer, index);
        data_expected = false;
    }

    if (index > 0 && buffer[0] == 0) {
        printf("Received 0x00, sending confirmation\n\r");
        uart_putc_raw(UART_INTRA, 0xFF);
        data_expected = true;
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

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);



        gpio_put(LED_PIN, 0);
        sleep_ms(250);

        //
    }
}
