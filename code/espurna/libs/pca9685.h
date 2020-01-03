#ifndef _PCA9685_H_
#define _PCA9685_H_

#include <Adafruit_PWMServoDriver.h>
#include <stdint.h>

#define PCA9685_PINS 16

class PCA9685Pwm {
	Adafruit_PWMServoDriver _pwm;
	uint16_t _pins[PCA9685_PINS];

       public:
	PCA9685Pwm();

	void begin();
	void setChannel(uint8_t channel, uint16_t value);
	void update();
};

#endif  // _PCA9685_H_
