/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"

#include "mbconfig.h"
#include "mbascii.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

#if MB_ASCII_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */
#define MB_SER_PDU_SIZE_MIN     3       /*!< Minimum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_LRC     1       /*!< Size of LRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_M_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_M_RX_RCV,               /*!< Frame is beeing received. */
    STATE_M_RX_WAIT_EOF           /*!< Wait for End of Frame. */
} eMBMasterRcvState;

typedef enum
{
    STATE_M_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_M_TX_START,             /*!< Starting transmission (':' sent). */
    STATE_M_TX_DATA,              /*!< Sending of data (Address, Data, LRC). */
    STATE_M_TX_END,               /*!< End of transmission. */
    STATE_M_TX_NOTIFY,             /*!< Notify sender that the frame has been sent. */
	STATE_M_TX_XFWR,			  /*!< Transmitter is in transfer finish and wait receive state. */
} eMBMasterSndState;

typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBBytePos;

/* ----------------------- Static functions ---------------------------------*/
static UCHAR    prvucMBCHAR2BIN( UCHAR ucCharacter );

static UCHAR    prvucMBBIN2CHAR( UCHAR ucByte );

static UCHAR    prvucMBLRC( UCHAR * pucFrame, USHORT usLen );

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBMasterSndState eMasterSndState;
static volatile eMBMasterRcvState eMasterRcvState;

/* We reuse the Modbus RTU buffer because only one buffer is needed and the
 * RTU buffer is bigger. */
extern volatile UCHAR ucMasterRTUSndBuf[];
static volatile UCHAR *ucMasterASCIISndBuf = ucMasterRTUSndBuf;

extern volatile UCHAR  ucMasterRTURcvBuf[];
static volatile UCHAR *ucMasterASCIIRcvBuf = ucMasterRTURcvBuf;

static volatile USHORT usMasterRcvBufferPos;
static volatile eMBBytePos eMasterBytePos;

static volatile UCHAR *pucMasterSndBufferCur;
static volatile USHORT usMasterSndBufferCount;

static volatile UCHAR ucMasterLRC;
static volatile UCHAR ucMasterMBLFCharacter;

