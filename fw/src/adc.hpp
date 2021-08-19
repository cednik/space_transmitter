#pragma once

#include <Arduino.h>

#include <queue>

class Adc {
    Adc(const Adc&) = delete;
public:
    Adc(uint8_t pin, adc_attenuation_t attenuation = ADC_0db)
        : m_pin(pin), m_value(0)
    {
        adcAttachPin(m_pin);
        analogSetPinAttenuation(m_pin, attenuation);
        s_instances.push(this);
        if (s_init && s_instances.empty())
            start();
    }

    ~Adc() {
        if (s_instances.empty())
            return;
        if (s_instances.front() == this) {
            //adcEnd(m_pin);
            s_instances.pop();
            if (!s_instances.empty())
                s_instances.front()->start();
        } else {
            const Adc* const actual = s_instances.front();
            do {
                s_instances.push(s_instances.front());
                s_instances.pop();
            } while (s_instances.front() != this);
            s_instances.pop();
            while (s_instances.front() != actual) {
                s_instances.push(s_instances.front());
                s_instances.pop();
            }
        }
    }

    //uint16_t value() const { return m_value; }
    uint16_t value() const { return analogRead(m_pin); }

private:
    void start() {
        //adcStart(m_pin);
    }

    const uint8_t m_pin;
    uint16_t m_value;

public:
    static void begin(adc_attenuation_t attenuation = ADC_0db) {
        if (s_init)
            return;
        s_init = true;
        analogSetAttenuation(attenuation);
        if (!s_instances.empty())
            s_instances.front()->start();
    }

    static void process() {
        return;
        if (s_instances.empty())
            return;
        Adc& adc = *s_instances.front();
        // if (adcBusy(adc.m_pin))
        //     return;
        // adc.m_value = adcEnd(adc.m_pin);
        adc.m_value = analogRead(adc.m_pin);
        s_instances.push(&adc);
        s_instances.pop();
        s_instances.front()->start();
    }
private:
    static bool s_init;
    static std::queue<Adc*> s_instances;
};

bool Adc::s_init = false;
std::queue<Adc*> Adc::s_instances;
