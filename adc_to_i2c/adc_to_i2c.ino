/*
 * Used Libraries:
 * https://github.com/regimantas/Oversampling (modified for managing 32-bit unsigned long return vlues, ref. https://github.com/regimantas/Oversampling/pull/2/files)
 * Default Wire library (I2C master/slave)
 */

#include <Oversampling.h>

/*
 * Oversampling(int adcbytes, int samplebytes, int Averaging)
 * https://github.com/regimantas/Oversampling
 * first parameter: adcbytes (always 10 on Attiny85 or Attiny88: min = 0, max = 1023)
 * second parameter: samplebytes (target bytes to get from oversampling; could be a value between 11 and 24; typically 12 to double precision)
 * third parameter: Averaging (it performs the sdampling more times, then computing the averaged value; 1 = single oversampling; 2 = average of two oversampling)
 */
Oversampling adc(10, 16, 2); // Adc Bytes 10 or 12, Oversampling Bytes 11-24, 2 is the averaging of more samples.

/* 10 = Resolution of the ATTINY85/ATTINY88 ADC (= 10 bit)
 * 16 = number of bits obtained with oversampling (e.g., from 10 bits to 16 bit)
 * 2 = number of consecutive taken oversampled readings before returning the value, having calculated the average
 *
 * Here is how to map the first value (MULTIPLIER) of the divisor ("divisor) to the oversampling value.
 * Oversampling -> first value of the divisor (Oversampling bytes = number)
 * 16 = 64
 * 15 = 32
 * 14 = 16
 * 13 = 8
 * 12 = 4
 * 11 = 2
 */

/*
 * The following settings convert LM32 values to temperatures, basing on the VREF voltage
 */
//#define VREF 5.0 // Volt
#define VREF 3.3 // Volt
#define MULTIPLIER 64 // see above (depends on the number of bits defined for the oversampling)
const float divisor = (float) MULTIPLIER / (VREF / 1024 * 100);

/* The following two parameters define:
 * - the number of ADCs to read
 * - the mapping of the ADC number and its port
 * NUM_ADC must be less than or equal to the number of elements in adc_pins.
 * The higher NUM_ADC, the longer the time to refresh the ADC readings
 */
#define NUM_ADC 4
int adc_pins[] = {0, 1, 2, 3};

typedef struct {
  float temp[NUM_ADC]; // temperature
  unsigned long adc_value[NUM_ADC]; // raw value
} type_temp;

type_temp temp_set, out_set;

/* The following parameter defines whether the ADC prescaler should be
 * configured with a value other than the default (128). Minor is
 * the value, higher the frequency (input clock frequency to the ADC)
 * shorter the reading time (higher sample rate), greater the error in
 * case of rapid fluctuations of the analog reading.
 * With clock of 16MHz, you should not use a lower divider than 16.
 */
#define FASTADC 1

/* I2C configuration and relative port on which the ADC readings are
 * transmitted
 */
#include <Wire.h>
#define SLAVE_ADDRESS 0x2A

#define I2C_HARDWARE 1
#define I2C_TIMEOUT 10
#define I2C_MAXWAIT 10
#define I2C_PULLUP 1
#define I2C_FASTMODE 1
#define SDA_PORT PORTC
#define SDA_PIN 4 // = A4
#define SCL_PORT PORTC
#define SCL_PIN 5 // = A5

/*-------------------------------------------------------------------------------------------------------*/
void setup() {
  // This setting modifies precision and performance
  #if FASTADC // Default ADCSRA (FASTADC unset) is 128 (slow)
    ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear prescaler bits
    
    // uncomment as required
    
  //  ADCSRA |= bit (ADPS0);                               //   2  
  //  ADCSRA |= bit (ADPS1);                               //   4  
  //  ADCSRA |= bit (ADPS0) | bit (ADPS1);                 //   8  
  //  ADCSRA |= bit (ADPS2);                               //  16 
  //  ADCSRA |= bit (ADPS0) | bit (ADPS2);                 //  32 
    ADCSRA |= bit (ADPS1) | bit (ADPS2);                 //  64 
  //  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // 128 (default)
  
  // https://github.com/stylesuxx/Oversample#implementation
  
  #endif

  /*
  The DIDR (Data Input Disable Register) disconnects the digital inputs from
  whichever ADC channels you are using. This is important for 2 reasons. First
  off, an analog input will be floating all over the place, and causing the
  digital input to constantly toggle high and low. This creates excessive noise
  near the ADC, and burns extra power. Secondly, the digital input and associated
  DIDR switch have a capacitance associated with them which will slow down your
  input signal if you are sampling a highly resistive load.
  */
  for (int i=0; i< NUM_ADC; i++)
    DIDR0 |= 1<<adc_pins[i];

  Wire.begin(SLAVE_ADDRESS);
  //By default .begin() will set I2C SCL to Standard Speed mode of 100kHz
  Wire.setClock(400000UL); // set I2C SCL to High Speed Mode of 400kHz
  Wire.onRequest(sendData);
} // setup

void sendData() {
  Wire.write((byte*)&out_set, sizeof(type_temp)); // stored bit per number=4; numbers = NUM_ADC; types = unsigned long, float = 32 bit = 4 bytes
}

/*-------------------------------------------------------------------------------------------------------*/

void loop() {
  for (int i=0; i< NUM_ADC; i++) {
    temp_set.adc_value[i] = adc.read(adc_pins[i]); // Oversampling and decimation
    temp_set.temp[i] = (float) temp_set.adc_value[i] / divisor;
  }
  noInterrupts();
  memcpy( (void*)&out_set, (void*)&temp_set, sizeof(type_temp) );
  interrupts();
}
