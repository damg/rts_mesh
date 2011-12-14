
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "io_avrfnkbrd.h"



#include "uart.h"

#include "protocol.h"





uint8_t generate_data( data_u* pData, dataType_e* pDataType )
{
	static uint8_t data_counter = 0;
	
	data_counter++;

	*pDataType = DT_INT_COUNTER;
	pData->iValue = data_counter;
	
	return 0;
};



int main ( void )
{
	uint16_t ticker = 0;
	uint8_t iError = 0;
		
	data_u* tx_data = NULL;
	dataType_e dataType = DT_NONE;

	INIT_TASTER;
	uart_init();
	proto_init();
	sei();

	uart_putstr ("\r\nHS Mhm\r\n");

	// ist der Taster beim Booten gedrueckt, kann/muss der Konten konfiguriert werden	
	proto_setup( TASTER1 );
	
	while ( 1 )
	{
		// Zykluszaehler 
		ticker++;
	
		proto_cycle();
			
		// zyklisch etwas senden
		if (ticker == 0)
		{
			tx_data = proto_get_tx_data();
			if ( tx_data )
			{
				iError = generate_data( tx_data, &dataType );
				if ( !iError )				
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
}

	
