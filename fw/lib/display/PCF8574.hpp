#pragma once

#include "port.hpp"
#include "i2c.hpp"
#include "../time/time.hpp"

template <class I2C>
class PCF8574_t
	:public port8_t
{
	typedef typename I2C::data_type data_type;
public:
	typedef typename I2C::address_type address_type;
	typedef port8_t port_type;
	typedef typename port_type::value_type value_type;
	
	

	PCF8574_t(I2C& bus, const address_type addr, const value_type value = 0)
		:port_type(value), m_bus(bus), m_addr(addr)
	{}

	PCF8574_t(const PCF8574_t<I2C>& port)
		:port_type(port.m_value), m_bus(port.m_bus), m_addr(port.m_addr)
	{}


protected:
	value_type _read()
	{
		m_bus.read(m_addr, reinterpret_cast<data_type*>(&m_value), sizeof(m_value));
		return m_value;
	}

	void _write(const value_type value)
	{
		m_value = value;
		m_bus.write(m_addr, reinterpret_cast<const data_type*>(&m_value), sizeof(m_value));
	}

	I2C& m_bus;
	const address_type m_addr;
};
