// Defintion der Protokoll-Funktionen fuer das RTS-Projekt IM1 WS2011

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "io_avrfnkbrd.h"
#include "protocol.h"
#include "uart.h"
#include "rfm12_config_rts.h"
#include "rfm12-1.1/src/rfm12.h"

#define USE_LEDS					1						// wenn als 1 definiert werden die LEDS zur Send-Empfangsanzeige genutzt

#define LED_LEUCHTDAUER				5000					// Nachleuchtdauer (in Zyklen)
#define ERR_TEXT_INV_OP				"op inv ERROR\r\n"		// Fehlertext bei Command auf Serviceschnittstelle unbekannt

//	#define TRACE0(l, a)				if ( l <= iLogLevel ) { uart_putstr(a); }
//	#define TRACE1(l, a, b)				if ( l <= iLogLevel ) { sprintf( sInfo, a, b ); uart_putstr(sInfo); }
//	#define TRACE2(l, a, b, c)			if ( l <= iLogLevel ) { sprintf( sInfo, a, b, c ); uart_putstr(sInfo); }
//	#define TRACE3(l, a, b, c, d)		if ( l <= iLogLevel ) { sprintf( sInfo, a, b, c, d ); uart_putstr(sInfo); }
//	#define TRACE4(l, a, b, c, d, e)	if ( l <= iLogLevel ) { sprintf( sInfo, a, b, c, d, e ); uart_putstr(sInfo); }
#define TRACE0( a)					uart_putstr(a); 
#define TRACE1( a, b)				sprintf( sInfo, a, b ); uart_putstr(sInfo); 
#define TRACE2( a, b, c)			sprintf( sInfo, a, b, c ); uart_putstr(sInfo); 
#define TRACE3( a, b, c, d)			sprintf( sInfo, a, b, c, d ); uart_putstr(sInfo); 
#define TRACE4( a, b, c, d, e)		sprintf( sInfo, a, b, c, d, e ); uart_putstr(sInfo); 

#define HAS_FLAG(a, b)				( ( (a) & (b) ) > 0 )


// modulglobale Variablen fuers Protokoll
uint8_t iNodeId = 0;						// NodeID des Moduls (Adresse)
uint8_t iIsGateway = 0;						// 1=Konten ist Gateway, 0=Sensorknoten
uint8_t iLogLevel = 2;						// Level der Logausgaben, 0=wenig, 1=error, 2=info

tele_s tx_tele[TELEBUFFER_MAX];
teleStatus_e tx_status[TELEBUFFER_MAX] = {TS_FREE};
uint8_t tx_retry[TELEBUFFER_MAX] = {0};
tele_s* pRx_tele = NULL;
uint8_t iTxTelePos = 0;
uint8_t iSeqNr = 0;
volatile uint16_t lTimeout[2] = {0,0};		// 32-bit integer selbst gebastelt um 2k ROM zu sparen

// modulglobale Hilfsvariablen
uint16_t led1_an = 0;
uint16_t led2_an = 0;
char sInfo[50] = "";

// EEPROM Variable
uint8_t EEPROM_nodeId EEMEM;				// um iNodeId persistent zu speichern	
uint8_t EEPROM_isGateway EEMEM;				// um iIsGateway persistent zu speichern
uint8_t EEPROM_logLevel EEMEM;				// um iLogLevel persistent zu speichern

// globale Funktionen
uint8_t proto_init();						/// initialisiert das Protokoll-Modul
uint8_t proto_setup( uint8_t service );		/// konfiguriert den Konten und dient zur Konfiguration des Moduls per RS232
uint8_t proto_cycle();
BOOL	proto_is_gateway();

data_u* proto_get_tx_data();
uint8_t proto_tx_data( data_u* pData, dataType_e dataType );
void    proto_tx_free( data_u* pData );	

// lokale Funktionen
static void		led_dimm();
static uint8_t	rx_telegram( tele_s* pTele );
static uint8_t	fwd_telegram( tele_s* pTele );
static uint8_t	quit_telegram( tele_s* pTele, uint8_t iError );
static uint8_t	getChecksum( tele_s* pTele );
static int8_t	checkNodeInPath( tele_s* pRx_tele, int8_t iAppend );
static uint8_t	free_telegram( tele_s* pTele );
static uint8_t	timeout_telegram();
static BOOL		isTimeout();

// ------------------------------------------------------------------------------------------
// Implementierung

