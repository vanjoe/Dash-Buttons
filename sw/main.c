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
//#define dg_printf(...) do{uartEnable(); _delay_ms(100);printf(__VA_ARGS__ ); _delay_ms(100);uartDisable();}while(0);
#define dg_printf(...)
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

#define SW_PAS_SEAT PA0
#define SW_LIGHTS   PA1
#define SW_LOCK_R   PA2
#define SW_LOCK_F   PA3
#define SW_DRV_SEAT PA4
#define BACKLIGHT_EN PA5
#define OUT_DRV_SEAT PA6
#define OUT_LOCK_F   PA7

#define OUT_PAS_SEAT PB0
#define OUT_LIGHTS   PB1
#define OUT_LOCK_R   PB2

void setup_pins(){
    PUEA = _BV(SW_DRV_SEAT) | _BV(SW_PAS_SEAT) | _BV(SW_LIGHTS) | _BV(SW_LOCK_F) | _BV(SW_LOCK_R);
    PUEB = 0;

    PORTA = _BV(BACKLIGHT_EN) | _BV(OUT_DRV_SEAT) | _BV(OUT_LOCK_F);
    PORTB = _BV(OUT_PAS_SEAT) | _BV(OUT_LIGHTS) | _BV(OUT_LOCK_R);
    DDRA = _BV(BACKLIGHT_EN) | _BV(OUT_DRV_SEAT) | _BV(OUT_LOCK_F);
    DDRB = _BV(OUT_PAS_SEAT) | _BV(OUT_LIGHTS) | _BV(OUT_LOCK_R);

}
typedef union{
 struct {
    uint8_t drv_seat ;
    uint8_t pas_seat ;
    uint8_t lights;
    uint8_t lock_f;
    uint8_t lock_r;
 };
 uint8_t as_array[5];
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

    PORTA = ( state.drv_seat << OUT_DRV_SEAT) | (state.lock_f << OUT_LOCK_F)| _BV(BACKLIGHT_EN);
    PORTB = ( state.lights << OUT_LIGHTS) | (state.pas_seat << OUT_PAS_SEAT)| (state.lock_r << OUT_LOCK_R) ;

}
void delay_us(int us){

}
void pwm_delayms(int ms,int dutycycle){
    static const int loops_per_ms=10;
    if(dutycycle>100)dutycycle=100;
    int inv_duty=100-dutycycle;
    for(int m =0;m<ms;++m){
	//debug(m);
	for(int i=0;i<loops_per_ms;++i){
	    //debug(i);
	    PORTA|=_BV(BACKLIGHT_EN);
	    for(int d=0;d<dutycycle;++d)_delay_us(1);
            PORTA &= ~_BV(BACKLIGHT_EN);
	    for(int d=0;d<inv_duty;++d)_delay_us(1);
	}
    }
}
int set_backlight(int en){
    if(en){
	PORTA|=_BV(BACKLIGHT_EN);
    }else{
	PORTA &= ~_BV(BACKLIGHT_EN);
    }
}
void blink_slow(int blinks){
    while(blinks--){
	set_backlight(0);
	_delay_ms(500);
	set_backlight(1);
	_delay_ms(500);
    }
}
void blink_fast(int blinks){
    while(blinks--){
	set_backlight(0);
	_delay_ms(100);
	set_backlight(1);
	_delay_ms(100);
    }
}

void pwm_test(){
    int pwm = 10;
    int test=1000;
    while(--test){
	//dg_printf("pwm = %d\r\n",pwm);
	pwm_delayms(1000,pwm);
	pwm+=10;
	if (pwm > 100){
	    pwm = 00;
	}
    }
    while(1){
	dg_printf("done_test\n");
	_delay_ms(1000);
    }
}
#define DEBOUNCE_COUNT 255

void debounce_falling(uint8_t* debounce,uint8_t sw){
    if(sw) {
	*debounce = DEBOUNCE_COUNT;
    } else if(*debounce){
	*debounce = *debounce-1;
    }
}
int main (void) {
// MAIN STARTUP CODE:

    wdt_disable();
    sei();

    setup_pins();

    switches_t state = {0};
    set_outputs(state);
    switches_t last_debounce_count={0};
    switches_t debounce_count = {0};

    //pwm_test();
    //blink_slow(4);
    blink_fast(4);

    while (1) {
	switches_t sw= get_sw();

	for(int s=0;s<sizeof(sw.as_array);++s){
	    //debounce look for falling edges
	    debounce_falling(&debounce_count.as_array[s], sw.as_array[s]);
	    if(debounce_count.as_array[s] == 0 && last_debounce_count.as_array[s] == 1){
		state.as_array[s] = !state.as_array[s];
	    }
	}
	last_debounce_count = debounce_count;
	set_outputs(state);

    }
    return 0;
}
