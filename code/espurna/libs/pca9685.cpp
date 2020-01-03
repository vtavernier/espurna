#include <string.h>

#include "pca9685.h"

PCA9685Pwm::PCA9685Pwm() : _pwm() { memset(_pins, 0, sizeof(_pins)); }

void PCA9685Pwm::begin() {
	Wire.setClock(700000);
	_pwm.begin();
	_pwm.setPWMFreq(200);
}

void PCA9685Pwm::setChannel(uint8_t channel, uint16_t value) {
	_pins[channel] = value;
}

void PCA9685Pwm::update() {
	for (unsigned char i = 0; i < sizeof(_pins) / sizeof(_pins[0]); ++i) {
		_pwm.setPin(i, _pins[i], false);
	}
}

