/*  Changes to this file are copyright (C) 2023 Akerasoft
 *  Robert Kolski is the programmer for Akerasoft
 *
 *  It was based on work by Raphael Assenat
 *  (Extenmote and snes2md projects are where this is from).
 *  Those works are Copyright (C) 2012  Raphael Assenat <raph@raphnet.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _gamepad_h__
#define _gamepad_h__

#define CONTROLLER_RAW_SIZE		2

typedef struct _controller_pad_data {
	unsigned short buttons;
} controller_pad_data;

#define CONTROLLER_BTN_B		0x0001
#define CONTROLLER_BTN_X		0x0002
#define CONTROLLER_BTN_MODE		0x0004
#define CONTROLLER_BTN_START	0x0008
#define CONTROLLER_BTN_DPAD_UP	0x0010
#define CONTROLLER_BTN_DPAD_DOWN	0x0020
#define CONTROLLER_BTN_DPAD_LEFT	0x0100
#define CONTROLLER_BTN_DPAD_RIGHT	0x0200
#define CONTROLLER_BTN_A		0x0800
#define CONTROLLER_BTN_C		0x1000
#define CONTROLLER_BTN_Y		0x2000
#define CONTROLLER_BTN_Z		0x4000

#define CONTROLLER_BTN_ALL		0x7b3f

typedef struct _gamepad_data {
	controller_pad_data controller;
} gamepad_data;


typedef struct {
	char (*update)(void);
	void (*getReport)(gamepad_data *dst);
} Gamepad;

//#define IS_SIMULTANEOUS(x,mask)	(((x)&(mask)) == (mask))

#endif // _gamepad_h__
