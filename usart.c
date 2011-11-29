/*
 * Baudy.c
 *
 * Created: 10.10.2011 20:09:40
 *  Author: mf
 * Verwendung des USART, serielle Schnittstelle in mehreren Varianten, Baudy light und Baudy Comfort
 */ 

#include <avr/io.h>
#include <stdio.h>

#include "usart.h"

//#define FOSC 3686400UL
#define FOSC 10000000UL
#define BAUD9600 9600UL

int
uart_putchar(char c, FILE *stream);   

int
uart_getchar(FILE *stream);

#ifdef UART_TO_STDIO
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW); 
#endif

void
uart_init(unsigned long baud)
{
	unsigned long ubrr = FOSC / 16 / baud - 1;
	/* Set baud rate */
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char) ubrr;
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);

	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);  // Asynchron 8N1
	
#   ifdef UART_TO_STDIO
	stdout = stdin = &uart_str;
#   endif
	
}

void
uart_init9600(void)
{
	return uart_init(BAUD9600);
}

 
// Baudy Comfort: Definition der Ausgabefunktion
int
uart_putchar( char c, FILE *stream )
{
    if( c == '\n' )
        uart_putchar( '\r', stream );
 
    loop_until_bit_is_set( UCSRA, UDRE );
    UDR = c;
    return 0;
}

// Baudy Comfort: für Eingabe ?ber scanf und umgeleitetem stdout
int
uart_getchar(FILE *stream)
{
   unsigned char ch;    
   while (!(UCSRA & (1<<RXC)));
   ch=UDR;  

   /* Echo the output back to the terminal */
   uart_putchar(ch,stream);      

   return ch;
}

// Baudy Light: primitive Ausgabefunktion
void 
USART_Transmit(unsigned char data)
{
	// wait for empty transmit buffer
	while (!(UCSRA & (1<<UDRE)))
	{
		
	}
	UDR=data;
}


// Baudy Light: primitive Eingabefunktion
unsigned char
USART_Receive(void)
{
	//wait for data to be received
	while (! (UCSRA & (1<<RXC)))
	;
	//get and return received data from buffer
	return UDR;
}

// Baudy Light extended: Stringausgabe für Baudy light
void uart_puts (char *s)
{
    while (*s)  /* so lange *s != '\0' also ungleich dem "String-Endezeichen(Terminator)" */
    {  
        USART_Transmit(*s);
        s++;
    }
}

/*
int ___main(void)
{

//	Baudy light:
	char s[8]="hello!\n\r";
	int i;
	int iin;


	unsigned char c;
	
	DDRD=0x62;
	PORTD|=0x60;
	
	USART_Init ( MYUBRR );
	
// Baudy comfort: Ausgabe
	stdout = stdin = &uart_str;
	printf("Hello World \n");
	printf("=========== \n");
	
// Baudy light:	Ausgabe "hello":
	for(i=0;i<8;i++)
	{
		USART_Transmit(s[i]);
	}
	

// Baudy light extended: auch so k?nnte man "hello" ausgeben:
	uart_puts(s);
	
		PORTD&=~(0x60); // nur'n L?mpchen schalten als Test
		
    while(1)
    {
//		Baudy light: auf Eingabe warten     
//		c=USART_Receive();
		//	PORTD|=0x60;  // testl?mpchen wieder aus.
//		Baudy light: erhaltenes Zeichen wieder senden (Echo)
//		USART_Transmit(c);	

// Baudy Comfort:
		printf("Enter your Number: ");
		scanf("%d",&iin);
		printf("\n Your number is: %d \n",iin);
    }
}
*/