#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <avr/wdt.h>
#include <avr/sleep.h>
#include "uart.h"
#include "stdlib.h"

//currently changes target timing and serial reports
#define dg_printf(...) do{uartEnable(); _delay_ms(100);printf(__VA_ARGS__ ); _delay_ms(100);uartDisable();}while(0);
#define debugx(x) dg_printf(#x "=0x%x\r\n",(unsigned int)(x));
#define debug(x) dg_printf(#x "=%d\r\n",(unsigned int)(x));




#define wdt_reset() __asm__ __volatile__ ("wdr")
inline static __attribute__((always_inline)) void wdt_disable() {
    MCUSR = 0;
    CCP = 0xD8;
    WDTCSR = 0;
    wdt_reset();
}


#define DEBOUNCE_MS 100

#define SW_DRV_SEAT PA0
#define SW_PAS_SEAT PA1
#define SW_LIGHTS   PA2
#define SW_LOCK_F   PA3
#define SW_LOCK_R   PA4
#define OUT_DRV_SEAT PA6
#define OUT_PAS_SEAT PA7

#define OUT_LIGHTS   PB0
#define OUT_LOCK_F   PB1
#define OUT_LOCK_R   PB2

void set_pin_modes(){
    PUEA =  _BV(SW_DRV_SEAT) | _BV(SW_PAS_SEAT) | _BV(SW_LIGHTS) | _BV(SW_LOCK_F) | _BV(SW_LOCK_R);
    DDRA = _BV(OUT_DRV_SEAT) | _BV(OUT_PAS_SEAT);
    DDRB = _BV(OUT_LIGHTS) | _BV(OUT_LOCK_F) | _BV(OUT_LOCK_R);
}
typedef union{
    struct {
        uint8_t drv_seat : 1;
	uint8_t pas_seat :1;
	uint8_t lights:1;
	uint8_t lock_f:1;
	uint8_t lock_r:1;
    };
    uint8_t as_int;
}switches_t;

switches_t get_sw(){
    switches_t ret;
    ret.drv_seat = !!(PINA & _BV(SW_DRV_SEAT));
    ret.pas_seat = !!(PINA & _BV(SW_PAS_SEAT));
    ret.lights   = !!(PINA & _BV(SW_LIGHTS));
    ret.lock_f   = !!(PINA & _BV(SW_LOCK_F));
    ret.lock_r   = !!(PINA & _BV(SW_LOCK_R));
    return ret;
}

void set_outputs(switches_t state){
    uint8_t old_a = PORTA & ~(_BV(OUT_DRV_SEAT)  | _BV(OUT_PAS_SEAT));
    PORTA = old_a | ( state.drv_seat << OUT_DRV_SEAT) | (state.pas_seat << OUT_PAS_SEAT);

    uint8_t old_b = PORTB & ~(_BV(OUT_LIGHTS)  | _BV(OUT_LOCK_F) | _BV(OUT_LOCK_R));
    PORTB = old_b | ( state.lights << OUT_LIGHTS) | (state.lock_f << OUT_LOCK_F)| (state.lock_r << OUT_LOCK_R);

}
#
int main (void) {
// MAIN STARTUP CODE:

    wdt_disable();
    sei();
    int i=0;
    set_pin_modes();

    switches_t state;
    state.as_int=0;
    set_outputs(state);
    switches_t last_sw=get_sw();
    int32_t loop_count=0;
    while (1) {
	switches_t sw= get_sw();

	//look for falling edges
	if(last_sw.drv_seat && !sw.drv_seat ){
	    dg_printf("drv_seat pressed\r\n");
	    state.drv_seat = !state.drv_seat;
	}

	if(last_sw.pas_seat && !sw.pas_seat ){
	    dg_printf("pas_seat pressed\r\n");
	    state.pas_seat = !state.pas_seat;
	}

	if(last_sw.lights && !sw.lights ){
	    dg_printf("lights pressed\r\n");
	    state.lights = !state.lights;
	}
	if(last_sw.lock_f && !sw.lock_f ){
	    dg_printf("lock_f pressed\r\n");
	    state.lock_f = !state.lock_f;
	}
	if(last_sw.lock_r && !sw.lock_r ){
	    dg_printf("lock_r pressed\r\n");
	    state.lock_r = !state.lock_r;
	}

	last_sw = sw;
	set_outputs(state);
	_delay_ms(1);
	loop_count++;
	if((loop_count&1023) == 0){
	    dg_printf("hello world %d %x %x\r\n",i++,state.as_int,get_sw().as_int);
	}

    }

}
