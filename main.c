 /*Attiny13A Qi wireless power receiver. PB4 to be connected to mosfet gate
	for modulation....
	PB3 to be connected to rectified DC voltage via 4.7/47k divider).
	
    Copyright (C) <2019>  <Vinod S http://blog.vinu.co.in>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.*/
	
#include <avr/io.h>
#define F_CPU 9600000
#include <util/delay.h>
#include <avr/power.h>
#include <avr/interrupt.h>
# define HIGH 1
# define LOW 0

void digitalWrite(uint8_t pin, uint8_t state) {
  if (state) PORTB |= 1 << PB4;
  else PORTB &= ~(1 << PB4);
}

void adc_init(void) {
  ADMUX = 1 << REFS0;
  ADMUX |= 3;
  ADCSRA = (1 << ADEN) | (1 << ADPS2);
}

uint16_t adc_read(void) {
  ADCSRA |= 1 << ADSC;
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

volatile uint8_t bit_state = 0;
void tx_byte(uint8_t data) {
  bit_state ^= 1;
  digitalWrite(2, bit_state);
  _delay_us(250);
  digitalWrite(2, bit_state);
  _delay_us(250);

  uint8_t parity = 0;
  for (int i = 0; i < 8; i++) {
    bit_state ^= 1;
    digitalWrite(2, bit_state);
    _delay_us(250);
    if (data & (1 << i)) {
      parity++;
      bit_state ^= 1;
    }
    digitalWrite(2, bit_state);
    _delay_us(250);
  }

  if (parity & 1) {
    parity = 0;
  } else
    parity = 1;

  bit_state ^= 1;
  digitalWrite(2, bit_state);
  _delay_us(250);

  if (parity) {
    bit_state ^= 1;
  }
  digitalWrite(2, bit_state);
  _delay_us(250);

  bit_state ^= 1;
  digitalWrite(2, bit_state);
  _delay_us(250);
  bit_state ^= 1;

  digitalWrite(2, bit_state);
  _delay_us(250);

}

void tx(uint8_t * data, int len) {
  uint8_t checksum = 0;
//  static uint8_t state = 0;
  for (int i = 0; i < 15; i++) {
    digitalWrite(2, HIGH);
    _delay_us(250);
    digitalWrite(2, LOW);
    _delay_us(250);
  }
  bit_state = 0;
  for (int i = 0; i < len; i++) {
    tx_byte(data[i]);
    checksum ^= data[i];
  }
  tx_byte(checksum);
}

static uint16_t adcvb[2];
int main() {

  clock_prescale_set(clock_div_1); //set clock to 9.6MHz, no prescaler
  adc_init();
  DDRB |= 1 << PB4;

  volatile uint8_t state = 0;
  while (1) {

    uint16_t adcv = adc_read();
    if (adcv < 423) {
      state = 0;
    }

    switch (state) {
    case 0:
      {
        uint16_t adcv = adc_read();
        if (adcv > 423) {
          state = 1;
          _delay_ms(10);
        }
        break;
      }
    case 1:
      {
        uint8_t dt[] = {
          0x1,
          255
        };
        for (int i = 0; i < 20; i++) {
          tx(dt, 2);	//send ping response so that the transmitter identifies receiver.
          _delay_ms(10);
        }
        state = 2;
        break;
      }

    case 2:
      {
        //if(adcv > ((423*3/2))) {
        int8_t error = 0;
        uint16_t adcv = adc_read();
        int16_t temp_error = 0;
        adcvb[0] = adcvb[1];
        adcvb[1] = adcv;
        //if(abs(adcvb[0] - adcvb[1]) > 20)
        // temp_error = (int16_t)((423* 3) - adcv);
        //else 
        temp_error = (int16_t)((423 * 2) - adcv);	//1.1v adc reference. 423 equals to 5V. (4.7/47K voltage divider)

        temp_error /= 5;
        if (temp_error > 127) temp_error = 127;
        if (temp_error < -128) temp_error = -128;
        error = (int8_t) temp_error;
        uint8_t dt[] = {
          0x3,
          (int8_t) error
        };
        tx(dt, 2);	//send error correction packet. 0x03 is error correction packet header. 1 BYTE payload, check WPC documents for more details.
        /*} else {
         uint8_t dt[] = {0x3,(int8_t)1};
         tx(dt,2);
        }*/
      } {
        uint8_t dt[] = {0x4, 0XFF};
        tx(dt, 2);	//received power indication packet. I am not sure if this is needed or not. Please read the WPC document
					//for more details. I jut implemented and it worked. But I am not sure if this is the proper way to do it.
      }
      //    _delay_ms(10);
      break;

    }
  }
}