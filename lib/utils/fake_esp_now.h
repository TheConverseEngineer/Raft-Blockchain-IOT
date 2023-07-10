#ifndef FAKE_ESP_NOW_H
#define FAKE_ESP_NOW_H

#define WIFI_STA 0
#define ESP_OK 0

#include <stdint.h>
#include <cstddef>

class WiFiObject {
    public:
        static const void mode(int) { }
};

static const WiFiObject WiFi;

int esp_now_init();

struct esp_now_peer_info {
    unsigned char peer_addr[6];
    int channel;
    bool encrypt;
};

int esp_now_add_peer(esp_now_peer_info *random);

int esp_now_send(const uint8_t *peer_addr, const uint8_t *data, std::size_t len);

#endif