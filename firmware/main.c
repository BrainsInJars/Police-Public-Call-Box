/*
Name: main.c
Author: Kevin Loney (kevin.loney@brainsinjars.com)
Copyright: 2013
Description: Basic control code for version 3.5 of the Police Public Call Box PCB

License:
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>

volatile uint8_t active = 0;

volatile unsigned long milliseconds = 0;

const unsigned long time_debounce = 200;
const unsigned long time_charge = 2000;
const unsigned long time_discharge = 2000;
const unsigned long time_on = 20000;

void setup(void);
void loop(void);
void snooze(void);

/*
PB0 - Roof Lantern (PWM)
PB1 - Unused
PB2 - Resistive Touch Input (AIN1)
PB3 - Unused
PB4 - Interior Lights
PB5 - Unused (RESET)
*/

void setup(void) {
	cli();
	power_all_disable();

	// Digital Configuration
	// Inputs
	DDRB &= ~0x3f; // Make all pins inputs
	PORTB |= 0x3f; // Enable all pull-ups

	// Outputs
	PORTB &= ~(_BV(PORTB0) | _BV(PORTB4)); // Make all outputs low
	DDRB |= _BV(DDB0) | _BV(DDB4); // Enable output pins

	// PWM Configuration
	power_timer0_enable();
	TCCR0A |= _BV(WGM01) | _BV(WGM00); // PB0 = fast non-inverting PWM
	TCCR0B |= _BV(CS00); // Activate
	OCR0A = 0xff;
	power_timer0_disable();

	// Time configuration
	power_timer1_enable();
	TCCR1 = _BV(CTC1) | (7 << CS10);	// CTC  mode, div64 
	OCR1C = 0.001 * F_CPU/64 - 1;		// 1ms, F_CPU @16MHz, div64 
	TIMSK |= _BV(OCIE1A);
	power_timer1_disable();

	// External Interrupt
	GIMSK |= _BV(INT0);
	MCUCR &= ~(_BV(ISC01) | _BV(ISC00));

	sei();
}

ISR(INT0_vect, ISR_NAKED) {
	reti();
}

ISR(TIMER1_COMPA_vect) { 
	++milliseconds;
}

void loop(void) {
	uint8_t state = 0, intensity = 0;
	unsigned long start = milliseconds, finish = start + time_charge, delta;

	power_timer0_enable();
	power_timer1_enable();

	PORTB |= _BV(PORTB4); // Turn on interior lights

	OCR0A = 0xFF;
	TCCR0A |= _BV(COM0A1) | _BV(COM0A0); // Enable PWM output on PB0

	while(milliseconds < time_on) {
		unsigned long now = milliseconds;

		if(now > finish) {
			state = !state;
			start = finish;
			delta = (state ? time_discharge : time_charge);
			finish = start + delta;
		}

		intensity = 0xff * (now - start) / delta;
		if(!state) {
			intensity = 0xff - intensity;
		}

		OCR0A = intensity;
	}

	TCCR0A &= ~(_BV(COM0A1) | _BV(COM0A0)); // Disable PWM output on PB0
	PORTB &= ~_BV(PORTB4); // Turn off interior lights

	power_timer1_disable();
	power_timer0_disable();
}

void snooze(void) {
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	sleep_mode();
	sleep_disable();
}

int main(void) {
	setup();

    for(;;) {
		snooze();
		loop();
    }
    return 0;
}
