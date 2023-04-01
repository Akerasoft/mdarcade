/* Arcade controller for Genesis/Megadrive
 * Copyright (C) 2022-2023 Akerasoft
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
#include "controller.h"

#define GAMEPAD_BYTES	2

/*********** prototypes *************/
static char controllerUpdate(void);

// the most recent bytes we fetched from the controller
static unsigned char last_read_controller_bytes[GAMEPAD_BYTES];

/*
 *
        PIN             Button Reported
        ===========     ===============
BYTE 0
        PB0             MD B            D0
        PB1             MD X            D1
        PB2             MD MODE         D2
        PB3             MD START        D3
        PB4             MD UP           D4
        PB5             MD DOWN         D5
        NA              none (zero)     D6
        NA              none (zero)     D7

BYTE 1
        PD0             MD LEFT         D0
        PD1             MD RIGHT        D1
        N/A             none (zero)     D2
        PD3             MD A            D3
        PD4             MD C            D4
        PD5             MD Y            D5
        PD6             MD Z            D6
        NA              none (zero)     D7
 *
 */

static char controllerUpdate(void)
{
	last_read_controller_bytes[0] = (~PINB) & 0x3F;
	last_read_controller_bytes[1] = (~PIND) & 0x7B;

	return 0;
}

static void controllerGetReport(gamepad_data *dst)
{
	unsigned char h, l;
	
	if (dst != NULL)
	{
		l = last_read_controller_bytes[0];
		h = last_read_controller_bytes[1];

		dst->controller.buttons = l;
		dst->controller.buttons |= h<<8;
	}
}

static Gamepad ControllerGamepad = {
	.update		= controllerUpdate,
	.getReport	= controllerGetReport
};

Gamepad *controllerGetGamepad(void)
{
	return &ControllerGamepad;
}