static volatile BOOL   xFrameIsBroadcast = FALSE;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBMasterASCIIInit(  UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );
    ucMasterMBLFCharacter = MB_ASCII_DEFAULT_LF;

    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 7, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else if( xMBMasterPortTimersInit( MB_ASCII_TIMEOUT_SEC * 20000UL ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }

    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBMasterASCIIStart( void )
{
    ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( TRUE, FALSE );
    eMasterRcvState = STATE_M_RX_IDLE;
    EXIT_CRITICAL_SECTION(  );

    /* No special startup required for ASCII. */
    ( void )xMBPortEventPost( EV_READY ,EV_ASCII , PORT_MB_ASCII_RTU);
}

void
eMBMasterASCIIStop( void )
{
    ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( FALSE, FALSE );
    vMBMasterPortTimersDisable(  );
    EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBMasterASCIIReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );
    //assert( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX );

    /* Length and CRC check */
    if( ( usMasterRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( prvucMBLRC( ( UCHAR * ) ucMasterASCIIRcvBuf, usMasterRcvBufferPos ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucMasterASCIIRcvBuf[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_LRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ucMasterASCIIRcvBuf[MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMasterASCIISend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR           usLRC;

    ENTER_CRITICAL_SECTION(  );
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( eMasterRcvState == STATE_M_RX_IDLE )
    {
        /* First byte before the Modbus-PDU is the slave address. */
        pucMasterSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usMasterSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usMasterSndBufferCount += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = prvucMBLRC( ( UCHAR * ) pucMasterSndBufferCur, usMasterSndBufferCount );
        ucMasterASCIISndBuf[usMasterSndBufferCount++] = usLRC;

        /* Activate the transmitter. */
        eMasterSndState = STATE_M_TX_START;
        vMBMasterPortSerialEnable( FALSE, TRUE );
    }
    else
    {
		vMBMasterSetErrorType(EV_ERROR_SEND_FRAME);
		xMBMasterPortEventPost( EV_MASTER_ERROR_PROCESS);
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
xMBMasterASCIIReceiveFSM( unsigned char data )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;
    UCHAR           ucResult;

    //assert( eMasterSndState == STATE_M_TX_IDLE );

    //( void )xMBMasterPortSerialGetByte( ( CHAR * ) & ucByte );
    ucByte = data;
    switch ( eMasterRcvState )
    {
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
    case STATE_M_RX_RCV:
        /* Enable timer for character timeout. */
        vMBMasterPortTimersEnable(  );
        if( ucByte == ':' )
        {
            /* Empty receive buffer. */
            eMasterBytePos = BYTE_HIGH_NIBBLE;
            usMasterRcvBufferPos = 0;
			eMasterSndState = STATE_M_TX_IDLE;
        }
        else if( ucByte == MB_ASCII_DEFAULT_CR )
        {
            eMasterRcvState = STATE_M_RX_WAIT_EOF;
        }
        else
        {
            ucResult = prvucMBCHAR2BIN( ucByte );
            switch ( eMasterBytePos )
            {
                /* High nibble of the byte comes first. We check for
                 * a buffer overflow here. */
            case BYTE_HIGH_NIBBLE:
                if( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX )
                {
                    ucMasterASCIIRcvBuf[usMasterRcvBufferPos] = ( UCHAR )( ucResult << 4 );
                    eMasterBytePos = BYTE_LOW_NIBBLE;
                    break;
                }
                else
                {
                    /* not handled in Modbus specification but seems
                     * a resonable implementation. */
                    eMasterRcvState = STATE_M_RX_IDLE;
                    /* Disable previously activated timer because of error state. */
                    vMBMasterPortTimersDisable(  );
                }
                break;

            case BYTE_LOW_NIBBLE:
                ucMasterASCIIRcvBuf[usMasterRcvBufferPos] |= ucResult;
                usMasterRcvBufferPos++;
                eMasterBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_M_RX_WAIT_EOF:
        if( ucByte == ucMasterMBLFCharacter )
        {
            /* Disable character timeout timer because all characters are
             * received. */
            vMBMasterPortTimersDisable(  );
            /* Receiver is again in idle state. */
            eMasterRcvState = STATE_M_RX_IDLE;

            /* Notify the caller of eMBASCIIReceive that a new frame
             * was received. */
            //xNeedPoll = xMBMasterPortEventPost( EV_MASTER_FRAME_RECEIVED);
            xNeedPoll = xMBMasterPortEventPost_FromISR(EV_MASTER_FRAME_RECEIVED);//目前测试是在中断里面
        }
        else if( ucByte == ':' )
        {
            /* Empty receive buffer and back to receive state. */
            eMasterBytePos = BYTE_HIGH_NIBBLE;
            usMasterRcvBufferPos = 0;
            eMasterRcvState = STATE_M_RX_RCV;

            /* Enable timer for character timeout. */
            vMBMasterPortTimersEnable(  );
        }
        else
        {
            /* Frame is not okay. Delete entire frame. */
            eMasterRcvState = STATE_M_RX_IDLE;
        }
        break;

    case STATE_M_RX_IDLE:
        if( ucByte == ':' )
        {
            /* Enable timer for character timeout. */
            vMBMasterPortTimersEnable(  );
            /* Reset the input buffers to store the frame. */
            usMasterRcvBufferPos = 0;;
            eMasterBytePos = BYTE_HIGH_NIBBLE;
            eMasterRcvState = STATE_M_RX_RCV;
        }
        break;
    }

    return xNeedPoll;
}

BOOL
xMBMasterASCIITransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;

    //assert( eMasterRcvState == STATE_M_RX_IDLE );
    switch ( eMasterSndState )
    {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
    case STATE_M_TX_START:
        ucByte = ':';
        xMBMasterPortSerialPutByte( ( CHAR )ucByte );
        eMasterSndState = STATE_M_TX_DATA;
        eMasterBytePos = BYTE_HIGH_NIBBLE;
		xNeedPoll = TRUE;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream with the high nibble sent first and the low nibble sent
         * last. If all data bytes are exhausted we send a '\r' character
         * to end the transmission. */
    case STATE_M_TX_DATA:
        if( usMasterSndBufferCount > 0 )
        {
            switch ( eMasterBytePos )
            {
            case BYTE_HIGH_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *pucMasterSndBufferCur >> 4 ) );
                xMBMasterPortSerialPutByte( ( CHAR ) ucByte );
                eMasterBytePos = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *pucMasterSndBufferCur & 0x0F ) );
                xMBMasterPortSerialPutByte( ( CHAR )ucByte );
                pucMasterSndBufferCur++;
                eMasterBytePos = BYTE_HIGH_NIBBLE;
                usMasterSndBufferCount--;
                break;
            }

        }
        else
        {
            xMBMasterPortSerialPutByte( MB_ASCII_DEFAULT_CR );
            eMasterSndState = STATE_M_TX_END;
        }
		xNeedPoll = TRUE;
        break;

        /* Finish the frame by sending a LF character. */
    case STATE_M_TX_END:
        xMBMasterPortSerialPutByte( ( CHAR )ucMasterMBLFCharacter );
        /* We need another state to make sure that the CR character has
         * been sent. */
        eMasterSndState = STATE_M_TX_NOTIFY;
		xNeedPoll = TRUE;
        break;

        /* Notify the task which called eMBASCIISend that the frame has
         * been sent. */
    case STATE_M_TX_NOTIFY:
        /* Disable transmitter. This prevents another transmit buffer
         * empty interrupt. */
        vMBMasterPortSerialEnable( TRUE, FALSE );

		xFrameIsBroadcast = ( ucMasterASCIISndBuf[MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
		eMasterSndState = STATE_M_TX_XFWR;
		/* If the frame is broadcast ,master will enable timer of convert delay,
		 * else master will enable timer of respond timeout. */
		if ( xFrameIsBroadcast == TRUE )
		{
			vMBMasterPortTimersConvertDelayEnable();//just timeout value
		}
		else
		{
			vMBMasterPortTimersRespondTimeoutEnable();//just timeout value
		}
        //eMasterSndState = STATE_M_TX_IDLE;
        break;

        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_M_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBMasterPortSerialEnable( TRUE, FALSE );
        break;
    }

    return xNeedPoll;
}

BOOL
xMBMasterASCIITimerT1SExpired( void )
{
	BOOL xNeedPoll;
    switch ( eMasterRcvState )
    {
        /* If we have a timeout we go back to the idle state and wait for
         * the next frame.
         */
    case STATE_M_RX_RCV:
    case STATE_M_RX_WAIT_EOF:
        eMasterRcvState = STATE_M_RX_IDLE;
        break;

    default:
        ////assert( ( eRcvState == STATE_RX_RCV ) || ( eRcvState == STATE_RX_WAIT_EOF ) );
        break;
    }
	eMasterRcvState = STATE_M_RX_IDLE;

	switch(eMasterSndState)
	{
		case STATE_M_TX_XFWR:
		if ( xFrameIsBroadcast == FALSE ) {
			vMBMasterSetErrorType(EV_ERROR_RESPOND_TIMEOUT);
			xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
		}
			break;
		default:
			break;
	}
	eMasterSndState = STATE_M_TX_IDLE;
    vMBMasterPortTimersDisable( );

    /* no context switch required. */
    return FALSE;
}


static          UCHAR
prvucMBCHAR2BIN( UCHAR ucCharacter )
{
    if( ( ucCharacter >= '0' ) && ( ucCharacter <= '9' ) )
    {
        return ( UCHAR )( ucCharacter - '0' );
    }
    else if( ( ucCharacter >= 'A' ) && ( ucCharacter <= 'F' ) )
    {
        return ( UCHAR )( ucCharacter - 'A' + 0x0A );
    }
    else
    {
        return 0xFF;
    }
}

static          UCHAR
prvucMBBIN2CHAR( UCHAR ucByte )
{
    if( ucByte <= 0x09 )
    {
        return ( UCHAR )( '0' + ucByte );
    }
    else if( ( ucByte >= 0x0A ) && ( ucByte <= 0x0F ) )
    {
        return ( UCHAR )( ucByte - 0x0A + 'A' );
    }
    else
    {
        /* Programming error. */
        //assert( 0 );
    }
    return '0';
}


static          UCHAR
prvucMBLRC( UCHAR * pucFrame, USHORT usLen )
{
    UCHAR           ucLRC = 0;  /* LRC char initialized */

    while( usLen-- )
    {
        ucLRC += *pucFrame++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    ucLRC = (~ucLRC) + 1;
    return ucLRC;
}

#endif
