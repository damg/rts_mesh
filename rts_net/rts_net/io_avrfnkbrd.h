#ifndef __AVR_FUNKBOARD_ID_HEADER__
#define __AVR_FUNKBOARD_ID_HEADER__

// allgemeine Definitionen
#define FALSE				0
#define TRUE				!FALSE
#define BOOL				uint8_t

// Definition der IO's des AVR-Funktboards

#define INIT_LED			DDRD |= ((1 << PD5) | (1 << PD6));			
#define LED1_ON				PORTD |= (1 << PD6);
#define LED1_OFF			PORTD &= ~(1 << PD6);
#define LED2_ON				PORTD |= (1 << PD5);
#define LED2_OFF			PORTD &= ~(1 << PD5);

#define INIT_TASTER			DDRB &= ~(1 << PB1);
#define TASTER1				(PINB & (1 << PINB1))


#endif //__AVR_FUNKBOARD_ID_HEADER__
