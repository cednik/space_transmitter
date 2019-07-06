#pragma once

#include "PCF8574.hpp"
#include "../util/math.hpp"
#include <stack>

template <int WIDTH, int HEIGHT, int E, int RW, int RS, int DATA, int BL>
class Display_t
	:public Print
{
public:
	typedef uint8_t coor_type;

	struct coords_type {
		coor_type x;
		coor_type y;
		coords_type(coor_type x, coor_type y): x(x), y(y) {}
	};

	Display_t(port8_t& port)
		:m_port(port),
		 m_x(0),
		 m_y(0),
		 m_entry_mode(1<<2),
		 m_on_off_control(1<<3),
		 m_enable_scrolling(true),
		 m_line_fed(false)
	{
		for(coor_type x = 0; x != c_width; ++x)
			for(coor_type y = 0; y != c_height; ++y)
				m_buff[x][y] = ' ';
	}

	void init()
	{
		m_port = uint8_t(~((1<<E) | (1<<RW) | (1<<RS) | (1<<BL)));
		wait(msec(16));
		_write_nibble(0x03);
		wait(msec(5));
		_write_nibble(0x03);
		wait(usec(200));
		_write_nibble(0x03);
		static const uint8_t init_values[] = { 0x32, 0x28, 0x08, 0x01, 0x02 };
		for(uint8_t i = 0; i != sizeof init_values; ++i)
		{
			uint8_t byte = init_values[i];
			_write_nibble(byte>>4);
			wait(usec(40));
			_write_nibble(byte);
			wait(usec(40));
		}
		_write_instr(0x14);
		_write_instr(0x0E);
		_write_instr(0x06);
		clear();
		on();
		automove(RIGHT);
		cursor(ON);
	}

	void clear()
	{
		_write_instr(0x01);
		wait(usec(1500));
		m_x = 0;
		m_y = 0;
		//_set_cursor_pos();
		for(coor_type x = 0; x != c_width; ++x)
			for(coor_type y = 0; y != c_height; ++y)
				m_buff[x][y] = ' ';
	}

	void home()
	{
		_write_instr(0x02);
		wait(usec(1500));
		m_x = 0;
		m_y = 0;
	}

	void write(char c) { write(uint8_t(c)); }

	size_t write(uint8_t c)
	{
		switch(c)
		{
		case '\b': // BS
		case 0x7F: // DEL
			backspace();
			break;

		case '\t': // HT
			x((1+(x()>>2))<<2);
			break;

		case '\n': // LF
			if(line_fed())
				break;
			x(0);
		case '\v': // VT
			move_down();
			break;

		case '\r': // CR
			x(0);
			break;

		case '\f': // FF
			clear();
			break;

		default:
			if(c > 0x1F)
			{
				m_buff[m_x][m_y] = c;
				_write_data(c);
				_inc_cursor_pos();
			}
			break;
		}
		return 1;
	}

	void move_to(const coor_type& x, const coor_type& y)
	{
		m_x = clamp(x, 0, c_width - 1);
		m_y = clamp(y, 0, c_height - 1);
		_set_cursor_pos();
	}

	void move_to(const coords_type& pos) { move_to(pos.x, pos.y); }

	void move_right(const coor_type& n = 1)
	{
		m_x += clamp(n, 0, c_width - m_x - 1);
		_set_cursor_pos();
	}

	void move_left(const coor_type& n = 1)
	{
		m_x -= clamp(n, 0, m_x);
		_set_cursor_pos();
	}

	void move_down(const coor_type& n = 1)
	{
		coor_type dy = clamp(n, 0, this->c_height - m_y - 1);
		if(dy == 0 && n != 0)
			_scroll_disp();
		else
			m_y += dy;
		_set_cursor_pos();
	}

	void move_up(const coor_type& n = 1)
	{
		m_y -= clamp(n, 0, m_y);
		_set_cursor_pos();
	}

	void move_forward(coor_type n = 1)
	{
		n = clamp(n, 0, (c_height - m_y) * c_width - m_x - 1);
		for(coor_type i = 0; i != n; ++i)
		{
			if(++m_x == c_width)
			{
				++m_y;
				m_x = 0;
			}
		}
		_set_cursor_pos();
	}

	void move_backward(coor_type n = 1)
	{
		n = clamp(n, 0, m_y * c_width + m_x);
		for(coor_type i = 0; i != n; ++i)
		{
			if(m_x == 0)
			{
				--m_y;
				m_x = c_width;
			}
			--m_x;
		}
		_set_cursor_pos();
	}

	coor_type x() const { return m_x; }

	coor_type y() const { return m_y; }

	void x(const coor_type& X)
	{
		m_x = clamp(X, 0, c_width - 1);
		_set_cursor_pos();
	}

	void y(const coor_type& Y)
	{
		m_y = clamp(Y, 0, c_width - 1);
		_set_cursor_pos();
	}

	void backspace()
	{
		move_backward();
		write(' ');
		move_backward();
	}

	coor_type width() const { return c_width; }

	coor_type height() const { return c_height; }

	bool enable_scrolling(const bool& en)
	{
		bool old = m_enable_scrolling;
		m_enable_scrolling = en;
		return old;
	}

	bool is_scrolling_enabled() const { return m_enable_scrolling; }

	bool line_fed()
	{
		bool tmp = m_line_fed;
		m_line_fed = false;
		return tmp;
	}

	void set_led(const uint8_t& index, const bool& value)
	{
		switch(index)
		{
			case 0:
				backlight(value);
				break;
		}
	}

	void backlight(const bool& en = true)
	{
		if(en)
			m_port.set_bit(BL);
		else
			m_port.clear_bit(BL);
	}

	bool is_backlight_on() const { return m_port.get_bit(BL); }

	enum dir_t {
		LEFT = false,
		RIGHT = true
	};

	void automove(dir_t dir) { _set_ctrl_bit(bool(dir), 1, m_entry_mode); }

	void displayshift(bool en)  { _set_ctrl_bit(en, 0, m_entry_mode); }
	
	void on(bool en = true)  { _set_ctrl_bit(en, 2, m_on_off_control); }
	void off() { on(false); }
	bool is_on() const { return m_on_off_control & (1<<2); }

	enum cursor_t {
		OFF = 0,
		ON = 2,
		BLINKING = 3
	};

	void cursor(cursor_t state = ON)
	{
		m_on_off_control = (m_on_off_control & ~3) | (state & 3);
		_write_instr(m_on_off_control);
	}

	cursor_t cursor_state() const {
		switch (m_on_off_control & 3) {
			default:
			case OFF: return OFF;
			case ON: return ON;
			case BLINKING: return BLINKING;
		}
	}

	void shift_cursor(dir_t dir)
	{
		_write_instr(0x10 | bool(dir)<<2);
		if(dir)
		{
			if(++m_x == c_width)
				m_x = 0;
		}
		else
		{
			if(m_x == 0)
				m_x = c_width - 1;
			else
				--m_x;
		}
	}

	void shift_display(dir_t dir)
	{
		_write_instr(0x18 | bool(dir)<<2);
	}

	coords_type get_cursor_pos() const { return coords_type(m_x, m_y); }

// Arduino interface
	void begin(coor_type, coor_type) { init(); }
	//void clear();
	//void home();
	void setCursor(coor_type col, coor_type row) { move_to(col, row); }
	//virtual size_t write(uint8_t c);
	//print...
	//void cursor(cursor_t state = ON);
	void noCursor() { cursor(OFF); }
	void blink() { cursor(BLINKING); }
	void noBlink() { cursor(OFF); }
	void display() { on(); }
	void noDisplay() { off(); }
	void scrollDisplayLeft() { shift_display(LEFT); }
	void scrollDisplayRight() { shift_display(RIGHT); }
	void autoscroll() { displayshift(true); }
	void noAutoscroll() { displayshift(false); }
	void leftToRight() { automove(RIGHT); }
	void rightToLeft() { automove(LEFT); }
	void createChar(uint8_t location, uint8_t charmap[]) {
		_write_instr(0x40 | ((location & 7) << 3));
		for (uint8_t i = 0; i != 8; ++i)
			_write_data(charmap[i]);
	}
	//createChar with PROGMEM input
	void createChar(uint8_t location, const char *charmap) {
		_write_instr(0x40 | ((location & 7) << 3));
		for (uint8_t i = 0; i != 8; ++i)
			_write_data(*charmap++);
	}

// not original Arduinu, but Arduino-like and usefull
	void setCursor(coords_type coor) { move_to(coor); }
	coords_type getCursor() const { return coords_type(m_x, m_y); }
	cursor_t cursorState() const { return cursor_state(); }

	void save_coords() { m_coords_stack.push(getCursor()); }
	void save_cursor() { m_cursor_stack.push(cursorState()); }
	void save_state() {
		save_coords();
		save_cursor();
	}

	bool restore_coords() {
		if (m_coords_stack.empty())
			return false;
		setCursor(m_coords_stack.top());
		m_coords_stack.pop();
		return true;
	}

	bool restore_cursor() {
		if (m_cursor_stack.empty())
			return false;
		cursor(m_cursor_stack.top());
		m_cursor_stack.pop();
		return true;
	}

	void restore_state() {
		restore_coords();
		restore_cursor();
	}

private:
	void _set_ctrl_bit(const bool val, const uint8_t pos, uint8_t& where)
	{
		if (val)
			where |= (1<<pos);
		else
			where &= ~(1<<pos);
		_write_instr(where);
	}

	void _set_cursor_pos()
	{
		static const coor_type line_tab[c_height] = { 0, 64, c_width, c_width + 64 };
		_write_instr(0x80 | (m_x + line_tab[m_y]));
	}

	void _inc_cursor_pos()
	{
		if(++m_x == c_width)
		{
			m_x = 0;
			m_line_fed = true;
			if(++m_y == c_height)
			{
				m_y = c_height - 1;
				_scroll_disp();
			}
			_set_cursor_pos();
		}
		else
			m_line_fed = false;
	}

	void _scroll_disp()
	{
		if(!m_enable_scrolling)
			return;
		coor_type x = m_x;
		coor_type y = m_y;
		for(m_y = 0; m_y != c_height-1; ++m_y)
		{
			m_x = 0;
			_set_cursor_pos();
			for(; m_x != c_width; ++m_x)
			{
				m_buff[m_x][m_y] = m_buff[m_x][m_y+1];
				_write_data(m_buff[m_x][m_y]);
			}
		}
		m_x = 0;
		m_y = c_height - 1;
		_set_cursor_pos();
		for(; m_x != c_width; ++m_x)
		{
			m_buff[m_x][m_y] = ' ';
			_write_data(' ');
		}
		m_x = x;
		m_y = y;
		_set_cursor_pos();
	}

	void _write_nibble(uint8_t nibble)
	{
		m_port.modify((1<<E) | ((nibble & 0x0F)<<DATA), (1<<E) | (0x0F<<DATA));
		m_port.clear_bit(E);
	}

	void _write_data(const uint8_t& byte)
	{
		m_port.clear_bit(RW, false);
		_write_nibble(byte>>4);
		_write_nibble(byte);
		//wait(usec(43));
	}

	void _write_instr(const uint8_t& byte)
	{
		m_port.clear_bit(RS, false);
		_write_data(byte);
		m_port.set_bit(RS, false);
	}

	static const coor_type c_width = WIDTH;
	static const coor_type c_height = HEIGHT;

	port8_t& m_port;

	coor_type m_x;
	coor_type m_y;

	uint8_t m_entry_mode;
	uint8_t m_on_off_control;

	bool m_enable_scrolling;

	bool m_line_fed;

	char m_buff[c_width][c_height];

	std::stack<coords_type> m_coords_stack;
	std::stack<cursor_t> m_cursor_stack;
};
