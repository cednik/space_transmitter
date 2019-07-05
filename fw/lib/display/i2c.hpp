#pragma once

#include <cstdint>
#include <cstdio>

#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "../util/mutex.hpp"

#include "../format/format.h"
using fmt::print;

#include <vector>

class I2C_master
{
public:
    typedef uint8_t address_type;
    typedef uint8_t data_type;
    typedef size_t size_type;

	I2C_master(const i2c_port_t i2c_num, const gpio_num_t sda, const gpio_num_t scl)
        : c_i2c_num(i2c_num), c_sda(sda), c_scl(scl)
    {
        if (!s_mutex_handle[c_i2c_num]) {
            //s_mutex_handle[c_i2c_num] = xSemaphoreCreateMutexStatic( s_mutex_memory + c_i2c_num );
            s_mutex_handle[c_i2c_num] = xSemaphoreCreateMutex();
        }
    }

    void init(void) {

        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = c_sda;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = c_scl;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 100000;
        i2c_param_config(c_i2c_num, &conf);
        i2c_driver_install(c_i2c_num, conf.mode, 0, 0, 0);
    }

    esp_err_t write(const address_type address, const data_type* data, const size_type size) {
        Mutex mutex(s_mutex_handle + c_i2c_num);
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ( address << 1 ) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, const_cast<uint8_t*>(data), size, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(c_i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
            print("i2c write: {:d} bytes from address 0x{:08X} [0x{:02X}] to device 0x{:02X} results in {:d}\n", size, uintptr_t(data), *data, address, ret);
        return ret;
    }

    esp_err_t read(const address_type address, data_type* data, const size_type size) {
        Mutex mutex(s_mutex_handle + c_i2c_num);
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ( address << 1 ) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, data, size, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(c_i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
            print("i2c read: {:d} bytes to address 0x{:08X} from device 0x{:02X} results in {:d}\n", size, uintptr_t(data), address, ret);
        return ret;
    }

private:
    const i2c_port_t c_i2c_num;
    const gpio_num_t c_sda;
    const gpio_num_t c_scl;

    //static StaticSemaphore_t s_mutex_memory[I2C_NUM_MAX];
    static SemaphoreHandle_t s_mutex_handle[I2C_NUM_MAX];
};

//StaticSemaphore_t I2C_master::s_mutex_memory[I2C_NUM_MAX];
SemaphoreHandle_t I2C_master::s_mutex_handle[I2C_NUM_MAX] = { 0 };

auto i2c_scan (I2C_master& i2c) {
    std::vector<I2C_master::address_type> ret;
    I2C_master::data_type data = 0;
    for (I2C_master::address_type i = 1; i != 128; ++i) {
        if (i2c.write(i, &data, 0) == ESP_OK)
            ret.push_back(i);
    }
    return ret;
}