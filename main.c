/* Arcade controller for Genesis/Megadrive
 * Copyright (C) 2022 Akerasoft based on work by Raphël Assénat
 * Original work remains Copyright Raphël Assénat changes
 * are Copyright by Akerasoft.
 *
 * Original work: Snes controller to Genesis/Megadrive adapter
 * Copyright (C) 2013-2016 Raphël Assénat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The original author may be contacted at raph@raphnet.net
 * The author of changes may be contacted at robert.kolski@akerasoft.com
 */
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "controller.h"

static void hwinit(void)
{
	/* PORTB
	 *
	 * Bit	Function	Dir		Level/pu
	 * 0	B Button	in		1
	 * 1	X Button	in		1
	 * 2	Mode Button	in		1
	 * 3	Start Btn   in		1
	 * 4	UP Button	in		1
	 * 5	DOWN Button	in		1
	 * 6	XTAL1
	 * 7	XTAL2
	 */
	DDRB = 0x00;
	PORTB = 0x3F;

	/* PORTC
	 *
	 * Bit	Function	Dir		Level/pull-up
	 * 0	START/C		In		1
	 * 1	A/B			In		1
	 * 2	0/RT/MODE	In		1
	 * 3	0/LF/X		In		1
	 * 4	DN/DN/Y		In		1
	 * 5	UP/UP/Z		In		1
	 */
	DDRC = 0x00;
	PORTC = 0xFF;

	/* PORTD
	 *
	 * Bit	Function	Dir		Level/pull-up
	 * 0	Left Button	in		1
	 * 1	Right Btn   in		1
	 * 2	SELECT		In		0 - 1* see note (add pull down resistor)
	 * 3	A Button	in		1
	 * 4	C Button	In		1
	 * 5	Y Button	In		1
	 * 6	Z Button	In		1
	 * 7	SW 3/6		In		0
	 */
	DDRD = 0x00;
	PORTD = 0x7B; // see note

	// For the Akerasoft project a 10k-ohm pull down resistor is used
	// at PORTD pin 2.

	// Note: Games like GoldenAxe II let the SELECT line float for a moment. With
	// the internal pull-up, the signal rises slowly and sometimes is seen as a
	// logic 1, but just barely. Depending on which buttons are pressed on the
	// controller, the ramping up of the SELECT signal is not always smooth, and
	// this is seen as a series of transition by the adapter which updates the
	// buttons.
	//
	//
	//       No buttons pressed            Buttons pressed
	// 1       ___        _             ___          _
	//        |   |      | |           |   |    , , | |
	//        |   |   /| | |           |   |   /|/| | |
	// 0 _____|   |__/ |_| |____   ____|   |__/   |_| |___
	//
	//              ^^^^                      ^^^^
	//
	// Disabling the internal pullup seems to help, but ideally an external pull-down
	// resistor should be added. Goldenaxe II is known to misbehave with 6 button
	// controllers. This is solved by forcing 3 button mode (by holding MODE at power
	// up).
	//
	// Interestingly, the adapter does not need to be in compatibility mode for this
	// game. Maybe the behaviour shown above is what cause problems with real 6
	// button controllers?
}

volatile uint8_t mddata[8] = { 0xff,0xf3,0xff,0xf3,0xff,0xc3,0xff,0xff };
volatile uint8_t dat_pos;
volatile uint8_t polled;

void fastint(void) __attribute__((naked)) __attribute__((section(".boot")));

