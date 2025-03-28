#include "stusb4500_internal.hpp"
#include <cstring> // pour memset
#include <esp_check.h>

namespace
{
    float decode_current(uint8_t val)
    {
        if (val == 0)
            return 0.0f;
        return val < 11 ? val * 0.25f + 0.25f : val * 0.5f - 2.5f;
    }

    uint8_t encode_current(float current)
    {
        if (current <= 0.25f)
            return 0;
        if (current <= 3.0f)
            return static_cast<uint8_t>((current - 0.25f) / 0.25f);
        return static_cast<uint8_t>((current + 2.5f) / 0.5f);
    }

    float decode_voltage(uint16_t val_mV)
    {
        return val_mV / 20.0f; // 20mV/LSB
    }

    uint16_t encode_voltage(float voltage)
    {
        return static_cast<uint16_t>(voltage * 20.0f);
    }
}

namespace stusb4500
{
    esp_err_t STUSB4500::read()
    {
        esp_err_t err = read_sectors();
        if (err != ESP_OK)
        {
            ESP_LOGE("STUSB4500", "Read failed: %s", esp_err_to_name(err));
            return err;
        }

        uint8_t current;

        pdos[0] = PDO{5.0f, decode_current((sector[3][2] & 0xF0) >> 4)};
        pdos[1] = PDO{
            decode_voltage((sector[4][1] << 2) | (sector[4][0] >> 6)),
            decode_current(sector[3][4] & 0x0F)};
        pdos[2] = PDO{
            decode_voltage(((sector[4][3] & 0x03) << 8) | sector[4][2]),
            decode_current((sector[3][5] & 0xF0) >> 4)};

        return ESP_OK;
    }

    esp_err_t STUSB4500::read_sectors()
    {
        uint8_t buffer[1];

        buffer[0] = FTP_CUST_PASSWORD;
        ESP_RETURN_ON_ERROR(write(FTP_CUST_PASSWORD_REG, buffer, 1), "STUSB4500", "Write failed");

        buffer[0] = 0x00;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Write failed");

        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Write failed");

        for (uint8_t i = 0; i < SectorCount; ++i)
        {
            buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N;
            ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Write failed");

            buffer[0] = (READ & FTP_CUST_OPCODE);
            ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "Write failed");

            buffer[0] = (i & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
            ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Write failed");

            do
            {
                ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "Read failed");
            } while (buffer[0] & FTP_CUST_REQ);

            ESP_RETURN_ON_ERROR(read(RW_BUFFER, &sector[i][0], SectorSize), "STUSB4500", "Read failed");
        }

