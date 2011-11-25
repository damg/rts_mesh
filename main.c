#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include <rfm12_config.h>
#include <rfm12.h>

int
main(void)
{
  uint8_t teststring[] = "teststring\r\n";
  uint8_t packettype = 0xEE;
  rfm12_init();
  sei();

  while(1)
    {
      rfm12_tx (sizeof(teststring), packettype, teststring);
      rfm12_tick();
    }

  return 0;
}