void fastint(void)
{
asm volatile(
		"	nop\nnop\n					\n" // VECTOR 1 : RESET

#if defined(__AVR_ATmega168__)
		// PORTC address is different.
		// And we can only use the first 64 registers
		// with in/out instructions. So ICR1L was not available.
		"	in __zero_reg__, 0x27 	; OCR0A		\n"
		"	out 0x08, __zero_reg__	; PORTC		\n"
#elif defined(__AVR_ATmega88PA__)
		// PORTC address is different.
		// And we can only use the first 64 registers
		// with in/out instructions. So ICR1L was not available.
		"	in __zero_reg__, 0x27 	; OCR0A		\n"
		"	out 0x08, __zero_reg__	; PORTC		\n"
#elif defined(__AVR_ATmega8__)
		"	in __zero_reg__, 0x26 	; ICR1L		\n"
		"	out 0x15, __zero_reg__	; PORTC		\n"
#else
#error MCU not supported yet
#endif

		// Now, let's prepare for the next transition...

		"	push r16					\n"
		"	push r30					\n"
		"	push r31					\n"

		"	in r16, __SREG__			\n"
		"	push r16					\n"

		// notify main loop
		"	ldi r16, 1					\n"
		"	sts polled, r16				\n"

		// manage pointer
		"	lds r16, dat_pos			\n"	// previous index (now on PORT)
		"	inc r16						\n" // next index
		"	andi r16, 0x07				\n" // 0-7
		"	sts dat_pos, r16			\n" // save next index

		"	ldi r30, lo8(mddata)		\n" // get mdata base adress into Z
		"	ldi r31, hi8(mddata)		\n"

		"	add r30, r16				\n" // Add current index to Z
		"	ldi r16, 0					\n"
		"	adc r31, r16				\n"

		"	ld r16, Z					\n"

#if defined(__AVR_ATmega168__)
		"	out 0x27, r16			;	 OCR0A		\n" // next value
#elif defined(__AVR_ATmega88PA__)
		"	out 0x27, r16			;	 OCR0A		\n" // next value
#elif defined(__AVR_ATmega8__)
		"	out 0x26, r16			;	 ICR1L		\n" // next value
#else
#error MCU not supported yet
#endif

		"	clr __zero_reg__			\n" // clear zero reg

		"	pop r16						\n"
		"	out __SREG__, r16			\n"

		"	pop r31						\n" // r31
		"	pop r30						\n" // r30
		"	pop r16						\n" // r16
		"	reti						\n"
	::);
}


EMPTY_INTERRUPT(INT0_vect);

#define OPT_TURBO			1	/* This button is turbo */
#define OPT_TURBO_SEL_SPEED	2	/* This button cycles through turbo speeds */

struct controller_md_map {
	uint16_t controller_btn;
	uint8_t s[3]; // [0] SELECT LOW STATE, [1] SELECT HIGH STATE, [2] 3RD HIGH STATE
	char opts;
};

#define DB9_NULL			{0x00, 0x00, 0x00 }

#define GEN_BTN_A			{0x02, 0x00, 0x00 }
#define GEN_BTN_B			{0x00, 0x02, 0x00 }
#define GEN_BTN_C			{0x00, 0x01, 0x00 }
#define GEN_BTN_X			{0x00, 0x00, 0x08 }
#define GEN_BTN_Y			{0x00, 0x00, 0x10 }
#define GEN_BTN_Z			{0x00, 0x00, 0x20 }
#define GEN_BTN_START		{0x01, 0x00, 0x00 }
#define GEN_BTN_MODE		{0x00, 0x00, 0x04 }
#define GEN_BTN_DPAD_UP		{0x20, 0x20, 0x00 }
#define GEN_BTN_DPAD_DOWN	{0x10, 0x10, 0x00 }
#define GEN_BTN_DPAD_LEFT	{0x00, 0x08, 0x00 }
#define GEN_BTN_DPAD_RIGHT	{0x00, 0x04, 0x00 }

struct controller_md_map md_controller[] = {

	{ CONTROLLER_BTN_A,			GEN_BTN_A },
	{ CONTROLLER_BTN_B, 		GEN_BTN_B },
	{ CONTROLLER_BTN_C,			GEN_BTN_C },
	{ CONTROLLER_BTN_X,			GEN_BTN_X },
	{ CONTROLLER_BTN_Y,			GEN_BTN_Y },
	{ CONTROLLER_BTN_Z,			GEN_BTN_Z },
	{ CONTROLLER_BTN_START,		GEN_BTN_START },
	{ CONTROLLER_BTN_MODE,		GEN_BTN_MODE },
	{ CONTROLLER_BTN_DPAD_UP,	GEN_BTN_DPAD_UP },
	{ CONTROLLER_BTN_DPAD_DOWN,	GEN_BTN_DPAD_DOWN },
	{ CONTROLLER_BTN_DPAD_LEFT,	GEN_BTN_DPAD_LEFT },
	{ CONTROLLER_BTN_DPAD_RIGHT,	GEN_BTN_DPAD_RIGHT },
	{ 0, }, /* CONTROLLER btns == 0 termination. */
};

