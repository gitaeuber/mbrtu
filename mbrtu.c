/*
 * mbrtu modbus command line tool
 * Copyright © 2015 Lars Täuber
 *
 * This file is part of mbrtu.
 *
 * mbrtu is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include "mbrtu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>


static inline void show_help (void)
{
    printf ("This is mbrtu version " MBRTU_VERSION " - GPL 2.1 (c) 2015 Lars Täuber\n"
	    "gitaeuber@users.noreply.github.com\n"
	    "\n"
	    "mbrtu -d DEVICE [-b BAUDRATE] [-p PARITY] [-s STOPPBITS] [-D]\n"
	    "     [-a ADDR] [-f FUNC] [-t TYPE] [-n #] -r REG\n"
	    "    [[-a ADDR] [-f FUNC] [-t TYPE] [-n #] -r REG ...]\n"
	    "\n"
	    "\nThe following options need to be specified first:\n"
	    "\t-h\t\t\tPrint this help message.\n"
	    "\t-d serial device\t/dev/ttyUSB?\n"
	    "\t-b baud rate\t\t\tdefault: 9600\n"
	    "\t-p parity\t\tE|O|N\tdefault: even\n"
	    "\t-s number of stop bits\t[1|2]\tdefault: 1 for odd and even parity, 2 for no parity\n"
	    "\n"
	    "\nThe following options generate calls:\n"
	    "\t-a address of slave to ask 1-255\n"
	    "\t   0 for broadcast\n"
	    "\t   default: last one called or 1\n"
	    "\n"
	    "\t-f function to send\t[h|i|w|3|4|6|16]\n"
	    "\t   h = 3 = read holding registers\n"
	    "\t   i = 4 = read input registers\t\t(default)\n"
	    "\t   w = 6 = write single register\n"
	    "\t  16 = write multiple 16 bit registers\n"
	    "\t  default: last one called or 4\n"
	    "\n"
	    "\t-n\n"
	    "\t   either: number of 16 bit registers to read\n"
	    "\t   or:     data to be written to registers\n"
	    "\t      integers or floats (depending on type set with \"-t\")\n"
	    "\t      separated by colons (:)\n"
	    "\t           example: -n 3.8:0.4:-5.0:\n"
	    "\t           example: -n 3:0:-5:0x4b\n"
	    "\t      string\n"
	    "\t           example: -n \"This array of chars\"\n"
	    "\t   default: number of registers read/written last time\n"
	    "\t        or: data written last time or 0\n"
	    "\n"
	    "\t-r register to start the function at\n"
	    "\t   default: last register called or 0\n"
	    "\t   this option emits the call on the bus\n"
	    "\t   so all other call options (-a, -f, -n, -t) have to be set before\n"
	    "\n"
	    "\t-t type of values\thex\t(default)\n"
	    "\t               or\t[u]int[16|32|64]\n"
	    "\t               or\tchar\n"
	    "\t               or\tF32_[abcd|badc|cdab|dcba]\n"
	    "\n"
	    "\t-O timeout while waiting for answers in ms\n"
	    "\t               value has to be greater or equal to 500 (default)\n"
	    "\n"
	    "\t-D enable debugging\n"
	    "\t-Q quiet mode: disable debugging and don't write anything to stderr\n"
	    "\t-P format output easier to parse\n"
	    "\t-T set delay between modbus messages in [ms]\n"
	    "\n"
	    "You can change the parameters ADDR, FUNC, TYPE or NUMBER in between register\n"
	    "calls.\n"
	    "\n"
	    "\tExample:\n"
	    "\tmbrtu -b /dev/ttyUSB-485 -a1 -tchar -n1 -r0x1018 -r244 -thex -r0x1020\n"
	    "\n"
	    "\tThis makes 3 calls to different registers.\n"
	    "\tThe first two calls print the result as two 8 Bit chars.\n"
	    "\tThe last call prints the result as hex number.\n"
	    "\tThe registers don't need to be different.\n"
	    "\tYou also can read 3 times the same register.\n"
	    "\n");
    exit(0);
}



int parse_n_opt (mbrtu_call *call)
{
    char	*endptr,
		*buf,
		**nbuf = &call->ntmp;

    int		ret = 0;


/* read function: */
    if (call->func == 3 || call->func ==4 ) {
	call->cnt = (uint16_t) strtol (call->ntmp, &endptr, 0);

	if (*endptr == '\0')
	    return 0;
	else
	    return -1;
    }

