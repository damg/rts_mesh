
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "io_avrfnkbrd.h"
#include "rfm12_config_rts.h"
#include "uart.h"
#include "protocol.h"


BOOL generate_data( data_u* pData, dataType_e* pDataType )
{
	static uint8_t data_counter = 0;
	
	data_counter++;

	*pDataType = DT_INT_COUNTER;
	pData->iValue = data_counter;
	
	return TRUE;
}

BOOL isTrigger()
{
	static uint16_t ticker[2] = {0,2};
	

	if ( ticker[1] >= 2 ) 
	{
		ticker[1] = 0;
		return TRUE;		
	}

	// Zykluszaehler 
	ticker[0]++;
	if ( ticker[0] == 0 )
	{
		ticker[1]++;
	}		

	return FALSE;
}

int main ( void )
{
	data_u* tx_data = NULL;
	dataType_e dataType = DT_NONE;
	
	INIT_TASTER;
	proto_init();
	uart_init();
	sei();

	uart_putstr ("\r\nHS Mhm\r\n");

	// ist der Taster beim Booten gedrueckt, kann/muss der Konten konfiguriert werden	
	proto_setup( TASTER1 );
	
	uart_putstr ("\r\nrunning ..\r\n");
	
	while ( 1 )
	{
		// zyklisch etwas senden
		if ( isTrigger() )
		{
			if ( !proto_is_gateway() )
			{
				// freien Telegrammpufferplatz anfordern
				tx_data = proto_get_tx_data();
				if ( tx_data )
				{
					// Nutzdaten in Telegramm schreiben und versenden
					if ( generate_data( tx_data, &dataType ) )				
					{
						proto_tx_data( tx_data, dataType );
					}
					else
					{	
						proto_tx_free( tx_data );
					}
				}
			}			
		}

		// Protokoll-/Telegrammverarbeitung triggern
		proto_cycle();
	}
}

	
