#include "stusb4500_internal.hpp"
#include "stusb4500_conf.hpp"

#include <esp_check.h>
#include "driver/gpio.h"

namespace stusb4500
{
    void STUSB4500::start_sync_task()
    {
        xTaskCreatePinnedToCore(
            &STUSB4500::sync_task,
            "stusb4500_sync",
            4096,
            this,
            5,
            &sync_task_handle,
            APP_CPU_NUM);
    }

    void IRAM_ATTR STUSB4500::alert_isr_handler(void *arg)
    {
        auto *self = static_cast<STUSB4500 *>(arg);
        self->alert_triggered = true;
    }

    esp_err_t STUSB4500::configure_alert_pin(gpio_num_t gpio)
    {
        alert_gpio = gpio;
        alert_triggered = false;

        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << gpio,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_LOW_LEVEL};
        ESP_RETURN_ON_ERROR(gpio_config(&io_conf), "STUSB4500", "GPIO config failed");

        // Installer service d’interruption si nécessaire
        static bool isr_service_installed = false;
        if (!isr_service_installed)
        {
            gpio_install_isr_service(0);
            isr_service_installed = true;
        }

        gpio_isr_handler_add(gpio, &STUSB4500::alert_isr_handler, this);
        alert_enabled = true;
        return ESP_OK;
    }

    void STUSB4500::sync_task(void *arg)
    {
        auto *self = static_cast<STUSB4500 *>(arg);

        while (true)
        {
            uint32_t now = esp_log_timestamp();
            TickType_t delay = pdMS_TO_TICKS(self->available ? 100 : 10000);

            uint8_t buf;
            esp_err_t ping = self->read(DPM_PDO_NUMB, &buf, 1);
            bool is_online = (ping == ESP_OK);

            if (is_online && !self->available)
            {
                self->available = true;
                self->read_sectors();

                if (!compare_sector(self->sector, default_sector_config))
                {
                    ESP_LOGW("STUSB4500", "Configuration NVM différente, mise à jour...");
                    self->write_default_sectors(default_sector_config);
                }

                // Ensuite lecture des registres volatiles (PDOs)
                self->sync_from_device();

                self->last_sync_ms = now;
                ESP_LOGI("STUSB4500", "STUSB4500 détecté, synchronisation initiale effectuée.");
            }
            else if (!is_online && self->available)
            {
                self->available = false;
                ESP_LOGD("STUSB4500", "STUSB4500 non détecté (hors tension ?)");
            }

            if (self->available)
            {
                if (self->alert_enabled && self->alert_triggered)
                {
                    self->alert_triggered = false;
                    self->sync_from_device();
                    self->last_sync_ms = now;
                }
                else if ((now - self->last_sync_ms) >= self->sync_interval_ms)
                {
                    self->sync_from_device();
                    self->last_sync_ms = now;
                }
            }

            vTaskDelay(delay);
        }
    }

    esp_err_t STUSB4500::sync_from_device()
    {
        return read(); // équivalent à read_sectors() + parsing dans pdos[] et sector[].
    }

} // namespace stusb4500