/* write function */

    if (call->type == MBRTU_TYPE_CHAR) {
	/* count the number of values given to be sent */
	/* 2 chars are one 16bit value to be written: */
	call->cnt = (uint16_t) strlen (call->ntmp);
	call->cnt ++;
	call->cnt /= 2;

	if (NULL == (call->data = realloc (call->data, call->cnt * sizeof(uint16_t)))) {
	    IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
	    exit (-1);
	}

	for (int i = 0; i < call->cnt; i++)
	    call->data[i] = (call->ntmp[2*i] << 8) + call->ntmp[2*i+1];

    } else {

	call->cnt = 1;				/* count the number of values given to be sent */
	for (int i=0; call->ntmp[i]; i++)
	    if (call->ntmp[i] == ':')
		call->cnt++;

	switch (call->type) {
	    case MBRTU_TYPE_INT32:
	    case MBRTU_TYPE_UINT32:
	    case MBRTU_TYPE_F32_ABCD:
	    case MBRTU_TYPE_F32_BADC:
	    case MBRTU_TYPE_F32_CDAB:
	    case MBRTU_TYPE_F32_DCBA:
		call->cnt *= 2;			/* one value means two bytes to be sent */
		break;
	    case MBRTU_TYPE_INT64:
	    case MBRTU_TYPE_UINT64:
		call->cnt *= 4;			/* one value means four bytes to be sent */
		break;
	} /* switch */

	if (NULL == (call->data = realloc (call->data, call->cnt * sizeof(uint16_t)))) {
	    IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
	    exit (-1);
	}

	switch (call->type) {

	    case MBRTU_TYPE_UINT16:
	    case MBRTU_TYPE_INT16:
	    case MBRTU_TYPE_HEX:
		for (int i = 0; i < call->cnt; i++) {
		    buf = strsep (nbuf, ":");
		    if ( buf == NULL )			/* empty values equal to 0 */
			call->data[i] = 0;
		    else {
			if (call->type == MBRTU_TYPE_UINT16)
			    call->data[i] = (uint16_t) strtoul (buf, &endptr, 0);
			else
			    call->data[i] = (uint16_t) strtol  (buf, &endptr, 0);
			if (*endptr != '\0')
			    ret--;
		    }
		}
		break;

	    case MBRTU_TYPE_UINT32:
	    case MBRTU_TYPE_INT32:
		for (int i = 0; i < call->cnt; i+=2) {
		    buf = strsep (nbuf, ":");
		    if ( buf == NULL )			/* empty values equal to 0 */
			MODBUS_SET_INT32_TO_INT16 (call->data, i, (long int) 0);
		    else {
			if (call->type == MBRTU_TYPE_UINT32)
			    MODBUS_SET_INT32_TO_INT16 (call->data, i, strtoul (buf, &endptr, 0));
			else
			    MODBUS_SET_INT32_TO_INT16 (call->data, i, strtol  (buf, &endptr, 0));
			if (*endptr != '\0')
			    ret--;
		    }
		}
		break;

	    case MBRTU_TYPE_UINT64:
	    case MBRTU_TYPE_INT64:
		for (int i = 0; i < call->cnt; i+=4) {
		    buf = strsep (nbuf, ":");
		    if ( buf == NULL )			/* empty values equal to 0 */
			MODBUS_SET_INT64_TO_INT16 (call->data, i, (long long int) 0);
		    else {
			if (call->type == MBRTU_TYPE_UINT64)
			    MODBUS_SET_INT64_TO_INT16 (call->data, i, strtoull (buf, &endptr, 0));
			else
			    MODBUS_SET_INT64_TO_INT16 (call->data, i, strtoll  (buf, &endptr, 0));
			if (*endptr != '\0')
			    ret--;
		    }
		}
		break;


	    default: {	/* 32 bit float */

		void (* set_float) (float, uint16_t *) = &modbus_set_float_abcd;

		switch (call->type) {
		    /* default */
/*		    case MBRTU_TYPE_F32_ABCD:
			break; */
		    case MBRTU_TYPE_F32_BADC:
			set_float = &modbus_set_float_badc;
			break;
		    case MBRTU_TYPE_F32_CDAB:
			set_float = &modbus_set_float_cdab;
			break;
		    case MBRTU_TYPE_F32_DCBA:
			set_float = &modbus_set_float_dcba;
			break;
		}

		for (int i = 0; i < call->cnt; i+=2) {
		    buf = strsep (nbuf, ":");
		    if ( buf == NULL )			/* empty values equal to 0 */
			set_float (0, &call->data[i]);
		    else {
			set_float (strtof (buf, &endptr), &call->data[i]);
			if (*endptr != '\0')
			    ret--;
		    }
IF_DEBUG		fprintf (stderr, "buf=%s\tfloat=%f\tdata[%u]=%u\tdata[%u]=%u\n", buf, strtof (buf, NULL), i, call->data[i], i+1, call->data[i+1]);
		}
	    } /* default */
	} /* switch */
    } /* else */
    return ret;
}



