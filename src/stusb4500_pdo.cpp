#include "stusb4500_internal.hpp"
#include <esp_check.h>

namespace stusb4500
{
    esp_err_t STUSB4500::soft_reset()
    {
        uint8_t buffer[1];

        buffer[0] = 0x0D; // SOFT_RESET Command
        ESP_RETURN_ON_ERROR(write(TX_HEADER_LOW, buffer, 1), "STUSB4500", "TX_HEADER_LOW failed");

        buffer[0] = 0x26; // SEND_COMMAND
        ESP_RETURN_ON_ERROR(write(PD_COMMAND_CTRL, buffer, 1), "STUSB4500", "PD_COMMAND_CTRL failed");

        return ESP_OK;
    }

    esp_err_t STUSB4500::read_pdo(uint8_t pdo_numb, uint32_t& out_pdo) {
        if (pdo_numb < 1 || pdo_numb > 3) return ESP_ERR_INVALID_ARG;
    
        uint8_t buffer[4];
        uint8_t reg = 0x85 + (pdo_numb - 1) * 4;
    
        esp_err_t err = read(reg, buffer, sizeof(buffer));
        if (err != ESP_OK) return err;
    
        out_pdo = (uint32_t)buffer[0] |
                  ((uint32_t)buffer[1] << 8) |
                  ((uint32_t)buffer[2] << 16) |
                  ((uint32_t)buffer[3] << 24);
        return ESP_OK;
    }
    
    esp_err_t STUSB4500::write_pdo(uint8_t pdo_numb, uint32_t pdo_data) {
        if (pdo_numb < 1 || pdo_numb > 3) return ESP_ERR_INVALID_ARG;
    
        uint8_t reg = 0x85 + (pdo_numb - 1) * 4;
        uint8_t buffer[4] = {
            static_cast<uint8_t>(pdo_data & 0xFF),
            static_cast<uint8_t>((pdo_data >> 8) & 0xFF),
            static_cast<uint8_t>((pdo_data >> 16) & 0xFF),
            static_cast<uint8_t>((pdo_data >> 24) & 0xFF)
        };
    
        return write(reg, buffer, sizeof(buffer));
    }
}