// Defintion der Protokoll-Funktionen fuer das RTS-Projekt IM1 WS2011

#define USE_LEDS			1	// wenn als 1 definiert werden die LEDS zur Send-Empfangsanzeige genutzt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include "io_avrfnkbrd.h"
#include "protocol.h"
#include "uart.h"
#include "rfm12_config.h"
#include "rfm12.h"

#define LED_LEUCHTDAUER		10000


// modulglobale Variablen fuers Protokoll
uint8_t iNodeId = 0;			// NodeID des Moduls (Adresse)
uint8_t iIsGateway = 0;			// 1=Konten ist Gateway, 0=Sensorknoten

tele_s tx_tele[TELEBUFFER_MAX];
teleStatus_e tx_status[TELEBUFFER_MAX];
tele_s* pRx_tele = NULL;
uint8_t iTxTelePos = 0;

// modulglobale Hilfsvariablen
uint16_t led1_an = 0;
uint16_t led2_an = 0;
char sInfo[255] = "";

// EEPROM Variable
uint8_t EEPROM_nodeId EEMEM;	// um iNodeId persistent zu speichern	
uint8_t EEPROM_isGateway EEMEM;	// um iIsGateway persistent zu speichern	

//#define EEPROM_START_ADDR    0
//#define EEPROM_ADDR_NODEID   (0 + EEPROM_START_ADDR)

static void led_dimm();


uint8_t rx_telegram( tele_s* pTele )
{
	uint8_t bOk = 0;
	char str[100];
	
	switch ( pTele->dataType )
	{
		case DT_INT_COUNTER:
			sprintf( str, "rx: %d (type=iC)\r\n", pTele->data.iValue );
			uart_putstr( str );
			bOk = 1;
			break;
			
		case DT_FLOAT_TEMP:
			sprintf( str, "rx: %.2f (type=fT)\r\n", pTele->data.fValue );
			uart_putstr( str );
			bOk = 1;
			break;
			
		default:
			break;
	}		
	
	return bOk;
}


uint8_t proto_init()
{
#if USE_LEDS
	INIT_LED;
#endif

	//PD7 <> FSK/DATA/NFFS auf 1 setzen
	DDR_SS |= (1 << PD7);	
	PORTD  |= (1 << PD7);

	rfm12_init();
	
	memset( tx_tele, 0, sizeof(tx_tele) );
	memset( tx_status, TS_FREE, sizeof(tx_status) );
	
	
	return 0;
}

uint8_t proto_setup( uint8_t service )
{
	char buf[100] = "";
	uint8_t loop = 1;
	
	if ( service )
	{
		// Service mode: Konten konfigurieren bzw. Infos anfragen
		uart_putstr( "service mode OK\r\n" );
	
		while ( loop )
		{
			// Format der Eingabe:  <CMD><OP>[<VALUE>]		
			// <CMD>	= Command: h=help, n=nodeId, x=exit
			// <OP>		= Operator: '='=set, '?'=get
			// <VALUE>	= optionaler Wert
		
			uart_getline( buf, sizeof(buf) );
		
			switch ( buf[0] )
			{
				case 'h':
				case 'H':
					if ( buf[1] == '?' )
					{
						uart_putstr( "Format der Eingabe:  <CMD><OP>[<VALUE>]\r\n" );
						uart_putstr( "<CMD>\t= Command: h=help, n=nodeId\r\n" );
						uart_putstr( "<VALUE>\t= optionaler Wert\r\n" );
						uart_putstr( "OK" );
					}		
					break;
				
				case 'n':
				case 'N':
					// NodeID persistent in EEPROM speichern (nur "einmalig" --> dann kann das EEPROM in ein File ausgelesen werden)
					switch ( buf[1] )
					{
						case '=':
							iNodeId = atoi( buf + 2 ) % 255;
							eeprom_write_byte( &EEPROM_nodeId, iNodeId );
					
							sprintf( sInfo, "nodeID=%d OK\r\n", iNodeId );
							uart_putstr( sInfo );
							break;
						
						case '?':
							iNodeId = eeprom_read_byte( &EEPROM_nodeId );
							sprintf( sInfo, "nodeID=%d OK\r\n", iNodeId );
							uart_putstr( sInfo );
							break;
							
						default:
							uart_putstr( "operator invalid ERROR\r\n" );
							break;
					}
					break;

				case 'g':
				case 'G':
					// isGateway persistent in EEPROM speichern
					switch ( buf[1] )
					{
						case '=':
							iIsGateway = (atoi( buf + 2 ) > 0);
							eeprom_write_byte( &EEPROM_isGateway, iIsGateway );
					
							sprintf( sInfo, "isGateway=%d OK\r\n", iIsGateway );
							uart_putstr( sInfo );
							break;
						
						case '?':
							iNodeId = eeprom_read_byte( &EEPROM_isGateway );
							sprintf( sInfo, "isGateway=%d OK\r\n", iIsGateway );
							uart_putstr( sInfo );
							break;
							
						default:
							uart_putstr( "operator invalid ERROR\r\n" );
							break;
					}
					break;
				
				case 'x':
				case 'X':
					loop = 0;
					uart_putstr( "service mode exit OK\r\n" );
					break;
					
				default:
					uart_putstr( "command invalid ERROR\r\n" );
					break;
			}
		}
	}
	else
	{
		// gespeicherte NodeID aus EEPROM lesen
		iNodeId = eeprom_read_byte( &EEPROM_nodeId );		
		sprintf( sInfo, "nodeID=%d\r\n", iNodeId );
		uart_putstr( sInfo );
	}
				
	return 0;	
}

