#define PCA9685_MAPPING 1, 0, 3, 2, 9, 8, 11, 10, 13, 14, 15, 12, 5, 6, 7, 4
#define PCA9685_COUNT 4
#define MANUFACTURER "vtavernier"
#define DEVICE "droidlight"

#define LIGHT_PROVIDER LIGHT_PROVIDER_PCA9685
#define LIGHT_CHANNELS 4
#define LIGHT_MAX_PWM 4095

#define RELAY_PROVIDER RELAY_PROVIDER_LIGHT
#define DUMMY_RELAY_COUNT 1

#define HYPERION_LIGHT_COUNT 4

#define ALEXA_SUPPORT 0
#define HOMEASSISTANT_SUPPORT 0