uint8_t proto_init()
{
#if USE_LEDS
	INIT_LED;
#endif

	//PD7 <> FSK/DATA/NFFS auf 1 setzen
	DDR_SS |= (1 << PD7);	
	PORTD  |= (1 << PD7);

	iTxTelePos = 0;
	//memset( tx_tele, 0, sizeof(tx_tele) );
	//memset( tx_status, 0, sizeof(tx_status) );
	//memset( tx_retry, 0, sizeof(tx_retry) );

	rfm12_init();
	return 0;
}

uint8_t proto_setup( uint8_t service )
{
	char buf[5] = "";
	uint8_t loop = 1;
	
	// Pruefung auf Designfehler
	if ( ( sizeof(tele_s) + 3 ) > RFM12_TX_BUFFER_SIZE )
	{
		TRACE0( "PrgErr #1\r\n" );
	}		
	
	if ( service )
	{
		// Service mode: Konten konfigurieren bzw. Infos anfragen
		TRACE0( "service mode OK\r\n" );
	
		while ( loop )
		{
			// Format der Eingabe:  <CMD><OP>[<VALUE>]		
			// <CMD>	= Command: h=help, n=nodeId, x=exit, l=logLevel
			// <OP>		= Operator: '='=set, '?'=get
			// <VALUE>	= optionaler Wert
		
			uart_getline( buf, sizeof(buf) );
		
			switch ( buf[0] )
			{
				case 'h':
				case 'H':
					if ( buf[1] == '?' )
					{
						TRACE0( "Format:\t<CMD><OP>[<VALUE>]\r\n" );
						TRACE0( "<CMD>\t= h=help, n=nodeId, g=gateway, l=logLevel, x=exit\r\n" );
						TRACE0( "<VALUE>\t= opt.Wert\r\n" );
						TRACE0( "OK\r\n" );
					}		
					break;
				
				case 'n':
				case 'N':
					// NodeID persistent in EEPROM speichern (nur "einmalig" --> dann kann das EEPROM in ein File ausgelesen werden)
					switch ( buf[1] )
					{
						case '=':
							iNodeId = atoi( buf + 2 );
							eeprom_write_byte( &EEPROM_nodeId, iNodeId );
							// kein break
						case '?':
							TRACE1( "nodeID=%d OK\r\n", iNodeId );
							break;
							
						default:
							TRACE0( ERR_TEXT_INV_OP );
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
							// kein break
						case '?':
							TRACE1( "isGateway=%d OK\r\n", iIsGateway );
							break;
							
						default:
							TRACE0( ERR_TEXT_INV_OP );
							break;
					}
					break;

		/*		case 'l':
				case 'L':
					// iLogLevel persistent in EEPROM speichern
					switch ( buf[1] )
					{
						case '=':
							iLogLevel = atoi( buf + 2 );
							eeprom_write_byte( &EEPROM_logLevel, iLogLevel );
							// kein break
						case '?':
							TRACE1( "iLogLevel=%d OK\r\n", iLogLevel );
							break;
							
						default:
							TRACE0( ERR_TEXT_INV_OP );
							break;
					}
					break;   */
				
				case 'x':
				case 'X':
					loop = 0;
					TRACE0( "service mode exit OK\r\n" );
					break;
					
				default:
					TRACE0( "cmd inv ERROR\r\n" );
					break;
			}
		}
	}
	else
	{
		// gespeicherte NodeID aus EEPROM lesen
		iNodeId		= eeprom_read_byte( &EEPROM_nodeId		);
		iIsGateway	= eeprom_read_byte( &EEPROM_isGateway	);
	//	iLogLevel	= eeprom_read_byte( &EEPROM_logLevel	);	
		
		// geloeschtes Flash, dann default kein Gateway
		if ( iIsGateway == 255 ) 
		{
			iIsGateway = 0;
			eeprom_write_byte( &EEPROM_isGateway, iIsGateway );
		}		
	/*	if ( iLogLevel == 255 ) 
		{
			iLogLevel = 1;
			eeprom_write_byte( &EEPROM_logLevel, iLogLevel );
		}		
		TRACE3(  "nodeID=%d, isGateway=%d, logLevel=%d\r\n", iNodeId, iIsGateway, iLogLevel );  */
		TRACE2( "nodeID=%d, isGateway=%d\r\n", iNodeId, iIsGateway );
	}
				
	return 0;	
}

BOOL proto_is_gateway()
{
	if ( iIsGateway > 0 )
		return TRUE;
	return FALSE;
}

