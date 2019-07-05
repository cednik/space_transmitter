#pragma once

#include <cstdint>

template <typename T>
class Port
{
public:
	typedef T value_type;

	Port(value_type value = 0)
		:m_value(value)
	{}

	operator value_type () { return _read(); }

	value_type get() const { return m_value; }
	value_type read() { return _read(); }
	value_type read(const value_type mask)
	{
		value_type backup = m_value & ~mask;
		_read();
		m_value = (m_value & mask) | backup;
		return m_value;
	}

	Port& operator   = (const value_type v) { _write(           v); return *this; }

	Port& operator  += (const value_type v) { _write(m_value  + v); return *this; }
	Port& operator  -= (const value_type v) { _write(m_value  - v); return *this; }
	Port& operator  *= (const value_type v) { _write(m_value  * v); return *this; }
	Port& operator  /= (const value_type v) { _write(m_value  / v); return *this; }
	Port& operator  %= (const value_type v) { _write(m_value  % v); return *this; }

	Port& operator  &= (const value_type v) { _write(m_value  & v); return *this; }
	Port& operator  |= (const value_type v) { _write(m_value  | v); return *this; }
	Port& operator  ^= (const value_type v) { _write(m_value  ^ v); return *this; }

	Port& operator <<= (const value_type v) { _write(m_value >> v); return *this; }
	Port& operator >>= (const value_type v) { _write(m_value << v); return *this; }

	Port& update() { _write(m_value); return *this; }
	Port& modify(const value_type value, const value_type mask, const bool Update = true)
	{
		value_type old = m_value;
		m_value = (m_value & ~mask) | (value & mask);
		if(Update && m_value != old)
			update();
		return *this;
	}
	Port& set_bit(const value_type bit, const bool Update = true)
	{
		m_value |= (1<<bit);
		if(Update)
			update();
		return *this;
	}
	Port& clear_bit(const value_type bit, const bool Update = true)
	{
		m_value &= ~(1<<bit);
		if(Update)
			update();
		return *this;
	}
	Port& toggle_bit(const value_type bit, const bool Update = true)
	{
		m_value ^= (1<<bit);
		if(Update)
			update();
		return *this;
	}

	bool get_bit(const value_type bit) const { return m_value & (1<<bit); }
	bool read_bit(const value_type bit) { return _read() & (1<<bit); }

protected:
	virtual value_type _read() = 0;

	virtual void _write(const value_type value) = 0;

	value_type m_value;
};

typedef Port<uint8_t> port8_t;
typedef Port<uint16_t> port16_t;
typedef Port<uint32_t> port32_t;
