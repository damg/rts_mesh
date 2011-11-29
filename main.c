#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include <rfm12_config.h>
#include <rfm12.h>

#define UART_TO_STDIO

#include "usart.h"

int
main(void)
{
  uint8_t teststring[] = "teststring\r\n";
  uint8_t packettype = 0xEE;
  
	uart_init9600();
  
  rfm12_init();
  sei();
  
  while(1)
    {
#ifdef SENDER    
      rfm12_tx (sizeof(teststring), packettype, teststring);
#else
		rf
	
      rfm12_rx (sizeof(teststring), packettype, teststring);
	  
	  teststring[ sizeof(teststring)-1 ] = 0;
	  printf( "rx: %s\r", teststring );
	  
#endif	  
      rfm12_tick();
	  
	  
    }

  return 0;
}
