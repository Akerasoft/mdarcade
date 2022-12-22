/* Arcade controller for Genesis/Megadrive
 * Copyright (C) 2022 Akerasoft
 * Developer: Robert Kolski
 * Changes are Copyright Akerasoft, original work Copyright by Raphaël Assénat
 *
 * Snes controller to Genesis/Megadrive adapter
 * Copyright (C) 2013-2016 Raphël Assénat
 *
 * This file is based on earlier work:
 *
 * Nes/Snes/N64/Gamecube to Wiimote
 * Copyright (C) 2012 Raphaël Assénat
 *
 * Nes/Snes/Genesis/SMS/Atari to USB
 * Copyright (C) 2006-2007 Raphaël Assénat
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
 * The author may be contacted at raph@raphnet.net
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "gamepads.h"
#include "snes.h"

#define GAMEPAD_BYTES	2

/*********** prototypes *************/
static char snesInit(void);
static char snesUpdate(void);


// the most recent bytes we fetched from the controller
static unsigned char last_read_controller_bytes[GAMEPAD_BYTES];
// the most recently reported bytes
static unsigned char last_reported_controller_bytes[GAMEPAD_BYTES];

static char nes_mode = 0;

static char snesInit(void)
{
	unsigned char sreg;
	sreg = SREG;
	cli();
	
	// do not alter PB6 and PB7, change PB0 to PB6 input
	DDRB &= 0xC0;
	
	// do not alter PB6 and PB7, change PB0 to PB6 to pullpup resistor enabled
	PORTB |= 0x3F;

	// do not alter PD2 and PD7, change PD0, PD1 and PD3 to PB6 input
	DDRD &= 0x84;
	
	// do not alter PD2 and PD7, change PD0, PD1 and PD3 to PB6 to pullpup resistor enabled
	PORTD |= 0x7B;

	snesUpdate();

	SREG = sreg;

	return 0;
}


/*
 *
        PIN             Button Reported
        ===========     ===============
BYTE 0
        PB0             B                  - MD B
        PB1             Y                  - MD X
        PB2             Select             - MD MODE
        PB3             Start              - MD START
        PB4             Up on joypad       - MD UP
        PB5             Down on joypad     - MD DOWN
        PD0             Left on joypad     - MD LEFT
        PD1             Right on joypad    - MD RIGHT
BYTE 1
        PD3             A                  - MD A
        PD4             X                  - MD C
        PD5             L                  - MD Y
        PD6             R                  - MD Z
        NA              none (always high) (zero)
        NA              none (always high) (zero)
        NA              none (always high) (zero)
        NA              none (always high) (zero)
 *
 */

static char snesUpdate(void)
{
	int i;
	unsigned char tmp=0;

	for (i = 0; i < 6; i++)
	{
		tmp <<= 1;
		if (!(PINB & (1<<i)))
		{
			tmp |= 0x01;
		}
	}
	for (i = 0; i < 2; i++)
	{
		tmp <<= 1;
		if (!(PIND & (1<<i)))
		{
			tmp |= 0x01;
		}
	}
	last_read_controller_bytes[0] = tmp;
	for (i = 3; i < 7; i++)
	{
		tmp <<= 1;
		if (!(PIND & (1<<i)))
		{
			tmp |= 0x01;
		}
	}
	tmp <<= 4;
	last_read_controller_bytes[1] = tmp;
	
	// simulate the delay of the original code
	_delay_us(204);

	return 0;
}

static char snesChanged(void)
{
	return memcmp(last_read_controller_bytes, 
					last_reported_controller_bytes, GAMEPAD_BYTES);
}

static void snesGetReport(gamepad_data *dst)
{
	unsigned char h, l;
	
	if (dst != NULL)
	{
		l = last_read_controller_bytes[0];
		h = last_read_controller_bytes[1];


		// The 4 last bits are always high if an SNES controller
		// is connected. With a NES controller, they are low.
		// (High on the wire is a 0 here due to the snesUpdate() implementation)
		// 
		if ((h & 0x0f) == 0x0f) {
			nes_mode = 1;
		} else {
			nes_mode = 0;
		}

		if (nes_mode) {
			// Nes controllers send the data in this order:
			// A B Sel St U D L R
			dst->nes.pad_type = PAD_TYPE_NES;
			dst->nes.buttons = l;
			dst->nes.raw_data[0] = l;
		} else {
			dst->nes.pad_type = PAD_TYPE_SNES;
			dst->snes.buttons = l;
			dst->snes.buttons |= h<<8;
			dst->snes.raw_data[0] = l;
			dst->snes.raw_data[1] = h;
		}
	}
	memcpy(last_reported_controller_bytes, 
			last_read_controller_bytes, 
			GAMEPAD_BYTES);	
}

static Gamepad SnesGamepad = {
	.init		= snesInit,
	.update		= snesUpdate,
	.changed	= snesChanged,
	.getReport	= snesGetReport
};

Gamepad *snesGetGamepad(void)
{
	return &SnesGamepad;
}

