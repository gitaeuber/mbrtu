/*
 * mbrtu modbus command line tool
 * Copyright © 2015 Lars Täuber
 *
 * This file is part of mbrtu.
 *
 * mbrtu is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mbrtu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with mbrtu; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#define MBRTU_VERSION		"0.3.3"

#include			<modbus.h>

#define MBRTU_SET_ADDR		0x01
#define MBRTU_SET_FUNC		0x02
#define MBRTU_SET_REG		0x04
#define MBRTU_SET_CNT		0x08


#define MBRTU_TYPE_UINT16	0x0001
#define MBRTU_TYPE_UINT32	0x0002
#define MBRTU_TYPE_UINT64	0x0003
#define MBRTU_TYPE_INT16	0x0004
#define MBRTU_TYPE_INT32	0x0005
#define MBRTU_TYPE_INT64	0x0006
#define MBRTU_TYPE_F16		0x0010
#define MBRTU_TYPE_F32_ABCD	0x0011
#define MBRTU_TYPE_F32_BADC	0x0012
#define MBRTU_TYPE_F32_CDAB	0x0013
#define MBRTU_TYPE_F32_DCBA	0x0014
#define MBRTU_TYPE_HEX		0x0100
#define MBRTU_TYPE_CHAR		0x0200


#define MBRTU_FLAGS_DEBUG	0x01
#define MBRTU_FLAGS_PARSEABLE	0x02
#define MBRTU_FLAGS_QUIET	0x04


#define IF_DEBUG		if (global_flags & MBRTU_FLAGS_DEBUG)
#define IF_N_QUIET		if (!(global_flags & MBRTU_FLAGS_QUIET))

uint8_t		global_flags = 0;

typedef struct {
    modbus_t		*ctx;
    char		*dev_file,
			 parity;
    uint8_t		 stopbits;
    int			 baudrate;
    uint16_t		 delay;
    long int		 timeout;
} mbrtu_conn;


typedef struct {
    uint8_t		addr,
			func;
    uint16_t		cnt,		/* bytes to be sent or received */
			reg,
			type,		/* type of value (int, float, char, ... ) */
			*data;
    char		*ntmp;
} mbrtu_call;

mbrtu_conn	*conn;