void print_data (mbrtu_call *call)
{
    int		i;

    if (global_flags & MBRTU_FLAGS_PARSEABLE)
	fprintf (stdout, "%u %u ", call->addr, call->reg);
    else
	fprintf (stdout, "ADDR=%u REG=%u DATA=", call->addr, call->reg);

    switch (call->type) {
	case MBRTU_TYPE_CHAR:
	    for (i=0; i < call->cnt; i++)
		fprintf (stdout, "%c%c", call->data[i] >> 8, call->data[i] & 0xFF);
	    break;

	case MBRTU_TYPE_UINT16:
	    if (call->cnt) {
		fprintf (stdout, "%u", call->data[0]);
		for (i=1; i < call->cnt; i++)
		    fprintf (stdout, ":%u", call->data[i]);
	    }
	    break;
	case MBRTU_TYPE_INT16:
	    if (call->cnt) {
		fprintf (stdout, "%d", call->data[0]);
		for (i=1; i < call->cnt; i++)
		    fprintf (stdout, ":%d", call->data[i]);
	    }
	    break;
	case MBRTU_TYPE_UINT32:
	    if (call->cnt) {
		fprintf (stdout, "%lu", (unsigned long int) MODBUS_GET_INT32_FROM_INT16(call->data, 0));
		for (i=2; i < call->cnt; i+=2)
		    fprintf (stdout, ":%lu", (unsigned long int) MODBUS_GET_INT32_FROM_INT16(call->data, i));
	    }
	    break;
	case MBRTU_TYPE_INT32:
	    if (call->cnt) {
		fprintf (stdout, "%ld", (long int) MODBUS_GET_INT32_FROM_INT16(call->data, 0));
		for (i=2; i < call->cnt; i+=2)
		    fprintf (stdout, ":%ld", (long int) MODBUS_GET_INT32_FROM_INT16(call->data, i));
	    }
	    break;
	case MBRTU_TYPE_UINT64:
	    if (call->cnt) {
		fprintf (stdout, "%llu", (unsigned long long int) MODBUS_GET_INT64_FROM_INT16(call->data, 0));
		for (i=4; i < call->cnt; i+=4)
		    fprintf (stdout, ":%llu", (unsigned long long int) MODBUS_GET_INT64_FROM_INT16(call->data, i));
	    }
	    break;
	case MBRTU_TYPE_INT64:
	    if (call->cnt) {
		fprintf (stdout, "%lld", (long long int) MODBUS_GET_INT64_FROM_INT16(call->data, 0));
		for (i=4; i < call->cnt; i+=4)
		    fprintf (stdout, ":%lld", (long long int) MODBUS_GET_INT64_FROM_INT16(call->data, i));
	    }
	    break;
	case MBRTU_TYPE_HEX:
	    if (call->cnt) {
		fprintf (stdout, "0x%X", call->data[0]);
		for (i=1; i < call->cnt; i++)
		    fprintf (stdout, ":0x%X", call->data[i]);
	    }
	    break;

	default: {	/* float */

	    float (* get_float) (const uint16_t *) = &modbus_get_float_abcd;

	    switch (call->type) {
		/* default */
/*		case MBRTU_TYPE_F32_ABCD:
		    break; */
		case MBRTU_TYPE_F32_BADC:
		    get_float = &modbus_get_float_badc;
		    break;
		case MBRTU_TYPE_F32_CDAB:
		    get_float = &modbus_get_float_cdab;
		    break;
		case MBRTU_TYPE_F32_DCBA:
		    get_float = &modbus_get_float_dcba;
		    break;
	    } /* switch */

	    if (call->cnt) {
		fprintf (stdout, "%.2f", get_float(&call->data[0]));
		for (i=2; i < call->cnt; i+=2)
		    fprintf (stdout, ":%.2f", get_float(&call->data[i]));
	    }
	} /* default */
    } /* switch */

    fprintf (stdout, "\n");
}




