#include "stusb4500_internal.hpp"
#include <esp_log.h>
#include <esp_check.h>

namespace stusb4500
{
    STUSB4500::STUSB4500(std::shared_ptr<I2CDevice> i2c)
        : i2c_dev(std::move(i2c)), sector{}
    {
        last_sync_ms = esp_log_timestamp();
        start_sync_task();
    }

    STUSB4500::~STUSB4500()
    {
        if (sync_task_handle)
        {
            vTaskDelete(sync_task_handle);
            sync_task_handle = nullptr;
        }
    }

    esp_err_t STUSB4500::read(uint8_t reg, uint8_t *data, size_t len)
    {
        return i2c_dev->read(reg, data, len);
    }

    esp_err_t STUSB4500::write(uint8_t reg, const uint8_t *data, size_t len)
    {
        return i2c_dev->write(reg, data, len);
    }
}