#define TURBO_SPEED_30		0 // 60 / 2
#define TURBO_SPEED_25		1 // 50 / 2
#define TURBO_SPEED_20		2 // 60 / 3
#define TURBO_SPEED_16		3 // 50 / 3
#define TURBO_SPEED_15		4 // 60 / 4
#define TURBO_SPEED_12_5	5 // 50 / 4

static uint8_t turbo_speed = TURBO_SPEED_25;

static void turboCycleSpeed(uint8_t btn_state)
{
	static uint8_t last_state;

	if (btn_state && !last_state) {
		turbo_speed++;
		if (turbo_speed > TURBO_SPEED_12_5) {
			turbo_speed = TURBO_SPEED_30;
		}
	}

	last_state = btn_state;
}

static uint8_t turbo_lock_on = 0;

static uint8_t turboGetTop(void)
{
	switch (turbo_speed)
	{
			// divide by 2
		default:
		case TURBO_SPEED_30:
		case TURBO_SPEED_25:
			return 2;

			// divide by 3
		case TURBO_SPEED_20:
		case TURBO_SPEED_16:
			return 3;

			// divide by 4
		case TURBO_SPEED_15:
		case TURBO_SPEED_12_5:
			return 4;
	}
}

static uint8_t turbo_state=0;

static void turboDo(void)
{
	static char turbo_count = 0;

	if (turbo_count <= 0) {
		turbo_count = turboGetTop();
		turbo_state = !turbo_state;
	}
	turbo_count--;
}

