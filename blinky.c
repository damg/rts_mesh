#include <avr/io.h>

int
main(void)
{
  int i;
  DDRD = 0xFF;
  PORTD = 0x55;
  while(1)
    {
      PORTD = 0x55;
      for(i = 0; i < 15000; ++i);
      PORTD = 0xCC;
      for(i = 0; i < 15000; ++i);
    }

  return 0;
}
