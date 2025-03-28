#pragma once

#include "stusb4500.hpp"
#include "esp_log.h"

namespace stusb4500 {

// === Macros de vérification interne ===
#define STUSB_CHECK_AVAILABLE_RET(retval) \
    if (!available) { \
        ESP_LOGW("STUSB4500", "%s: périphérique non disponible", __FUNCTION__); \
        return retval; \
    }

#define STUSB_CHECK_AVAILABLE() \
    if (!available) { \
        ESP_LOGW("STUSB4500", "%s: périphérique non disponible", __FUNCTION__); \
        return; \
    }

// === Constantes ===
constexpr int SectorCount = 5;
constexpr int SectorSize = 8;

// === Utilitaires internes ===
inline bool compare_sector(const uint8_t a[5][8], const uint8_t b[5][8]) {
    for (int i = 0; i < SectorCount; ++i) {
        for (int j = 0; j < SectorSize; ++j) {
            if (a[i][j] != b[i][j]) return false;
        }
    }
    return true;
}

} // namespace stusb4500