uint8_t proto_cycle()
{
	int iLen = 0;
	
	// wenn etwas empfangen dieses verarbeiten und ggf forwarden
	if ( rfm12_rx_status() == STATUS_COMPLETE )
	{
		LED1_ON;
		led1_an = LED_LEUCHTDAUER;

		iLen = rfm12_rx_len();
		pRx_tele = (tele_s*)rfm12_rx_buffer();
		while ( iLen >= sizeof(tele_s) )
		{
			if ( pRx_tele->dataChecksum == getChecksum(pRx_tele) )
			{
				// Telegramm auswerten und ausgeben (immer damit man etwas was passiert)
				rx_telegram( pRx_tele );

				// verarbeiten/quittieren ODER weiterleiten wenn nicht selbst Empfaenger
				if (	(pRx_tele->adr_dest == iNodeId)												// direkt Empfaenger
					||	(proto_is_gateway() && HAS_FLAG(pRx_tele->adr_flags_dest, AF_GATEWAY) ) 	// selbst Gateway und an Gateway adressiert
					)
				{
					if ( HAS_FLAG(pRx_tele->adr_flags_src, AF_ACK|AF_NACK) )
					{
						// quittiertes Nutzdaten-Telegramm freigeben
						free_telegram( pRx_tele );
					}
					else
					{
						// Telegramm positiv quittieren
						quit_telegram( pRx_tele, 0 );
					}
				}				
				else  // selbst nicht direkt Empfaenger dann ggf weiterleiten
				{
					// Telegramm ggf weiterleiten
					fwd_telegram( pRx_tele );
				}
			}
			else
			{
				TRACE2( "rx wrong CS (seqNr=%d, sum=%d)\r\n", pRx_tele->seqNr, pRx_tele->dataChecksum );
			}				
			iLen -= sizeof(tele_s);
		}
			
		// Buffer und ggf unvollstaendige Telegramme freigeben
		rfm12_rx_clear();
	}
	
	// Telegramme ggf wiederholen
	if ( isTimeout() )
	{
		timeout_telegram();
	}

	// Telegrammverarbeitung 
	rfm12_tick();
	
	// zyklische Aufgaben
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

BOOL isTimeout()
{
	if ( lTimeout[0] > 0)
	{
		lTimeout[0]--;
	}	
	if ( lTimeout[0] == 0 )
	{
		if ( lTimeout[1] >= 0)
		{
			lTimeout[1]--;
		}		
		if ( lTimeout[1] == 0 )
		{
			lTimeout[0] = TELE_TIMEOUT;	
			lTimeout[1] = TELE_TIMEOUT;
			return TRUE;
		}		
	}
	return FALSE;
}

data_u* proto_get_tx_data()
{
	int i = 0;
	
	for ( ; i < TELEBUFFER_MAX; i++ )
	{
		if ( tx_status[iTxTelePos] == TS_FREE )
		{
			tx_status[iTxTelePos] = TS_PREPARED;
			//memset( tx_tele+iTxTelePos, 0, sizeof(tx_tele[0]) );
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
	
	if ( rfm12_tx_status() != STATUS_FREE )
	{
		// darunter liegender Puffer voll
		iError = 2;
	}
	
	if ( &pTele->data != pData )
	{
		// uebergebener Puffer und angefordener Puffer verschieden (Mehrfachanforderung vom Puffer wird auf Platzgruenden nicht unterstuetzt)
		iError = 1;
	}
	
	if ( !iError )
	{
		pTele->adr_flags_src = (uint8_t)( iIsGateway ? AF_GATEWAY : 0 );
		pTele->adr_src = iNodeId;
				
		pTele->adr_flags_dest = (uint8_t)( iIsGateway ? 0 : AF_GATEWAY );
		pTele->adr_dest = 0;
			
		pTele->ttl = TTL_MAX;
		pTele->seqNr = ++iSeqNr;
		pTele->dataType = dataType;
		pTele->dataChecksum = getChecksum( pTele );
				
		iError = rfm12_tx( sizeof(*pTele), 0, (uint8_t*)pTele );
		if ( iError == RFM12_TX_ENQUEUED )
		{
			TRACE4( "tx (adrSrc=%d, adrDest=%d, ttl=%d, seqNr=%d)\r\n", pTele->adr_src, pTele->adr_dest, pTele->ttl, pTele->seqNr );
			
			tx_status[iTxTelePos] = TS_SEND;
			tx_retry[iTxTelePos] = 0;
			lTimeout[0] = TELE_TIMEOUT;	// sollte der Konten schneller senden als die Quittung emfangen, dann wird er beim vollen Telegrammpuffer gebremst
			lTimeout[1] = TELE_TIMEOUT;
		
			LED2_ON;
			led2_an = LED_LEUCHTDAUER;
		}
		else
		{
			TRACE1( "tx error (%d)", iError );
		}		
	}
		
	return iError;
}

void proto_tx_free( data_u* pData )
{
	tele_s* pTele = &tx_tele[iTxTelePos];
	
	if ( &pTele->data == pData )
	{
		tx_status[iTxTelePos] = TS_FREE;
	}	
}

/// getChecksum berechnet die Checksumme uber die Nutzdaten (kann ggf. komplexer gestaltet werden)
static uint8_t getChecksum( tele_s* pTele )
{
	uint8_t sum = 103;
	uint8_t* pT = (uint8_t*)&pTele->dataType;
	uint8_t i = 0;
	
	while ( i < (sizeof(pTele->data) + sizeof(pTele->dataType)) )
	{
		sum ^= pT[i];
		i++;
	}
	
	return sum;
}

uint8_t rx_telegram( tele_s* pTele )
{
	uint8_t bOk = 0;
	
	switch ( pTele->dataType )
	{
		case DT_INT_COUNTER:
			TRACE2( "rx: %d (adrSrc=%d, type=iC)\r\n", pTele->data.iValue, pTele->adr_src );
			bOk = 1;
			break;
			
		case DT_FLOAT_TEMP:
			TRACE1( "rx: %g (type=fT)\r\n", (double)pTele->data.fValue );
			bOk = 1;
			break;
			
		case DT_INT_ERROR:
			TRACE2( "rx: %cack (error=%d)\r\n", HAS_FLAG(pTele->adr_flags_src, AF_ACK) ? ' ':'n', pTele->data.iValue );
			bOk = 1;
			break;
			
		default:
			TRACE1( "rx: -?- (type=%d)\r\n", pTele->dataType );
			break;
	}		
	
	return bOk;
}

uint8_t fwd_telegram( tele_s* pRx_tele )
{
	uint8_t i = 0;
	uint8_t bFwd = 0;
	char* pDir = "fwd";
	
	// nur weiterleiten wenn TTL nicht abgelaufen
	pRx_tele->ttl--;
	if ( pRx_tele->ttl > 0 )
	{
		if ( ( pRx_tele->adr_flags_src & (AF_ACK | AF_NACK) ) > 0 )
		{
			// Fall 2: ein Antworttelegramm entsprechend des Pfads (beim Versand gefundene Route) weiterleiten
			if ( checkNodeInPath(pRx_tele, -1) > 0 )
			{
				bFwd = -1;
			}
		}
		else
		{
			// Fall 1: Ein Nutzdatentelegramm ggf weiterleiten
			// nur weiterleiten wenn nicht schonmal weitergeleitet von diesem Konten
			if ( !checkNodeInPath(pRx_tele, 1) > 0 )
			{
				bFwd = 1;
			}
		}				
	}
	
	if ( bFwd != 0 )
	{
		if ( rfm12_tx_status() == STATUS_FREE )
		{
			if ( bFwd < 0 )
			{
				pDir = (pRx_tele->adr_flags_src&AF_ACK) ? "ack" : "nack";
			}		
			TRACE4( "%s (adrSrc=%d, adrDest=%d, ttl=%d", pDir, pRx_tele->adr_src, pRx_tele->adr_dest, pRx_tele->ttl );

			for ( i = 0 ; (i < TELEPATH_MAX) && pRx_tele->path[i] ; i++ )
			{
				TRACE2( ", path[%d]=%d", i, pRx_tele->path[i] );
			}	
			TRACE0( ")\r\n" );
								
			rfm12_tx( sizeof(tele_s), 0, (uint8_t*)pRx_tele );
		
			return 0;  // weitergeleitet
		}
		else
		{
			// kein Puffer, Telegramm verwerfen
			TRACE0( "fwd error\r\n")
		}		
	}
	
	return 1;  // ok, aber nicht weitergeleitet
}

/// quittieren des empfangenen Telegramms
/// pTele:	empfangenes Telegramm (um RAM zu sparen) 
/// iError: 0=ACK, >0=NACK mit Fehlercode
uint8_t quit_telegram( tele_s* pTele, uint8_t iError )
{
	pTele->adr_dest = pTele->adr_src;
	pTele->adr_src = iNodeId;
	pTele->adr_flags_dest = pTele->adr_flags_src & AF_GATEWAY;
	
	pTele->adr_flags_src = (uint8_t)( iIsGateway ? AF_GATEWAY : 0 );
	if ( iError )
		pTele->adr_flags_src |= AF_NACK;
	else
		pTele->adr_flags_src |= AF_ACK;
		
	pTele->dataType = DT_INT_ERROR;	
	pTele->data.iValue = iError;	
	pTele->dataChecksum = getChecksum( pTele );
	
	pTele->ttl = TTL_MAX;
	
	if ( rfm12_tx_status() == STATUS_FREE )
	{
		if ( iError )	
		{
			TRACE3( "nack (adrSrc=%d, adrDest=%d, ttl=%d)\r\n", pTele->adr_src, pTele->adr_dest, pTele->ttl );
		}		
		else
		{
			TRACE3( "ack (adrSrc=%d, adrDest=%d, ttl=%d)\r\n", pTele->adr_src, pTele->adr_dest, pTele->ttl );
		}	
				
		rfm12_tx( sizeof(*pTele), 0, (uint8_t*)pTele );

		LED2_ON;
		led2_an = LED_LEUCHTDAUER;
		return 0;
	}
	return 1;		
}

/// pRx_tele: zu pruefendens Telegramm
/// bAppend:  0=nichts, 1=haenge den Knoten an Path an, -1=Konten aus Pfad entfernen
int8_t checkNodeInPath( tele_s* pRx_tele, int8_t iAppend )
{
	int i = 0;	
	
	for ( ; (i < TELEPATH_MAX) && pRx_tele->path[i] ; i++ )
	{
		if ( pRx_tele->path[i] == iNodeId )
		{
			if ( iAppend < 0 )  //Konten aus Pfad entfernen
			{
				for ( ; (i < (TELEPATH_MAX-1)) && pRx_tele->path[i] ; i++ )
				{
					pRx_tele->path[i] = pRx_tele->path[i+1];
				}
			} 
			return 1;
		}		
	}
	
	if ( iAppend > 0 )	// haenge den Knoten an Path an
	{
		if ( i < TELEPATH_MAX )
		{
			pRx_tele->path[i] = iNodeId;
			return 0;
		}
		else
		{
			return -1;	// Fehler: maximale Laenge des Pfads erreicht --> Telegramm verwerfen
		}
	}
	
	return 0;  // Konten nicht im Pfad vorhanden
}

/// nach Empfang der Quittung den Platz im Telegramm-Puffer freigeben
uint8_t free_telegram( tele_s* pTele )
{
	int i = 0;
	
	for ( i = 0; i < TELEBUFFER_MAX; i++ )
	{
		if ( ( tx_status[i] == TS_SEND ) && ( tx_tele[i].seqNr == pTele->seqNr ) )
		{
			TRACE2( "%s (seqNr=%d)\r\n", pTele->adr_flags_src & AF_ACK ? "ack" : "nack", pTele->seqNr );
			tx_status[i] = TS_FREE;
			return 0;  // freigegeben
		}
	}
	
	return -1;	// nicht gefunden
}

/// Den Telegrammpuffer auf nicht quittierte Telegramme ueberprufen.
/// Ggf. das Telegramm wiederholen oder den Puffer einfach freigeben
uint8_t timeout_telegram()
{
	BOOL bFree = FALSE;
	
	for ( int i = 0; i < TELEBUFFER_MAX; i++ )
	{
		bFree = FALSE;
		
		if ( tx_status[i] == TS_SEND ) 
		{
			if ( tx_retry[i] < TELE_RETRY_MAX )
			{
				if ( rfm12_tx_status() == STATUS_FREE )  // wenn kein Platz im darunterliegenden Puffer dann Telegramm verwerfen
				{
					tx_retry[i]++;
					TRACE2( "tx retry (seqNr=%d, retry=%d)\r\n", tx_tele[i].seqNr, tx_retry[i] );
					rfm12_tx( sizeof(tx_tele[i]), 0, (uint8_t*)tx_tele+i );
				}
				else
				{
				//	bFree = TRUE;
				}
			}
			else
			{
				bFree = TRUE;
			}
		}

		if ( bFree )
		{
			TRACE1( "error: no ack (seqNr=%d)\r\n", tx_tele[i].seqNr );
			tx_status[i] = TS_FREE;
			tx_retry[i] = 0;
		}			
	}	
	return 0;
}