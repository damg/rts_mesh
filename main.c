#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include <rfm12_config.h>
#include <rfm12.h>

int
main(void)
{
  rfm12_init();
  return 0;
}