int make_call (mbrtu_call *call)
{
    int		ret;
    static char first_call = 1;

IF_DEBUG	fprintf (stdout, "ADDR=%u FUNC=%u REG=%u CNT=%u\n", call->addr, call->func, call->reg, call->cnt);

    if (call->func == 3 || call->func == 4)
	if (NULL == (call->data = realloc (call->data, call->cnt * sizeof(uint16_t)))) {
	    IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
	    exit (-1);
	}

    ret = modbus_set_slave (conn->ctx, call->addr);

    if ( first_call) {
	if ((ret = modbus_connect(conn->ctx)) < 0) {
	    IF_N_QUIET fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	    return ret;
	}
	first_call = 0;
    } else
	usleep (conn->delay * 1000);


    switch (call->func) {
/* read functions: */
	case 3:
	    ret = modbus_read_registers(conn->ctx, call->reg, call->cnt, call->data);
	    if (ret < 0) {
		IF_N_QUIET fprintf (stderr, "ADDR=%u REG=%u ERROR: %s\n", call->addr, call->reg, modbus_strerror(errno));
	    } else {
		call->cnt = ret;
		print_data (call);
	    }
	    break;
	case 4:
	    ret = modbus_read_input_registers(conn->ctx, call->reg, call->cnt, call->data);
	    if (ret < 0) {
		IF_N_QUIET fprintf (stderr, "ADDR=%u REG=%u ERROR: %s\n", call->addr, call->reg, modbus_strerror(errno));
	    } else {
		call->cnt = ret;
		print_data (call);
	    }
	    break;

/* write functions: */
	case 6:
IF_DEBUG {
	    fprintf (stdout, "DATA= ");
	    for (int i=0; i < call->cnt; i++)
		fprintf (stdout, "%u ", call->data[i]);
	    fprintf (stdout, "\n");
	} /* IF_DEBUG */

	    ret = modbus_write_register(conn->ctx, call->reg, call->data[0]);
	    if (ret < 0) {
		IF_N_QUIET fprintf (stderr, "ADDR=%u REG=%u ERROR: %s\n", call->addr, call->reg, modbus_strerror(errno));
	    }
	    break;

	case 16:
IF_DEBUG {
	    fprintf (stdout, "DATA= ");
	    for (int i=0; i < call->cnt; i++)
		fprintf (stdout, "%u ", call->data[i]);
	    fprintf (stdout, "\n");
	} /* IF_DEBUG */

	    ret = modbus_write_registers(conn->ctx, call->reg, call->cnt, call->data);
	    if (ret < 0) {
		IF_N_QUIET fprintf (stderr, "ADDR=%u REG=%u ERROR: %s\n", call->addr, call->reg, modbus_strerror(errno));
	    } else
		if (global_flags & MBRTU_FLAGS_PARSEABLE)
		    fprintf (stdout, "%u %u %u\n", call->addr, call->reg, ret);
		else
		    fprintf (stdout, "ADDR=%u REG=%u written registers=%u\n", call->addr, call->reg, ret);
	    break;
    }
    return ret ;
}



