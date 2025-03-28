#include "stusb4500_internal.hpp"
#include "stusb4500_register_map.h"

namespace stusb4500
{
    float STUSB4500::get_voltage(uint8_t pdo_numb)
    {
        STUSB_CHECK_AVAILABLE_RET(0.0f);
        if (pdo_numb < 1 || pdo_numb > 3)
            return 0.0f;
        return pdos[pdo_numb - 1].voltage;
    }

    float STUSB4500::get_current(uint8_t pdo_numb)
    {
        STUSB_CHECK_AVAILABLE_RET(0.0f);
        if (pdo_numb < 1 || pdo_numb > 3)
            return 0.0f;
        return pdos[pdo_numb - 1].current;
    }

    uint8_t STUSB4500::get_upper_voltage_limit(uint8_t pdo_numb)
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        switch (pdo_numb)
        {
        case 1:
            return ((sector[3][3] >> 4) & 0x0F) + 5;
        case 2:
            return (sector[3][5] & 0x0F) + 5;
        case 3:
            return (sector[3][6] >> 4) + 5;
        default:
            return 0;
        }
    }

    uint8_t STUSB4500::get_lower_voltage_limit(uint8_t pdo_numb)
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        switch (pdo_numb)
        {
        case 2:
            return (sector[3][4] >> 4) + 5;
        case 3:
            return (sector[3][6] & 0x0F) + 5;
        default:
            return 0; // PDO1 non configurable
        }
    }

    float STUSB4500::get_flex_current()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        uint16_t raw = ((sector[4][4] & 0x0F) << 6) | ((sector[4][3] & 0xFC) >> 2);
        return raw / 100.0f;
    }

    uint8_t STUSB4500::get_pdo_number()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        uint8_t value = 0;
        read(DPM_PDO_NUMB, &value, 1); // Valeur volatile actuelle
        return value & 0x07;
    }

    uint8_t STUSB4500::get_external_power()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return (sector[3][2] >> 3) & 0x01;
    }

    uint8_t STUSB4500::get_usb_comm_capable()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return sector[3][2] & 0x01;
    }

    uint8_t STUSB4500::get_config_ok_gpio()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return (sector[4][4] >> 5) & 0x03;
    }

    uint8_t STUSB4500::get_gpio_ctrl()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return (sector[1][0] >> 4) & 0x03;
    }

    uint8_t STUSB4500::get_power_above_5v_only()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return (sector[4][6] >> 3) & 0x01;
    }

    uint8_t STUSB4500::get_req_src_current()
    {
        STUSB_CHECK_AVAILABLE_RET(0);
        return (sector[4][6] >> 4) & 0x01;
    }
}