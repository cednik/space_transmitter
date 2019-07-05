#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdexcept>

class Mutex
{
public:

    struct timeout_error : public std::runtime_error
    {
        timeout_error(char const* const message = nullptr) noexcept
            :std::runtime_error(message)
        {}
    };

    typedef SemaphoreHandle_t handle_type;
    typedef TickType_t time_type;

    Mutex(const Mutex&) = delete;
    Mutex& operator = (const Mutex&) = delete;

    Mutex(handle_type* handle = nullptr, bool Acquire = true, time_type timeout = portMAX_DELAY)
        : c_copy(handle != nullptr), m_handle( c_copy ? *handle : xSemaphoreCreateMutex()), m_acquired(false)
    {
        if (Acquire) {
            if (!acquire(timeout))
                throw timeout_error();
        }
    }

    ~Mutex(void) {
        release();
        if (!c_copy) {
            vSemaphoreDelete(m_handle);
        }
    }

    bool acquire(time_type timeout = 0) {
        return m_acquired = (xSemaphoreTake(m_handle, timeout) == pdTRUE);
    }

    bool release(void) {
        if (!m_acquired)
            return false;
        m_acquired = false;
        return xSemaphoreGive(m_handle) == pdTRUE;
    }

    bool is_acquired(void) {
        return m_acquired;
    }

private:
    const bool c_copy;
    handle_type m_handle;
    bool m_acquired;
};