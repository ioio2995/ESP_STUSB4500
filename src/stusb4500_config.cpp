#include "stusb4500_internal.hpp"
#include "stusb4500_register_map.h"

namespace stusb4500
{
    void STUSB4500::set_voltage(uint8_t pdo_numb, float voltage)
    {
        STUSB_CHECK_AVAILABLE();
        if (pdo_numb < 1 || pdo_numb > 3)
            return;
        if (pdo_numb == 1)
            voltage = 5.0f;

        if (voltage < 5.0f)
            voltage = 5.0f;
        if (voltage > 20.0f)
            voltage = 20.0f;

        uint32_t pdo;
        if (read_pdo(pdo_numb, pdo) != ESP_OK)
            return;

        pdo &= ~(0x3FF << 10);
        pdo |= (static_cast<uint32_t>(voltage * 20) & 0x3FF) << 10;

        write_pdo(pdo_numb, pdo);
    }

    void STUSB4500::set_current(uint8_t pdo_numb, float current)
    {
        STUSB_CHECK_AVAILABLE();
        if (pdo_numb < 1 || pdo_numb > 3)
            return;

        if (current < 0.0f)
            current = 0.0f;
        if (current > 5.0f)
            current = 5.0f;

        uint32_t pdo;
        if (read_pdo(pdo_numb, pdo) != ESP_OK)
            return;

        pdo &= ~0x3FF;
        pdo |= static_cast<uint32_t>(current / 0.01f) & 0x3FF;

        write_pdo(pdo_numb, pdo);
    }

    void STUSB4500::set_pdo_number(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value > 3)
            value = 3;
        uint8_t buf = value;
        write(DPM_PDO_NUMB, &buf, 1);
    }

    void STUSB4500::set_upper_voltage_limit(uint8_t pdo_numb, uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value < 5)
            value = 5;
        if (value > 20)
            value = 20;

        read_sectors();

        switch (pdo_numb)
        {
        case 1:
            sector[3][3] = (sector[3][3] & 0x0F) | ((value - 5) << 4);
            write_sector(3, sector[3]);
            break;
        case 2:
            sector[3][5] = (sector[3][5] & 0xF0) | (value - 5);
            write_sector(3, sector[3]);
            break;
        case 3:
            sector[3][6] = (sector[3][6] & 0x0F) | ((value - 5) << 4);
            write_sector(3, sector[3]);
            break;
        }
    }

    void STUSB4500::set_lower_voltage_limit(uint8_t pdo_numb, uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value < 5)
            value = 5;
        if (value > 20)
            value = 20;

        read_sectors();

        switch (pdo_numb)
        {
        case 2:
            sector[3][4] = (sector[3][4] & 0x0F) | ((value - 5) << 4);
            write_sector(3, sector[3]);
            break;
        case 3:
            sector[3][6] = (sector[3][6] & 0xF0) | (value - 5);
            write_sector(3, sector[3]);
            break;
        }
    }

    void STUSB4500::set_flex_current(float value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value < 0.0f)
            value = 0.0f;
        if (value > 5.0f)
            value = 5.0f;

        read_sectors();

        uint16_t raw = static_cast<uint16_t>(value * 100);

        sector[4][3] = (sector[4][3] & 0x03) | ((raw & 0x3F) << 2);
        sector[4][4] = (sector[4][4] & 0xF0) | ((raw >> 6) & 0x0F);

        write_sector(4, sector[4]);
    }

    void STUSB4500::set_external_power(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        value = value ? 1 : 0;

        read_sectors();
        sector[3][2] = (sector[3][2] & 0xF7) | (value << 3);
        write_sector(3, sector[3]);
    }

    void STUSB4500::set_usb_comm_capable(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        value = value ? 1 : 0;

        read_sectors();
        sector[3][2] = (sector[3][2] & 0xFE) | value;
        write_sector(3, sector[3]);
    }

    void STUSB4500::set_config_ok_gpio(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value < 2)
            value = 0;
        else if (value > 3)
            value = 3;

        read_sectors();
        sector[4][4] = (sector[4][4] & 0x9F) | (value << 5);
        write_sector(4, sector[4]);
    }

    void STUSB4500::set_gpio_ctrl(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        if (value > 3)
            value = 3;

        read_sectors();
        sector[1][0] = (sector[1][0] & 0xCF) | (value << 4);
        write_sector(1, sector[1]);
    }

    void STUSB4500::set_power_above_5v_only(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        value = value ? 1 : 0;

        read_sectors();
        sector[4][6] = (sector[4][6] & 0xF7) | (value << 3);
        write_sector(4, sector[4]);
    }

    void STUSB4500::set_req_src_current(uint8_t value)
    {
        STUSB_CHECK_AVAILABLE();
        value = value ? 1 : 0;

        read_sectors();
        sector[4][6] = (sector[4][6] & 0xEF) | (value << 4);
        write_sector(4, sector[4]);
    }
}