static inline int parse_bus_parameter_options (int argc, char *argv[])
{
    int		ret;

    opterr = 0;
			/* bus  options:     O:b:d:p:s:T:	    */
			/* call options:		 a:f:n:r:t: */
    while ((ret = getopt (argc, argv, "+:DhPQO:b:d:p:s:T:a:f:n:r:t:")) > 0) {
	switch (ret) {
	    case 'd':
		ret = strlen(optarg);
		conn->dev_file = (char*) realloc (conn->dev_file, ret+1);
		if (NULL != conn->dev_file) {
		    conn->dev_file      = strncpy (conn->dev_file, optarg, ret);
		    conn->dev_file[ret] = '\0';
		} else {
		    IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
		    return -1;
		}
		break;

	    case 'b':
		switch ((conn->baudrate = (int) strtol (optarg, NULL, 0))) {
		    case 1200:
		    case 2400:
		    case 4800:
		    case 9600:
		    case 19200:
		    case 38400:
		    case 57600:
		    case 115200:
			break;
		    default:
			IF_N_QUIET fprintf (stderr, "Baud rate \"%s\" not available. Setting to 9600!\n", optarg);
			conn->baudrate = 9600;
		}
		break;

	    case 'p':
		switch (optarg[0]) {
		    case 'O':
		    case 'o':
			conn->parity = 'O';
			conn->stopbits = 1;
			break;
		    case 'N':
		    case 'n':
			conn->parity = 'N';
			conn->stopbits = 2;
			break;
		    case 'E':
		    case 'e':
			break;
		    default:
			IF_N_QUIET fprintf (stderr, "Couldn't parse parity: \"%s\"!\n", optarg);
			return -1;
		}
		break;

	    case 's':
		switch (optarg[0]) {
		    case '1':
                        conn->stopbits = 1;
                        break;
		    case '2':
                        conn->stopbits = 2;
			break;
		    default:
			IF_N_QUIET fprintf (stderr, "Couldn't parse number stop bits: \"%s\"!\n", optarg);
			return -1;
		}
		break;

	    case 'h':
		show_help();
		break;

	    case 'D':
		global_flags |= MBRTU_FLAGS_DEBUG;
		break;

	    case 'O':
		conn->timeout = strtol (optarg, NULL, 0);
		if ((errno != 0) || (conn->timeout < 500)) {
		    IF_N_QUIET fprintf (stderr, "Couldn't parse argument \"%s\". Value not greater than 500 or not in range for long int. Ignoring!\n", optarg);
		    conn->timeout = 500;
		}
		break;

	    case 'P':
		global_flags |= MBRTU_FLAGS_PARSEABLE;
		break;

	    case 'Q':
		global_flags |= MBRTU_FLAGS_QUIET;
		global_flags &= ~MBRTU_FLAGS_DEBUG;
		break;

	    case 'T':
		conn->delay = (uint16_t) strtol (optarg, NULL, 0);
		break;
	} /* switch */

	if (( ret == 'a' )		/* stop parsing of bus parameter options */
	   |( ret == 'f' )		/* these options are in optstring because */
	   |( ret == 'n' )		/* optarg won't be set otherwise */
	   |( ret == 'r' )
	   |( ret == 't' ))
	    return ret;

    } /* while - loop over bus device settings */

    return ret;
}