int main(void)
{
	gamepad_data last_data;
	uint8_t next_data[8];
	Gamepad *controllerpad;
	char ignore_buttons = 1;
	char tribtn_compat = 0;
	char genesis_polling = 0;
	char sw_3_6_btn = 0;

	hwinit();

	if (PIND & (1<<PIND2)) {
		dat_pos = 1;
		PORTC = mddata[0];
	} else {
		dat_pos = 0;
	}
	
	dat_pos = 1;
	
	if (PIND & (1<<PIND7))
	{
		sw_3_6_btn = 1;
	}
	else
	{
		sw_3_6_btn = 0;
	}

	_delay_ms(20);

	controllerpad = controllerGetGamepad();
	controllerpad->update();
	controllerpad->getReport(&last_data);

	// Push-pull drive for Genesis
	DDRC = 0xFF;
	PORTC = 0xFf;

	// If MODE is down, enable 3 button compatibility mode
	if (last_data.controller.buttons & CONTROLLER_BTN_MODE) {
		tribtn_compat = 1;
	}
		
	if (sw_3_6_btn) {
		tribtn_compat = 1;
	}

	// setup SELECT external interrupt
	//
#if defined(__AVR_ATmega8__)
	// Move the vector to the bootloader section where we have direct code for
	// INT0.
	GICR = (1<<IVCE);
	GICR = (1<<IVSEL);

	//
	MCUCR |= (1<<ISC00); // Any change generates an interrupt
	MCUCR &= ~(1<<ISC01);
	GICR |= (1<<INT0);
#elif defined(__AVR_ATmega168__)
	/* Move the vector to the bootloader section where we have direct code for INT0. */
	MCUCR = (1<<IVCE);
	MCUCR = (1<<IVSEL);

	/* Any changes triggers INT0 */
	EICRA = (1<<ISC00);
	EIMSK = (1<<INT0);
#elif defined(__AVR_ATmega88PA__)
	/* Move the vector to the bootloader section where we have direct code for INT0. */
	MCUCR = (1<<IVCE);
	MCUCR = (1<<IVSEL);

	/* Any changes triggers INT0 */
	EICRA = (1<<ISC00);
	EIMSK = (1<<INT0);
#else
#error MCU not supported
#endif

	// Interrupts are only used by the Genesis mode
	sei();

	while(1)
	{
		struct controller_md_map *map;
		uint8_t sel_low_dat, sel_high_dat, sel_x_dat;
		int i;
		
		if (PIND & (1<<PIND7))
		{
			// if the switch was off but now it is on
			// then enable 3 button mode
			if (!sw_3_6_btn)
			{
				sw_3_6_btn = 1;
				tribtn_compat = 1;
			}
		}
		else
		{
			// if the switch was off but
			// mode was pressed at start up, then keep 3 button mode
			// e.g. no code here

			// if the switch was on but now it is off
			// then enable 6 button mode
			if (sw_3_6_btn)
			{
				sw_3_6_btn = 0;
				tribtn_compat = 0;
			}
		}

		if (!genesis_polling)
		{
			_delay_ms(15);
			controllerpad->update();
			controllerpad->getReport(&last_data);

			memcpy((void*)mddata, next_data, 8);
			if (PIND & (1<<PIND2)) {
				dat_pos = 1;
				PORTC = mddata[0];
			} else {
				PORTC = mddata[1];
				dat_pos = 0;
			}

#if defined(__AVR_ATmega168__)
			OCR0A = mddata[dat_pos];
#elif defined(__AVR_ATmega88PA__)
			OCR0A = mddata[dat_pos];
#elif defined(__AVR_ATmega8__)
			ICR1L = mddata[dat_pos];
#else
#error MCU not supported yet
#endif
			if (polled) {
				genesis_polling = 1;
			}
		}
		else
		{
			char c = 0;
			// Genesis mode
			polled = 0;
			while (!polled)
			{
				c++;
				_delay_ms(1);
				if (c > 100)
				{
					// After 100ms, fall back to self-timed mode (for SMS games)
					genesis_polling = 0;
					break;
				}
			}
			_delay_ms(1.5);

			// Timeout from the 6button mode
			memcpy((void*)mddata, next_data, 8);
			if (PIND & (1<<PIND2))
			{
				dat_pos = 1;
				PORTC = mddata[0];
			}
			else
			{
				PORTC = mddata[1];
				dat_pos = 0;
			}

#if defined(__AVR_ATmega168__)
			OCR0A = mddata[dat_pos];
#elif defined(__AVR_ATmega88PA__)
			OCR0A = mddata[dat_pos];
#elif defined(__AVR_ATmega8__)
			ICR1L = mddata[dat_pos];
#else
#error MCU not supported yet
#endif
			controllerpad->update();
			controllerpad->getReport(&last_data);
		}

		turboDo();

		sel_low_dat = 0;
		sel_high_dat = 0;
		sel_x_dat = 0;

		map = md_controller;
		while (map->controller_btn) {
			// To prevent buttons pressed at powerup (for mappings) from confusing
			// controller detection, buttons are reported as idle until we first
			// see a 'no button pressed' state.
			if (ignore_buttons && (0 == (last_data.controller.buttons & CONTROLLER_BTN_ALL))) {
				ignore_buttons = 0;
			}

			if (map->opts&OPT_TURBO) {
				if (turbo_lock_on) {
					if (turbo_state && !ignore_buttons) {
						sel_low_dat |= map->s[0];
						sel_high_dat |= map->s[1];
						sel_x_dat |= map->s[2];
					}
				}
			}

			if ((last_data.controller.buttons & map->controller_btn))
			{
				if (!(map->opts&OPT_TURBO) || turbo_state) {
					if (!ignore_buttons) {
						sel_low_dat |= map->s[0];
						sel_high_dat |= map->s[1];
						sel_x_dat |= map->s[2];
					}
				}
			}

			if ((map->opts&OPT_TURBO_SEL_SPEED)) {
				turboCycleSpeed(last_data.controller.buttons & map->controller_btn);
			}

			map++;
		}

		if (tribtn_compat) {
			next_data[0] = sel_high_dat;
			next_data[1] = sel_low_dat | 0x0c;
			next_data[2] = sel_high_dat;
			next_data[3] = sel_low_dat | 0x0c;
			next_data[4] = sel_high_dat;
			next_data[5] = sel_low_dat | 0x0c;
			next_data[6] = sel_high_dat;
			next_data[7] = sel_low_dat | 0x0c;
		}
		else {
			next_data[0] = sel_high_dat;
			next_data[1] = sel_low_dat | 0x0c; // Force right/left to 0 for detection
			next_data[2] = sel_high_dat;
			next_data[3] = sel_low_dat | 0x0c; // Force right/left to 0 for detection
			next_data[4] = sel_high_dat;
			next_data[5] = sel_low_dat | 0x3c; // FORCE UP/Dn/lef/right to 0 for detection.
			next_data[6] = sel_x_dat;
			next_data[7] = sel_high_dat & 0x03; // Keep Up/Dn/Left/Right high for detection.
		}

		for (i=0; i<8; i++) {
			next_data[i] ^= 0xff;
		}
	}

	return 0;
}
