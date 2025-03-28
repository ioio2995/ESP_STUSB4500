#pragma once

#include <memory>
#include <cstdint>
#include <expected>
#include "esp_err.h"
#include "I2CDevice.hpp"
#include "STUSB4500_register_map.h"

namespace stusb4500 {

struct PDO {
    float voltage;  // en Volts
    float current;  // en Ampères
};

/**
 * @brief Driver C++ moderne pour le STUSB4500 utilisant une interface I2C générique.
 */
class STUSB4500 {
public:
    explicit STUSB4500(std::shared_ptr<I2CDevice> i2c);
    ~STUSB4500();

    // === Communication bas-niveau ===
    esp_err_t read(uint8_t reg, uint8_t* data, size_t len);
    esp_err_t write(uint8_t reg, const uint8_t* data, size_t len);

    // === Contrôle PD ===
    esp_err_t soft_reset();
    esp_err_t read_pdo(uint8_t pdo_numb, uint32_t& out_pdo);
    esp_err_t write_pdo(uint8_t pdo_numb, uint32_t pdo_data);

    // === Gestion NVM ===
    esp_err_t read();
    esp_err_t read_sectors();
    esp_err_t write_sectors(bool use_defaults = false);
    esp_err_t write_sector(uint8_t sector_num, const uint8_t* data);
    esp_err_t write_default_sectors(const uint8_t custom_sector[5][8]);
    esp_err_t enter_write_mode(uint8_t erased_sectors);
    esp_err_t exit_test_mode();

    // === Accesseurs (lecture de configuration) ===
    float get_voltage(uint8_t pdo_numb);
    float get_current(uint8_t pdo_numb);
    uint8_t get_upper_voltage_limit(uint8_t pdo_numb);
    uint8_t get_lower_voltage_limit(uint8_t pdo_numb);
    float get_flex_current();
    uint8_t get_pdo_number();
    uint8_t get_external_power();
    uint8_t get_usb_comm_capable();
    uint8_t get_config_ok_gpio();
    uint8_t get_gpio_ctrl();
    uint8_t get_power_above_5v_only();
    uint8_t get_req_src_current();

    // === Mutateurs (configuration) ===
    void set_voltage(uint8_t pdo_numb, float voltage);
    void set_current(uint8_t pdo_numb, float current);
    void set_upper_voltage_limit(uint8_t pdo_numb, uint8_t value);
    void set_lower_voltage_limit(uint8_t pdo_numb, uint8_t value);
    void set_flex_current(float value);
    void set_pdo_number(uint8_t value);
    void set_external_power(uint8_t value);
    void set_usb_comm_capable(uint8_t value);
    void set_config_ok_gpio(uint8_t value);
    void set_gpio_ctrl(uint8_t value);
    void set_power_above_5v_only(uint8_t value);
    void set_req_src_current(uint8_t value);

    // === Synchronisation automatique ===
    static void IRAM_ATTR alert_isr_handler(void* arg);
    bool is_available() const { return available; }

private:
    // === Interface bas-niveau ===
    std::shared_ptr<I2CDevice> i2c_dev;

    // === Données locales ===
    uint8_t sector[5][8] = {};
    PDO pdos[3];

    // === Sync & alert ===
    gpio_num_t alert_gpio = GPIO_NUM_NC;
    volatile bool alert_triggered = false;
    bool alert_enabled = false;
    bool available = false;
    uint32_t last_sync_ms = 0;
    uint32_t sync_interval_ms = 60000;
    TaskHandle_t sync_task_handle = nullptr;

    // === Logique interne ===
    void start_sync_task();
    static void sync_task(void* arg);
    esp_err_t sync_from_device();
    esp_err_t configure_alert_pin(gpio_num_t gpio);
};

} // namespace stusb4500
