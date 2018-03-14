// bcd-clock.c
//
// Reading the local time from the Pi, and showing it on screen with a MAX7219 display driving a 6x4 LED matrix.
// When combined with NTP doing time update, should give me a very accurate clock.
//
// Kris Adcock, Jan/Feb 2018. See http://www.danceswithferrets.org/geekblog/
//
// To compile:
//   gcc -o bcd-clock bcd-clock.c -l bcm2835
//
// To run:
//   sudo ./bcd-clock [brightness]          (Might not even need the sudo.)
//
//                                    [brightness] if provided, must be a number between 1 and 15.
//

#include <bcm2835.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

//   pin 1 on MAX (Data In)   -> pin 11 on Ras-Pi
//   pin 12 on MAX (Load /CS) -> pin 13 on Ras-Pi
//   pin 13 on MAX (clock)    -> pin 12 on Ras-Pi

// note that in defined RPI_GPIO_P1_xx, the _XX is the pin-number on the board!
#define kPin_Max7219_DataIn   RPI_GPIO_P1_11  //  GPIO 17   
#define kPin_Max7219_CS       RPI_GPIO_P1_13  //  GPIO 27
#define kPin_Max7219_Clock    RPI_GPIO_P1_12  //  GPIO 18

const uint8_t max7219_reg_noop        = 0x00;
const uint8_t max7219_reg_digit0      = 0x01;
const uint8_t max7219_reg_digit1      = 0x02;
const uint8_t max7219_reg_digit2      = 0x03;
const uint8_t max7219_reg_digit3      = 0x04;
const uint8_t max7219_reg_digit4      = 0x05;
const uint8_t max7219_reg_digit5      = 0x06;
const uint8_t max7219_reg_digit6      = 0x07;
const uint8_t max7219_reg_digit7      = 0x08;
const uint8_t max7219_reg_decodeMode  = 0x09;
const uint8_t max7219_reg_intensity   = 0x0a;
const uint8_t max7219_reg_scanLimit   = 0x0b;
const uint8_t max7219_reg_shutdown    = 0x0c;
const uint8_t max7219_reg_displayTest = 0x0f;

void SetupBasics(int iBrightness);
unsigned int millis(void);
void SetDigit(uint8_t digit, uint8_t val);
uint8_t int_to_bcd(int in);
void Write(uint8_t reg, uint8_t col);
void SendByte(uint8_t data);

static unsigned long long epoch;

int main(int argc, char **argv)
{
	int iBrightness = 8;
	
	if (argc > 1)
	{
		iBrightness = atoi(argv[1]);
		if (iBrightness < 1) iBrightness = 1;
		if (iBrightness > 15) iBrightness = 15;
	}
	
	struct timeval tv;
	gettimeofday (&tv, NULL);
	epoch = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;

	if (!bcm2835_init())
	{
		printf("bcm2835_init failed!\n");
		return 1;
	}

	bcm2835_gpio_fsel(kPin_Max7219_DataIn, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(kPin_Max7219_Clock, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(kPin_Max7219_CS, BCM2835_GPIO_FSEL_OUTP);
		
	bcm2835_gpio_write(kPin_Max7219_DataIn, HIGH);
	bcm2835_gpio_write(kPin_Max7219_Clock, HIGH);
	bcm2835_gpio_write(kPin_Max7219_CS, HIGH);
	
	SetupBasics(iBrightness);
	
	for (int digit = 0; digit < 8; ++digit)
	{
		SetDigit(digit, 0);
	} 
	
	uint8_t uOldSeconds = 0xFF;
	unsigned int uLastButtonTime = millis();
	
	while (1)
	{
		time_t t;
		struct tm tm;
		
		// we regularly re-setup the screen, just in case of an intermitten connection problem that resets the screen. Not really critical, to be honest.
		SetupBasics(iBrightness);
		
		// keep reading the time until it DIFFERS from the last time we read.
		while (1)
		{
			t = time(NULL);
			tm = *localtime(&t);
			if (tm.tm_sec != uOldSeconds) break;
			bcm2835_delay(50); // this stops the code sitting in a tight-loop and hogging all the CPU. Number could be increased, at the possible cost of the screen being a fraction of a second delayed from the true time.
		}
		
		uOldSeconds = tm.tm_sec;
		
		uint8_t uSecondsBCD = int_to_bcd(tm.tm_sec);
		uint8_t uHoursBCD = int_to_bcd(tm.tm_hour);
		uint8_t uMinutesBCD = int_to_bcd(tm.tm_min);
  
		int digit[6];
		digit[5] = (uHoursBCD & 0xF0) >> 4;
		digit[4] = (uHoursBCD & 0x0F);
		digit[3] = (uMinutesBCD & 0xF0) >> 4;
		digit[2] = (uMinutesBCD & 0x0F);
		digit[1] = (uSecondsBCD & 0xF0) >> 4;
		digit[0] = (uSecondsBCD & 0x0F);
		
		for (int x = 5; x >= 0; --x)
		{
			SetDigit(x, digit[x]);
		}
	}

	bcm2835_close();

    return 0;
}

void SetupBasics(int iBrightness)
{
	Write(max7219_reg_scanLimit, 0x05);      
	Write(max7219_reg_decodeMode, 0x00); // using IC in matrix mode (not seven-segment mode)
	Write(max7219_reg_shutdown, 0x01); // not in shutdown mode
	Write(max7219_reg_displayTest, 0x00); // no display test, thanks
	Write(max7219_reg_intensity, iBrightness);
}

unsigned int millis(void)
{
	struct timeval now;
	unsigned long long ms;    
	
	gettimeofday(&now, NULL);
	
	ms = (now.tv_sec * 1000000 + now.tv_usec) / 1000 ;
	
	return ((uint32_t) (ms - epoch ));
}

// digits 0 to 7 are registers 1 to 8.
void SetDigit(uint8_t digit, uint8_t val) {Write(digit + 1, val);}

uint8_t int_to_bcd(int in) {return ((in / 10) << 4) | (in % 10);}

// writes a byte to a register. The registers have constants defined in the header.
void Write(uint8_t reg, uint8_t col)
{    
  bcm2835_gpio_write(kPin_Max7219_CS, LOW);
  SendByte(reg);
  SendByte(col);
  bcm2835_gpio_write(kPin_Max7219_CS, HIGH);
}

// low-level bit-banging
void SendByte(uint8_t data)
{
  // sends byte out on m_iDataIn, one bit at a time (highest bit first)
  for (uint8_t i = 8; i > 0; --i)
  {   
    bcm2835_gpio_write(kPin_Max7219_Clock, LOW);
    bcm2835_gpio_write(kPin_Max7219_DataIn, (data & (1 << (i - 1))) ? HIGH : LOW); 
    bcm2835_gpio_write(kPin_Max7219_Clock, HIGH);
  }
}
