 #pragma once

 #include <expected>
 #include <stdexcept>
 #include "STUSB4500_driver.hpp"
 #include "freertos/FreeRTOS.h"
 #include "freertos/semphr.h"
 #include "driver/i2c_master.h"
 
 
 #define DEFAULT_STUSB4500_I2C_ADDRESS 0x40
 
 /**
  * @brief Represents an STUSB4500 device for current and power monitoring.
  * 
  * This class provides an interface for interacting with the STUSB4500 device
  * over I2C communication. It allows initializing the I2C bus, writing to
  * and reading from the device registers, and handling I2C-related errors.
  */
 class STUSB4500 : public STUSB4500_Driver {
 public:
     /**
      * @brief Initializes the I2C of ESP-IDF
      * 
      * @param[in] sda_io_num GPIO number of SDA pin. Defaults to GPIO 21 for ESP32.
      * @param[in] scl_io_num GPIO number of SCL pin. Defaults to GPIO 22 for ESP32.
      * @param[in] address I2C address of STUSB4500. Defaults to 0x40.
      * @param[in] scl_frequency Frequency of SCL pin. Defaults to 100000.
      * @param[in] i2c_port_num I2C port number.
      */
     STUSB4500(const gpio_num_t sda_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA), const gpio_num_t scl_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL), const uint16_t address = CONFIG_STUSB4500_I2C_ADDRESS, const uint32_t scl_frequency = CONFIG_I2C_MASTER_FREQUENCY, const i2c_port_num_t i2c_port_num = static_cast<i2c_port_num_t>(CONFIG_I2C_MASTER_PORT_NUM));
 
     /**
      * @brief Initializes the STUSB4500 using an existing I2C handle.
      * 
      * @param[in] i2c_bus_handle Existing I2C bus handle to use for this STUSB4500.
      * @param[in] address I2C address of STUSB4500. Defaults to 0x40.
      * @param[in] scl_frequency Frequency of SCL pin. Defaults to 100000.
      */
     STUSB4500(i2c_master_bus_handle_t i2c_bus_handle, const uint16_t address = CONFIG_STUSB4500_I2C_ADDRESS, const uint32_t scl_frequency = CONFIG_I2C_MASTER_FREQUENCY);
 
 protected:
     /**
      * @brief I2C write function for ESP-IDF. Converts from little endian (ESP-IDF) to big endian (STUSB4500).
      * 
      * @param[in] Register The register to write to on STUSB4500
      * @param[in] Value The value to write to the register
      * @return std::expected<void, std::runtime_error> 
      */
     std::expected<void, std::runtime_error> I2C_Write(const Register Register, const uint16_t Value) override;
 
     /**
      * @brief I2C read function for ESP-IDF. Converts from big endian (STUSB4500) to little endian (ESP-IDF).
      * 
      * @param[in] Register The register to read from on STUSB4500
      * @return std::expected<uint16_t, std::runtime_error> 
      */
     std::expected<uint16_t, std::runtime_error> I2C_Read(const Register Register) override;
 
     /**
      * @brief Creates a mutex for I2C bus
      * 
      * @param[out] mutex The mutex to create
      * @return esp_err_t 
      */
     esp_err_t CreateMutex(SemaphoreHandle_t &mutex);
 
 protected:
     SemaphoreHandle_t Lock;
     i2c_master_bus_config_t i2c_bus_config;
     i2c_master_bus_handle_t i2c_bus_handle;
     i2c_device_config_t i2c_dev_cfg;
     i2c_master_dev_handle_t i2c_dev_handle;
 };