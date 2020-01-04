/*

HYPERION MODULE

*/

#if HYPERION_SUPPORT

#include <WiFiUdp.h>

WiFiUDP _hyperion_udp;

enum hyperion_listen_state_t {
    HL_INACTIVE,
    HL_ACTIVE,
} _hyperion_state = HL_INACTIVE;
unsigned long long _hyperion_state_changed = 0;
bool _hyperion_saved_light_state;

bool _hyperion_listen;
uint16_t _hyperion_port;

void hyperionListen(bool listen) {
    _hyperion_listen = listen;
    setSetting("hyperionListen", listen);
    saveSettings();
}

void _hyperionConfigure() {
    _hyperion_listen = getSetting("hyperionListen", HYPERION_LISTEN).toInt() == 1;
    _hyperion_port = getSetting("hyperionPort", HYPERION_PORT).toInt();
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#if MQTT_SUPPORT
void _hyperionMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_HYPERION_LISTEN);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char *) topic);

        if (t.equals(MQTT_TOPIC_HYPERION_LISTEN)) {
            hyperionListen(atoi(payload));
            return;
        }
    }
}
#endif // MQTT_SUPPORT

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

#if API_SUPPORT
void _hyperionAPISetup() {
    apiRegister(MQTT_TOPIC_HYPERION_LISTEN,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _hyperion_listen ? 1 : 0);
        },
        [](const char * payload) {
            hyperionListen(atoi(payload));
        }
    );
}
#endif // API_SUPPORT

#if TERMINAL_SUPPORT
void _hyperionInitCommands() {
    terminalRegisterCommand(F("HYPERION"), [](Embedis* e) {
        if (e->argc > 2) {
            terminalError(F("Wrong arguments"));
            return;
        }

        if (e->argc == 2) {
            int value = String(e->argv[1]).toInt();
            hyperionListen(value == 1);
        }

        DEBUG_MSG_P(PSTR("Listen: %s\n"), _hyperion_listen ? "true" : "false");
        DEBUG_MSG_P(PSTR("State: %s\n"), _hyperion_state == HL_INACTIVE ? "inactive" : "active");

        terminalOK();
    });
}
#endif // TERMINAL_SUPPORT

void _hyperionLoop() {
    int packetSize = _hyperion_udp.parsePacket();
    int componentsPerLed = packetSize / HYPERION_LIGHT_COUNT;

    bool updated = false;

    if (_hyperion_listen && packetSize > 0 && componentsPerLed <= LIGHT_CHANNELS) {
        // If we were in an inactive state, change it now
        if (_hyperion_state == HL_INACTIVE) {
            _hyperion_state = HL_ACTIVE;
            _hyperion_saved_light_state = lightState();
            _lightSaveRtcmem();
        }

        // An Hyperion packet is 8-bit RGB data in the order
        // of the configured LEDs. We can parse that now.
        uint8_t packetBuffer[packetSize];
        _hyperion_udp.read(packetBuffer, packetSize);

        unsigned char led = 0;
        for (int i = 0; i < packetSize; i += componentsPerLed, led += LIGHT_CHANNELS) {
            // We assume that the packet contains HYPERION_LIGHT_CHANNELS channels per light
            for (int j = 0; j < componentsPerLed; ++j) {
                lightChannel(led + j, packetBuffer[i + j]);

                // Switch on channel
                lightState(led + j, true);
            }

            // Extra channels should not be enabled
            for (int j = componentsPerLed; j < LIGHT_CHANNELS; ++j) {
                // Clear channel value
                lightChannel(led + j, 0);
            }
        }

        // Full brightness, Hyperion should be handling color transforms
        lightBrightness(LIGHT_MAX_BRIGHTNESS);

        // Switch on lights
        lightState(true);

        // Let Hyperion manage colors if we have all channels
        lightHyperionUpdate(componentsPerLed == LIGHT_CHANNELS);

        // Do not forward Hyperion colors
        // Do not save values to EEPROM
        lightUpdate(false, false, false);

        // Update current state
        _hyperion_state_changed = millis();
    }

    if (_hyperion_state == HL_ACTIVE &&
            millis() - _hyperion_state_changed > 10000) {
        // We're in the active state but no data came through recently (10s)
        _hyperion_state = HL_INACTIVE;
        _lightRestoreRtcmem();
        lightState(_hyperion_saved_light_state);
        lightUpdate(false, false, false);
    }
}

void hyperionSetup() {
    _hyperionConfigure();

    // Start udp object
    _hyperion_udp.begin(_hyperion_port);

    // Register loop
    espurnaRegisterLoop(_hyperionLoop);

    #if API_SUPPORT
        _hyperionAPISetup();
    #endif

    #if MQTT_SUPPORT
        mqttRegister(_hyperionMQTTCallback);
    #endif

    #if TERMINAL_SUPPORT
        _hyperionInitCommands();
    #endif
}

#endif // HYPERION_SUPPORT
