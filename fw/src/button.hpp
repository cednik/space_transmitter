/*
 * sklad
 *  main_board
 *   button.hpp
 *
 * Created: 20.1.2015 13:24:11
 *  Author: kubas
 */ 


#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include <math.hpp>
#include <functional>
#include <Arduino.h>

#ifndef MAX_BUTTONS_COUNT
#define MAX_BUTTONS_COUNT 1
#endif

class pin_t {
public:
	pin_t(gpio_num_t pin, bool invert = false)
		: m_pin(pin),
		  m_invert(invert)
	{}
	bool read(void) {
		return digitalRead(m_pin) ^ m_invert;
	}
private:
	gpio_num_t m_pin;
	bool m_invert;
};

class button_t
{
public:
	typedef const bool& callback_arg_type;
	typedef std::function<void(callback_arg_type)> callback_type;

	button_t(const pin_t& pin, callback_type const &callback = [](callback_arg_type){})
		:m_pin(pin), m_last_state(m_pin.read()), m_index(s_index < MAX_BUTTONS_COUNT ? s_index : -1), m_filter(msec(10)), m_callback(callback)
	{
		m_filter.cancel();
		if(s_index < MAX_BUTTONS_COUNT)
			s_buttons[s_index++] = this;
	}
		
	~button_t()
	{
		if(m_index != uint8_t(-1))
		{
			s_buttons[m_index] = nullptr;
			if(s_index > (m_index + 1))
			{
				s_buttons[m_index] = s_buttons[s_index - 1];
				s_buttons[m_index]->m_index = m_index;
			}
			--s_index;
		}
	}
	
	inline bool value() { return m_last_state; }
	
	inline pin_t& pin() { return m_pin; }
		
	virtual void process()
	{
		const bool v = m_pin.read();
		if(v != m_last_state && !m_filter.running())
		{
			m_filter.restart();
			m_last_state = v;
			m_callback(bool(m_last_state));
		}
		else if(m_filter)
		{
			m_filter.cancel();
		}
	}
	
	void force_update()
	{
		m_last_state = m_pin.read();
	}
	
	void register_callback(const callback_type& callback)
	{
		m_callback = callback;
	}
	
protected:
	pin_t m_pin;
	volatile bool m_last_state;
	uint8_t m_index;
	timeout m_filter;
	callback_type m_callback;
	
// static
public:
	static void process_all()
	{
		for(uint8_t i = 0; i != s_index; ++i)
			if(s_buttons[i] != nullptr)
				s_buttons[i]->process();
	}
protected:
	static uint8_t s_index;
	static button_t* s_buttons[];
};

uint8_t button_t::s_index = 0;
button_t* button_t::s_buttons[MAX_BUTTONS_COUNT] = { nullptr };

/*class button_intr_t
	:public button_t
{
public:
	button_intr_t(const pin_t& pin, const uint8_t intr_num,
		callback_type const &sync_callback = [](callback_arg_type){}, callback_type const &async_callback = [](callback_arg_type){})
		:button_t(pin, sync_callback), m_async_callback(async_callback), m_intr_num(avrlib::clamp(intr_num, 0, 1)), m_sync_callback(false)
	{
		m_pin.intr_en(intr_num, true);
	}
	
	void register_async_callback(const callback_type& callback)
	{
		m_async_callback = callback;
	}
		
	void process()
	{
		if(m_sync_callback)
		{
			m_sync_callback = false;
			m_callback(bool(m_last_state));
		}
		else if(m_filter)
		{
			m_filter.cancel();
			m_pin.intr_en(m_intr_num, true);
		}
		else if(!m_filter.running())
		{
			const bool v = m_pin.read();
			if(v != m_last_state)
			{
				send_spgm(debug, PSTR("button_intr missed some edge!\n"));
				force_update();
				m_callback(bool(m_last_state));
			}
		}
	}
	
	void process_intr()
	{
		const bool v = m_pin.read();
		if(v != m_last_state)
		{
			m_pin.intr_en(m_intr_num, false);
			m_filter.restart();
			m_last_state = v;
			m_async_callback(bool(m_last_state));
			m_sync_callback = true;
		}
	}
	
private:
	callback_type m_async_callback;
	const uint8_t m_intr_num;
	volatile bool m_sync_callback;
};
*/

#endif /* BUTTON_HPP_ */