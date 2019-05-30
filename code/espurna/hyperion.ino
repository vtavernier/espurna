/*

HYPERION MODULE

*/

#if HYPERION_SUPPORT

#include <WiFiUdp.h>

WiFiUDP _hyperion_udp;

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

        terminalOK();
    });
}
#endif // TERMINAL_SUPPORT

void _hyperionLoop() {
    int packetSize = _hyperion_udp.parsePacket();
    if (_hyperion_listen &&
        packetSize > 0 && (packetSize % HYPERION_LIGHT_CHANNELS) == 0) {
        // An Hyperion packet is 8-bit RGB data in the order
        // of the configured LEDs. We can parse that now.
        uint8_t packetBuffer[packetSize];
        _hyperion_udp.read(packetBuffer, packetSize);

        unsigned char led = 0;
        for (int i = 0; i < packetSize; i += HYPERION_LIGHT_CHANNELS, led += LIGHT_CHANNELS) {
            // We assume that the packet contains HYPERION_LIGHT_CHANNELS channels per light
            for (int j = 0; j < HYPERION_LIGHT_CHANNELS; ++j) {
                lightChannel(led + j, packetBuffer[i + j]);

                // Switch on channel
                lightState(led + j, true);
            }

            // Extra channels should not be enabled
            for (int j = HYPERION_LIGHT_CHANNELS; j < LIGHT_CHANNELS; ++j) {
                // Clear channel value
                lightChannel(led + j, 0);
            }
        }

        // Full brightness, Hyperion should be handling color transforms
        lightBrightness(LIGHT_MAX_BRIGHTNESS);

        // Switch on lights
        lightState(true);

        // Let Hyperion manage colors if we have all channels
        lightHyperionUpdate(HYPERION_LIGHT_CHANNELS == LIGHT_CHANNELS);
        // Do not forward Hyperion colors
        lightUpdate(true, false, false);
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
