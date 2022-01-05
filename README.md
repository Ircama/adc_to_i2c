# Arduino-based ADC to I2C converter

This Arduino code allows extending the Raspberry Pi with a simple 16-bit ADC support implemented through the integration of an Arduino-based ATTINY85/88 device interfaced via I2C (4-wire bus).

Tested with an ATTINY88 device. It should also work with ATTINY85, Digispark, or other Arduino devices.

While this code allows easy extensions and customizations (it is also fast to serve I2C readings), it is very slow in updating the 16-bit ADC samples. If additional performance is needed with 16-bit sampling, use a specific ADC to I2C hw converter instead, like the ADS1115.

Features:

- Read a configurable number and sequence of 10-bit hw analog ports (A0 to An)
- Perform n-bit sw oversampling via [Arduino Oversampling And Decimation Library](https://github.com/regimantas/Oversampling) with averaging
- Provide I2C slave interface to a Raspberry Pi
- Include calculation of LM35 centigrade temperature analog sensors (to read appropriate temperature values, it is important to predefine the power supply voltage in the code; e.g.  3.3V or 5V, ref. `#define VREF`)
- Decouple the queries performed by the Raspberry Pi with the rate of the ADC reading process (the latter taking seconds in case of several oversampled values without pre-scaling).

Default configuration:

- 16 bit oversampling with averaging of two samples (each oversampling reading returns the average of two oversampled values)
- set the ADCSRA pre-scaler register to 64 instead of 128, to increase the sampling rate; notice anyway that for reading ambient temperatures (which generally have very slow variations), keeping the pre-scaler to 128 (safe value) is suggested to always ensure max read quality. Using a pre-scaler of 16 or 8 might still work in certain circumstances, highly improving the sampling rate, where the reduced quality is compensated by the 16-bit oversampling.
- Preconfigured 4 ADC input ports (available with ATTINY88), which can be redefined via `#define NUM_ADC 4`
- Used ports: 0, 1, 2, 3 (which can be redefined by changing `int adc_pins[] = {0, 1, 2, 3};`)
- 3.3V power supply (which can be changed via `#define VREF 3.3`)
- Use I2C 2A slave address (which can be redefined by changing `#define SLAVE_ADDRESS 0x2A`)
- Upon each I2C query, the code returns a stream of 32 bytes corresponding to 8 numbers: 4 single-precision floating-point values (taking 4 bytes) and 4 unsigned long integers (also taking 4 bytes). The first 4 numbers are the LM35 temperatures in Celsius degrees, the other 4 numbers are the raw oversampled readings.
- The 4 returned raw ADC values are integers between 0 and 65535 (16 bit values).

## Used libraries

 - https://github.com/regimantas/Oversampling (modified for managing 32-bit unsigned long return vlues, ref. https://github.com/regimantas/Oversampling/pull/2/files)
 - Default Arduino Wire library (I2C master/slave)
 
# Raspberry Pi interface

Installing pip3: `sudo apt-get install python3-pip`

Installing smbus: `sudo apt-get install python3-smbus python3-dev`

```python
import struct
import smbus
import time

bus = smbus.SMBus(1)
address = 0x2a
data_array = bus.read_i2c_block_data(address, 0, 4 * 2 * 4) # 4 ADC ports, 2 numbers per each ADC port, 4 bytes per each "float" or "unsigned long" value 

temperature = []
for i in [data_array[x:x+4] for x in range(0, int(len(data_array)/2), 4)]:
    temperature.append(struct.unpack('<f', bytes(i))[0])

adc_value = []
for i in [data_array[x:x+4] for x in range(int(len(data_array)/2), len(data_array), 4)]:
    adc_value.append(struct.unpack('<L', bytes(i))[0])

print(temperature, adc_value)

```

Note:

The Arduino "Oversampling And Decimation Library" has been modified to manage 32-bit unsigned long return values, ref. https://github.com/regimantas/Oversampling/pull/2/files)
