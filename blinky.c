#include <avr/io.h>

int
main(void)
{
  int i;
  DDRB = 0xFF;
  DDRC = 0xFF;
  DDRD = 0xFF;
  while(1)
    {
      PORTB = 1;
      PORTC = 1;
      PORTD = 1;
      for(i = 0; i < 15000; ++i);
      PORTB = 0;
      PORTC = 0;
      PORTD = 0;
      for(i = 0; i < 15000; ++i);
    }

  return 0;
}
