/*
Name: main.c
Author: Kevin Loney (kevin.loney@brainsinjars.com)
Copyright: 2013
Description: Basic control code for version 3.6 of the Police Public Call Box PCB

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

#define LERP(v, l0, h0, l1, h1)	(((h1) - (l1)) * ((v) - (l0)) / ((h0) - (l0)) + (l1))

#define TIME_CHARGE			2000
#define TIME_DISCHARGE		2000

#define STEPS_CHARGE		200
#define STEPS_DISCHARGE		200

#define DELAY_CHARGE		(TIME_CHARGE / STEPS_CHARGE)
#define DELAY_DISCHARGE		(TIME_DISCHARGE / STEPS_DISCHARGE)

#define TOTAL_LOOPS			5

void setup(void);
void loop(void);
void snooze(void);

/*
PB0 - Resistive Touch Input (AIN0)
PB1 - Resistive Touch Level (AIN1)
PB2 - Interior Lights
PB3 - Unused
PB4 - Roof Lantern (PWM)
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
	PORTB &= ~(_BV(PORTB2) | _BV(PORTB4)); // Make all outputs low
	DDRB |= _BV(DDB2) | _BV(DDB4); // Enable output pins

	// Analog Comparator
	PORTB &= ~(_BV(PORTB0) | _BV(PORTB1)); // Disable pull-ups on AIN0 and AIN1
	ACSR = 0x00; // Enable the analog comparator

	ACSR |= _BV(ACIE); // Enable analog comparator interrupt
	ACSR |= _BV(ACIS1); // Trigger interrupt on falling edge

	ADCSRB &= ~(_BV(ACME)); // Use AIN1 (PB1) as the negative input

	// Time configuration
	power_timer1_enable();
	TCCR1 = 0<<PWM1A | 0<<COM1A0 | 1<<CS10;
	GTCCR = 1<<PWM1B | 2<<COM1B0;
	OCR1B = 0x00;
	power_timer1_disable();

	sei();
}

ISR(ANA_COMP_vect, ISR_NAKED) {
	reti();
}

void loop(void) {
	unsigned long loop, level;

	power_timer1_enable();

	PORTB |= _BV(PORTB2); // Turn on interior lights

	for(loop = 0; loop < TOTAL_LOOPS; ++loop) {
		for(level = 0; level < STEPS_CHARGE; ++level) {
			OCR1B = LERP(level, 0, STEPS_CHARGE, 0, 255) & 0xff;
			_delay_ms(DELAY_CHARGE);
		}
		for(level = 0; level < STEPS_DISCHARGE; ++level) {
			OCR1B = 0xff - LERP(level, 0, STEPS_DISCHARGE, 0, 255);
			_delay_ms(DELAY_DISCHARGE);
		}
	}

	OCR1B = 0x00;
	PORTB &= ~_BV(PORTB2); // Turn off interior lights

	power_timer1_disable();
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
