/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"
/* ----------------------- Variables ----------------------------------------*/
static eMBEventType eQueuedEvent[MODBUS_SLAVE_MAXNUM];
static BOOL     xEventInQueue[MODBUS_SLAVE_MAXNUM];
static eMBEventFrom xEventFrom[MODBUS_SLAVE_MAXNUM];

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( void )
{
	for(uint8_t i = 0; i < MODBUS_SLAVE_MAXNUM ; i++)
    xEventInQueue[i] = FALSE;
    return TRUE;
}

BOOL
xMBPortEventPost( eMBEventType eEvent , eMBEventFrom eFrom , eMBPort port)
{
	if(port >= PORT_MB_MAX)
	{
		return FALSE;
	}
    xEventInQueue[port] = TRUE;
    eQueuedEvent[port] = eEvent;
	xEventFrom[port] = eFrom;
    return TRUE;
}

BOOL
xMBPortEventGet( eMBEventType * eEvent , eMBEventFrom * eFrom , eMBPort port)
{
    BOOL            xEventHappened = FALSE;
	if(port >= PORT_MB_MAX)
	{
		return FALSE;
	}
    if( xEventInQueue[port] )
    {
        *eEvent = eQueuedEvent[port];
        xEventInQueue[port] = FALSE;
        xEventHappened = TRUE;
		*eFrom = xEventFrom[port];
    }
    return xEventHappened;
}