        return exit_test_mode();
    }

    esp_err_t STUSB4500::write_sectors(bool use_defaults)
    {
        if (use_defaults)
        {
            memset(sector, DEFAULT, sizeof(sector));
        }
        else
        {
            // === PDO1 (5V fixe) dans sector[3][2]
            uint8_t cur1 = encode_current(pdos[0].current);
            sector[3][2] = (cur1 << 4) | (get_pdo_number() << 1); // bits [7:4]=I, [2:1]=PDO#

            // === PDO2 : Voltage (sector[4][0] + sector[4][1]), Current (sector[3][4])
            uint16_t v2 = encode_voltage(pdos[1].voltage);
            sector[4][0] = (v2 & 0x003F) << 2;                                               // bits [7:2] of low 6 bits
            sector[4][1] = (v2 >> 6) & 0xFF;                                                 // high bits
            sector[3][4] = (sector[3][4] & 0xF0) | (encode_current(pdos[1].current) & 0x0F); // LSBs

            // === PDO3 : Voltage (sector[4][2] + sector[4][3]), Current (sector[3][5])
            uint16_t v3 = encode_voltage(pdos[2].voltage);
            sector[4][2] = v3 & 0xFF;
            sector[4][3] = (sector[4][3] & 0xFC) | ((v3 >> 8) & 0x03);
            sector[3][5] = (sector[3][5] & 0x0F) | (encode_current(pdos[2].current) << 4);
        }

        // Entrée en mode écriture NVM
        ESP_RETURN_ON_ERROR(enter_write_mode(SECTOR_0 | SECTOR_1 | SECTOR_2 | SECTOR_3 | SECTOR_4), "STUSB4500", "Enter write mode failed");

        // Écriture séquentielle des secteurs
        for (uint8_t i = 0; i < SectorCount; ++i)
        {
            ESP_RETURN_ON_ERROR(write_sector(i, sector[i]), "STUSB4500", "Write sector failed");
        }

        return exit_test_mode();
    }

    esp_err_t STUSB4500::write_sector(uint8_t sector_num, const uint8_t *data)
    {
        uint8_t buffer[1];

        // Étape 1 : écrire les 8 octets à RW_BUFFER
        ESP_RETURN_ON_ERROR(write(RW_BUFFER, data, SectorSize), "STUSB4500", "Write RW_BUFFER failed");

        // Étape 2 : PWR + RST_N
        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "CTRL0 failed");

        // Étape 3 : Opcode WRITE_PL
        buffer[0] = WRITE_PL & FTP_CUST_OPCODE;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "CTRL1 (WRITE_PL) failed");

        // Étape 4 : Trigger WRITE_PL
        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Trigger WRITE_PL failed");

        do
        {
            ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "WAIT WRITE_PL");
        } while (buffer[0] & FTP_CUST_REQ);

        // Étape 5 : Opcode PROG_SECTOR
        buffer[0] = PROG_SECTOR & FTP_CUST_OPCODE;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "CTRL1 (PROG_SECTOR) failed");

        // Étape 6 : Trigger PROG_SECTOR avec sélection du secteur
        buffer[0] = (sector_num & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Trigger PROG_SECTOR failed");

        do
        {
            ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "WAIT PROG_SECTOR");
        } while (buffer[0] & FTP_CUST_REQ);

        return ESP_OK;
    }

    esp_err_t STUSB4500::write_default_sectors(const uint8_t custom_sector[5][8])
    {
        ESP_RETURN_ON_ERROR(
            enter_write_mode(SECTOR_0 | SECTOR_1 | SECTOR_2 | SECTOR_3 | SECTOR_4),
            "STUSB4500",
            "Enter write mode failed");

        for (uint8_t i = 0; i < 5; ++i)
        {
            ESP_RETURN_ON_ERROR(
                write_sector(i, custom_sector[i]),
                "STUSB4500",
                "Write sector failed");
        }

        return exit_test_mode();
    }

    esp_err_t STUSB4500::enter_write_mode(uint8_t erased_sectors)
    {
        uint8_t buffer[1];

        // Étape 1 : mot de passe
        buffer[0] = FTP_CUST_PASSWORD;
        ESP_RETURN_ON_ERROR(write(FTP_CUST_PASSWORD_REG, buffer, 1), "STUSB4500", "Password failed");

        // Étape 2 : Préparer RW_BUFFER (partiel efface = 0)
        buffer[0] = 0x00;
        ESP_RETURN_ON_ERROR(write(RW_BUFFER, buffer, 1), "STUSB4500", "RW_BUFFER reset failed");

        // Étape 3 : Reset interne du contrôleur
        buffer[0] = 0x00;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "CTRL0 reset failed");

        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "CTRL0 power/reset failed");

        // Étape 4 : Écriture de l’opcode WRITE_SER avec sélection de secteur(s)
        buffer[0] = ((erased_sectors << 3) & FTP_CUST_SER) | (WRITE_SER & FTP_CUST_OPCODE);
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "CTRL1 opcode write failed");

        // Étape 5 : Lancer la commande WRITE_SER
        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "REQ failed");

        // Attente fin d'exécution
        do
        {
            ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "CTRL0 read failed");
        } while (buffer[0] & FTP_CUST_REQ);

        // Étape 6 : Soft programming
        buffer[0] = SOFT_PROG_SECTOR & FTP_CUST_OPCODE;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "Soft prog opcode failed");

        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Soft prog exec failed");

        do
        {
            ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "Soft prog wait failed");
        } while (buffer[0] & FTP_CUST_REQ);

        // Étape 7 : Effacement des secteurs (obligatoire avant prog)
        buffer[0] = ERASE_SECTOR & FTP_CUST_OPCODE;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_1, buffer, 1), "STUSB4500", "Erase opcode failed");

        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "Erase exec failed");

        do
        {
            ESP_RETURN_ON_ERROR(read(FTP_CTRL_0, buffer, 1), "STUSB4500", "Erase wait failed");
        } while (buffer[0] & FTP_CUST_REQ);

        return ESP_OK;
    }

    esp_err_t STUSB4500::exit_test_mode()
    {
        uint8_t buffer[1];

        buffer[0] = FTP_CUST_RST_N;
        ESP_RETURN_ON_ERROR(write(FTP_CTRL_0, buffer, 1), "STUSB4500", "CTRL0 reset");

        buffer[0] = 0x00;
        ESP_RETURN_ON_ERROR(write(FTP_CUST_PASSWORD_REG, buffer, 1), "STUSB4500", "Clear password");

        return ESP_OK;
    }
}