 #include "STUSB4500_interface.hpp"
 #include <utility>
 #include <string>
 #include "esp_err.h"
 
 #define I2C_TIMEOUT_MS 100
 
 std::expected<void, std::runtime_error> STUSB4500::I2C_Write(const Register Register, const uint16_t Value) {
     esp_err_t err;
 
     const uint8_t WriteBuffer[] = {
         static_cast<uint8_t>(Register),
         static_cast<uint8_t>(Value >> 8),
         static_cast<uint8_t>(Value & 0xFF),
     };
 
     if (xSemaphoreTake(Lock, portMAX_DELAY) != pdTRUE) {
         return std::unexpected(std::runtime_error("I2C write timeout: could not take mutex"));
     }
 
     err = i2c_master_transmit(i2c_dev_handle, WriteBuffer, sizeof(WriteBuffer), I2C_TIMEOUT_MS);
     xSemaphoreGive(Lock);
 
     switch (err) {
         case ESP_OK:
             return {};
         case ESP_ERR_INVALID_ARG:
             return std::unexpected(std::runtime_error("I2C write invalid arg"));
         case ESP_ERR_TIMEOUT:
             return std::unexpected(std::runtime_error("I2C write timeout: could not transmit"));
         default:
             return std::unexpected(std::runtime_error("I2C write unknown error. err = " + std::to_string(err)));
     }
 }
 
 std::expected<uint16_t, std::runtime_error> STUSB4500::I2C_Read(const STUSB4500::Register Register) {
     esp_err_t err;
 
     const auto WriteBuffer = Register;
 
     uint16_t ReadBuffer;
 
     if (xSemaphoreTake(Lock, portMAX_DELAY) != pdTRUE) {
         return std::unexpected(std::runtime_error("I2C read timeout: could not take mutex"));
     }
 
     err = i2c_master_transmit_receive(i2c_dev_handle, reinterpret_cast<const uint8_t *>(&WriteBuffer), sizeof(WriteBuffer), reinterpret_cast<uint8_t *>(&ReadBuffer), sizeof(ReadBuffer), I2C_TIMEOUT_MS);
     xSemaphoreGive(Lock);
     
     switch (err) {
         case ESP_OK:
             return (ReadBuffer << 8) | (ReadBuffer >> 8);
         case ESP_ERR_INVALID_ARG:
             return std::unexpected(std::runtime_error("I2C read invalid arg"));
         case ESP_ERR_TIMEOUT:
             return std::unexpected(std::runtime_error("I2C read timeout"));
         default:
             return std::unexpected(std::runtime_error("I2C read unknown error. err = " + std::to_string(err)));
     }
 }
 
 STUSB4500::STUSB4500(const gpio_num_t sda_io_num, const gpio_num_t scl_io_num, const uint16_t address, const uint32_t scl_frequency, const i2c_port_num_t i2c_port_num)
     : i2c_bus_config{
         .i2c_port = i2c_port_num,
         .sda_io_num = sda_io_num,
         .scl_io_num = scl_io_num,
         .clk_source = I2C_CLK_SRC_DEFAULT,
         .glitch_ignore_cnt = 7,
         .intr_priority = 0,
         .trans_queue_depth = 0,
         .flags{
             .enable_internal_pullup = true,
         },
     },
     i2c_dev_cfg{
         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
         .device_address = address,
         .scl_speed_hz = scl_frequency,
     } 
     
 {
     esp_err_t err;
 
     err = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
     if (err != ESP_OK)
         throw std::runtime_error("I2C bus initialization failed. err = " + std::to_string(err));
 
     err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_cfg, &i2c_dev_handle);
     if (err != ESP_OK)
         throw std::runtime_error("I2C add device failed. err = " + std::to_string(err));
 
     err = CreateMutex(Lock);
     if (err != ESP_OK)
         throw std::runtime_error("I2C mutex creation failed. err = " + std::to_string(err));
 
     auto start_driver = InitDriver();
     if (start_driver.has_value() == false)
         throw std::runtime_error(std::string("STUSB4500 driver initialization failed. err = ") + start_driver.error().what());
 }
 
 STUSB4500::STUSB4500(i2c_master_bus_handle_t bus_handle, const uint16_t address, const uint32_t scl_frequency)
     : i2c_bus_handle(bus_handle),
     i2c_dev_cfg{
         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
         .device_address = address,
         .scl_speed_hz = scl_frequency,
     }
 {
     esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_cfg, &i2c_dev_handle);
     if (err != ESP_OK)
         throw std::runtime_error("I2C add device failed. err = " + std::to_string(err));
 
     err = CreateMutex(Lock);
     if (err != ESP_OK)
         throw std::runtime_error("I2C mutex creation failed. err = " + std::to_string(err));
 
     auto start_driver = InitDriver();
     if (start_driver.has_value() == false)
         throw std::runtime_error(std::string("STUSB4500 driver initialization failed. err = ") + start_driver.error().what());
 }
 
 esp_err_t STUSB4500::CreateMutex(SemaphoreHandle_t &mutex) {
     mutex = xSemaphoreCreateMutex();
     if (!mutex) {
         return ESP_ERR_NO_MEM;
     }
     return ESP_OK;
 }