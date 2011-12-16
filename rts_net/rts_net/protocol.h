#ifndef __RTS_PROTOCOL_HEADER__
#define __RTS_PROTOCOL_HEADER__

#include <inttypes.h>

#define TELEBUFFER_MAX		2
#define TELE_RETRY_MAX		3
#define TELE_TIMEOUT		20000
#define TELEPATH_MAX		3
#define TTL_MAX				5	

#define AF_GATEWAY			0x01
#define AF_ACK				0x02
#define AF_NACK				0x04

typedef enum dataType_etag
{
	DT_NONE,
	DT_INT_COUNTER,					/// Nutzdaten Integer (Zaehlerwert)
	DT_FLOAT_TEMP,					/// Nutzdaten Double (Temperatur)
	DT_INT_ERROR,					/// Fehlercode Integer
	DT_MAX
} dataType_e;

typedef enum teleStatus_etag
{
	TS_FREE,						/// Telegrammplatz frei bzw. altes Telegramm quittiert
	TS_PREPARED,					/// Telegrammplatz angefordert/reserviert
	TS_SEND,						/// Telegramm(platz) versendet und warten auf Quittung
	TS_MAX	
} teleStatus_e;

typedef union data_utag
{
	float fValue;
	uint16_t iValue;		
} data_u;			

typedef struct tele_stag
{
	uint8_t adr_flags_src;			/// Flags fuer Broadcast, Gateway-Verbindung (Sender)
	uint8_t adr_src;				/// Knotenadresse 1 - 255 (Sender)

	uint8_t adr_flags_dest;			/// Flags fuer Broadcast, Gateway-Verbindung (Empfaenger)
	uint8_t adr_dest;				/// Knotenadresse 1 - 255 (Empfaenger)
	
	uint8_t path[TELEPATH_MAX];		/// Route des Telegramms (ausgebaut beim Transport, genutzt bei Antwort)
	uint8_t ttl;					/// Time To Life (Countdounzaehler fuer Weiterleitung)
	uint8_t seqNr;					/// Sequenz-Nummer des Telegramms
	
	dataType_e dataType;			/// Typ der Nutzdaten 'data' (muss direkt vor 'data' kommen)
	data_u data;					/// Nutzdaten
	uint8_t dataChecksum;			/// Checksumme ueber die Nutzdaten ('dataType'+'data')
} tele_s;

extern uint8_t iNodeId;

extern uint8_t proto_init();											/// initialisiert das Protokoll-Modul
extern uint8_t proto_setup( uint8_t service );							/// konfiguriert den Konten und dient zur Konfiguration des Moduls per RS232
extern uint8_t proto_cycle();
extern BOOL	proto_is_gateway();

extern data_u* proto_get_tx_data();
extern uint8_t proto_tx_data( data_u* pData, dataType_e dataType );
extern void    proto_tx_free( data_u* pData );


#endif // __RTS_PROTOCOL_HEADER__
