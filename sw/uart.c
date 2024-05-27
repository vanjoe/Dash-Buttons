#include <avr/io.h>
#include <stdio.h>

#define BAUD 9600
#include <util/setbaud.h>

static int uart_putc(char b, FILE *stream);
FILE uart_output = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);

void uartEnable() {
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
    UCSR1B = _BV(TXEN1);
    UCSR1C = (3<<UCSZ10);
	stdout = &uart_output;
}

void uartDisable() {
    UCSR1B &= ~_BV(TXEN1);
}

static int uart_putc(char b, FILE *stream) {
    while ( !( UCSR1A & (1<<UDRE1)) );
    UDR1 = b;
    return 0;
}
