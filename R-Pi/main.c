/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "math.h"

/* Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (On Pico this is GP5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
*/

// By default these devices  are on bus address 0x68
static int addr = 0x68;
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define UART_ID uart0
#define BAUD_RATE 9600
#define BUTTON_PINA 19
#define BUTTON_PINB 18


#ifdef i2c_default
static void mpu6050_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x00};
    i2c_write_blocking(i2c0, addr, buf, 2, false);
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c0, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c0, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c0, addr, &val, 1, true);
    i2c_read_blocking(i2c0, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c0, addr, &val, 1, true);
    i2c_read_blocking(i2c0, addr, buffer, 2, false);  // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}
#endif



int main() {
    stdio_init_all();
    
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    gpio_init(BUTTON_PINA);
    gpio_init(BUTTON_PINB);
    gpio_set_dir(BUTTON_PINA, GPIO_IN);
    gpio_set_dir(BUTTON_PINB, GPIO_IN);
    gpio_pull_down(BUTTON_PINA);
    gpio_pull_down(BUTTON_PINB);
    
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(28);
    // Select ADC input 0 (GPIO26)
    adc_select_input(2);
    
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    // Make the I2C pins available to picotool
   // bi_decl(bi_2pins_with_func(I2C_SDA_PIN, I2C_SCL_PIN, GPIO_FUNC_I2C));

    mpu6050_reset();
    const float conversion_factor = 3.3f / (1 << 12);
    int16_t acceleration[3], gyro[3], temp, previousA;
    char result[9] = {0};
    char result_adc[9] = {0};
    char buffer[9] = {' ',' ',' ',' ',' ',' ',' ',' ',' '};
    float counter=1;
    previousA = gpio_get(BUTTON_PINA);
    while (1) {
    mpu6050_read_raw(acceleration, gyro, &temp);
    uint16_t raw_adc = adc_read();
    //float raw_float_adc = raw_adc*conversion_factor;
    float roll_rad = atan2(-acceleration[0], sqrt((acceleration[2]*acceleration[2])+(acceleration[1]*acceleration[1])));
	if(gpio_get(BUTTON_PINA) != previousA){
		if(gpio_get(BUTTON_PINA) != gpio_get(BUTTON_PINB)){
			counter--;
			if(uart_is_writable(UART_ID)){
         			gcvt(counter, 6, buffer);
         			gcvt(roll_rad, 6, result);
         			//gcvt(raw_float_adc, 6, result_adc);
         			sprintf(result_adc, "%d", raw_adc);
         			/*result[6] = ' ';result[7]=' ';
         			buffer[6] = ' ';
         			result[8] = '\n';
			        buffer[8] = '\n';	*/
			        strcat(result,",");
			        strcat(result,result_adc);
			        strcat(result,",");
			        strcat(result,buffer);
			        strcat(result,"\n");
			        uart_puts(UART_ID,result);		
	 			//uart_puts(UART_ID,result);
				//strcat(result_adc,",\n");		
	 			//uart_puts(UART_ID,result_adc);
	  			//strcat(buffer,",\n");
	 			//uart_puts(UART_ID,buffer);
	  			sleep_ms(200);
	 		}
		}	
		else{
			counter++;
			if(uart_is_writable(UART_ID)){
         			gcvt(counter, 6, buffer);
         			gcvt(roll_rad, 6, result);
         			//gcvt(raw_float_adc, 6, result_adc);         			
         			sprintf(result_adc, "%d", raw_adc);
         			/*result[6] = ' ';result[7]=' ';
         			buffer[6] = ' ';
         			result[8] = '\n';
			        buffer[8] = '\n';	*/
			        strcat(result,",");
			        strcat(result,result_adc);
			        strcat(result,",");
			        strcat(result,buffer);
			        strcat(result,"\n");
			        uart_puts(UART_ID,result);
			        //strcat(result,",\n");		
	 			//uart_puts(UART_ID,result);
				//strcat(result_adc,",\n");		
	 			//uart_puts(UART_ID,result_adc);
	  			//strcat(buffer,",\n");
	 			//uart_puts(UART_ID,buffer);
	  			sleep_ms(200);
	 		}		
		}
	}
	previousA = gpio_get(BUTTON_PINA);     
         //strcat(buffer, result);
    }

    return 0;
}
     // These are the raw numbers from the chip, so will need tweaking to be really useful.
        // See the datasheet for more information
        //printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
        //printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
        // Temperature is simple so use the datasheet calculation to get deg C.
        // Note this is chip temperature.
        //printf("Temp. = %f\n", (temp / 340.0) + 36.53);