inline int parse_call_parameter_options (int argc, char *argv[], int option)
{
    int		set_flags = 0;
    int		ret = 0;
    mbrtu_call	*call;

    call = (mbrtu_call *) malloc(sizeof(mbrtu_call));
    if (NULL == call) {
	IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
	return -1;
    }

    call->addr		= 1;
    call->func		= 4;
    call->cnt		= 0;
    call->reg		= 0;
    call->data		= NULL;
    call->ntmp		= NULL;
    call->type		= MBRTU_TYPE_CHAR;

    do {			/* while getopt */
	long int	tmp;
	char		* endptr;

	switch (option) {
	    case 'a':
		tmp = strtol (optarg, &endptr, 0);

		if (*endptr == '\0' && tmp >= 0 && tmp <= 255) {
		    call->addr = (uint8_t) tmp;
//		    set_flags |= MBRTU_SET_ADDR;
		} else
		    IF_N_QUIET fprintf (stderr, "Couldn't parse argument or address \"%s\" out of range [0-255]. Ignoring!\n", optarg);
		break;


	    case 'f':
		switch (optarg[0]) {
		    case 'w':
		    case 'W':
			call->func = 6;
//			set_flags |= MBRTU_SET_FUNC;
			break;
		    case 'h':
		    case 'H':
			call->func = 3;
//			set_flags |= MBRTU_SET_FUNC;
			break;
		    case 'i':
		    case 'I':
			call->func = 4;
//			set_flags |= MBRTU_SET_FUNC;
			break;
		    default:
			tmp = strtol (optarg, &endptr, 0);
			if (*endptr != '\0' )
			    tmp = -1;
			switch (tmp) {
			    case 3:
			    case 4:
			    case 6:
			    case 16:
				call->func = (uint8_t) tmp;
//				set_flags |= MBRTU_SET_FUNC;
				break;
			    default:
				IF_N_QUIET fprintf (stderr, "Function \"%s\" not implemented. Ignoring!\n", optarg);
			}
		}
		break;


	    case 'n':
		call->ntmp = optarg;
		set_flags |= MBRTU_SET_CNT;
		break;


	    case 'r':
		tmp = strtol (optarg, &endptr, 0);
		if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
		    call->reg = (uint16_t) tmp;
		    if (set_flags & MBRTU_SET_CNT)
			if (! parse_n_opt(call))
			    set_flags &= ~MBRTU_SET_CNT;
		    ret = make_call(call);
		} else
		    IF_N_QUIET fprintf (stderr, "Couldn't parse argument \"%s\" or register address out of range [0-65535]. Ignoring!\n", optarg);
		break;


	    case 't':
		call->type = MBRTU_TYPE_HEX;

		if      (! strncasecmp ("int",      optarg, 3))
		    call->type = MBRTU_TYPE_INT16;
		else if (! strncasecmp ("uint",     optarg, 4))
		    call->type = MBRTU_TYPE_UINT16;
		else if (! strncasecmp ("CHAR",     optarg, 4))
		    call->type = MBRTU_TYPE_CHAR;
		else if (! strncasecmp ("int16",    optarg, 5))
		    call->type = MBRTU_TYPE_INT16;
		else if (! strncasecmp ("int32",    optarg, 5))
		    call->type = MBRTU_TYPE_INT32;
		else if (! strncasecmp ("int64",    optarg, 5))
		    call->type = MBRTU_TYPE_INT64;
		else if (! strncasecmp ("uint16",   optarg, 6))
		    call->type = MBRTU_TYPE_UINT16;
		else if (! strncasecmp ("uint32",   optarg, 6))
		    call->type = MBRTU_TYPE_UINT32;
		else if (! strncasecmp ("uint64",   optarg, 6))
		    call->type = MBRTU_TYPE_UINT64;
		else if (! strncasecmp ("F32_ABCD", optarg, 8))
		    call->type = MBRTU_TYPE_F32_ABCD;
		else if (! strncasecmp ("F32_BADC", optarg, 8))
		    call->type = MBRTU_TYPE_F32_BADC;
		else if (! strncasecmp ("F32_CDAB", optarg, 8))
		    call->type = MBRTU_TYPE_F32_CDAB;
		else if (! strncasecmp ("F32_DCBA", optarg, 8))
		    call->type = MBRTU_TYPE_F32_DCBA;

//		set_flags |= MBRTU_SET_TYPE;
		break;

	    case 'D':
		global_flags |= MBRTU_FLAGS_DEBUG;
		modbus_set_debug(conn->ctx, 1);
		break;

	    case 'P':
		global_flags |= MBRTU_FLAGS_PARSEABLE;
		break;

	    case 'Q':
		global_flags |= MBRTU_FLAGS_QUIET;
		global_flags &= ~MBRTU_FLAGS_DEBUG;
		break;

	    case 'T':
		conn->delay = (uint16_t) strtol (optarg, NULL, 0);
		break;

	    case ':':
		IF_N_QUIET fprintf (stderr, "Missing argument - ignoring option %s.\n", argv[optind-1]);
		break;

	    default:
		IF_N_QUIET fprintf (stderr, "unknown call parameter -%c - ignoring\n", optopt);
	} /* switch */
    } while ((option = getopt (argc, argv, "+:DPQa:f:n:r:t:T:")) > 0);

    return ret;
}



int main(int argc, char *argv[])
{
    int		ret;

    conn = (mbrtu_conn *) malloc(sizeof(mbrtu_conn));
    if ( NULL == conn ) {
	IF_N_QUIET fprintf (stderr, "Not enough memory available!\n");
	return -1;
    }

    conn->parity   = 'E';
    conn->stopbits = 1;
    conn->baudrate = 9600;
    conn->delay    = 0;
    conn->timeout  = 500;
/*
 * parse bus parameter options
 */
    ret = parse_bus_parameter_options (argc, argv);

    if (NULL == conn->dev_file) {
	IF_N_QUIET fprintf (stderr, "\nPlease give a device to use!\n"
				    "Use -h for help!\n\n");
	return 0;
    }

    if ( optarg != NULL ) {	/* only try to bind to device and */
				/* parse next option if there was at least one call parameter */
	conn->ctx = modbus_new_rtu (conn->dev_file, conn->baudrate, conn->parity, 8, conn->stopbits);
	if (conn->ctx == NULL) {
	    IF_N_QUIET fprintf(stderr, "Unable to create the libmodbus context.\n");
	    return -1;
	}

	modbus_set_response_timeout (conn->ctx, conn->timeout/1000, (conn->timeout%1000)*1000);
IF_DEBUG	fprintf (stderr, "Timeout set to %lds and %ldms.\n", conn->timeout/1000, (conn->timeout%1000));

	IF_DEBUG modbus_set_debug(conn->ctx, 1);
	ret = parse_call_parameter_options (argc, argv, ret);	/* read call parameters and do the calls */
	modbus_close (conn->ctx);
	modbus_free  (conn->ctx);
    }

    return (ret>0) ? 0 : -ret;
}