uint8_t proto_cycle()
{
	int iLen = 0;
	
	// wenn etwas empfangen dieses verarbeiten und ggf forwarden
	if ( rfm12_rx_status() == STATUS_COMPLETE )
	{
		LED1_ON;
		led1_an = LED_LEUCHTDAUER;
		//uart_putstr ("rx packet: ");

		iLen = rfm12_rx_len();
		pRx_tele = (tele_s*)rfm12_rx_buffer();
		while ( iLen >= sizeof(tele_s) )
		{
			rx_telegram( pRx_tele );
				
			pRx_tele->ttl--;
			if ( pRx_tele->ttl > 0 )
			{
				sprintf( sInfo, "fwd (adrSrc=%d, adrDest=%d, ttl=%d)\r\n", pRx_tele->adr_src, pRx_tele->adr_dest, pRx_tele->ttl );
				uart_putstr( sInfo );
							
				rfm12_tx( sizeof(tele_s), 0, (uint8_t*)pRx_tele );
			}
				
			iLen -= sizeof(tele_s);
		}
			
		// Buffer und ggf unvollstaendige Telegramme freigeben
		rfm12_rx_clear();
	}

	// Telegrammverarbeitung 
	rfm12_tick();
	
	led_dimm();

	return 1;
}

void led_dimm()
{
	// ggf LED's nach kurzer Leuchtzeit wieder ausschalten
	if ( led1_an > 0 )
		led1_an--;
	if ( led1_an == 0 )
		LED1_OFF;
	if ( led2_an > 0 )
		led2_an--;
	if ( led2_an == 0 )
		LED2_OFF;
		
	return;
}

data_u* proto_get_tx_data()
{
	int i = 0;
	
	for ( ; i < TELEBUFFER_MAX; i++ )
	{
		if ( tx_status[iTxTelePos] == TS_FREE )
		{
			tx_status[iTxTelePos] = TS_PREPARED;
			memset( tx_tele+iTxTelePos, 0, sizeof(tx_tele[0]) );
			return &tx_tele[iTxTelePos].data;
		}

		iTxTelePos++;
		iTxTelePos %= TELEBUFFER_MAX;
	}
	return NULL;
}

uint8_t proto_tx_data( data_u* pData, dataType_e dataType )
{
	uint8_t iError = 0;
	tele_s* pTele = &tx_tele[iTxTelePos];
	
	if ( &pTele->data != pData )
	{
		// uebergebener Puffer und angefordener Puffer verschieden
		iError = 1;
	}
	
	if ( !iError )
	{
		pTele->adr_flags_src = (uint8_t)( iIsGateway ? AF_GATEWAY : 0 );
		pTele->adr_src = iNodeId;
				
		pTele->adr_flags_dest = (uint8_t)AF_GATEWAY;
		pTele->adr_dest = 0;
			
		pTele->ttl = TTL_MAX;
				
		sprintf( sInfo, "tx (adrSrc=%d, adrDest=%d,ttl=%d)\r\n", pTele->adr_src, pTele->adr_dest, pTele->ttl );
		uart_putstr( sInfo );
				
		rfm12_tx( sizeof(*pTele), 0, (uint8_t*)pTele );

		LED2_ON;
		led2_an = LED_LEUCHTDAUER;
	}
		
	return iError;
}