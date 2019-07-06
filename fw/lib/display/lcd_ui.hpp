#ifndef LCD_UI_HPP
#define LCD_UI_HPP

#include <string>

template <typename Display>
void clearLine(
    Display& display,
    typename Display::coor_type line,
    typename Display::coor_type begin = 0,
    typename Display::coor_type length = 0,
    char eraser = ' ') {
        if (length == 0)
            length = display.width() - begin;
        display.save_state();
        display.noCursor();
        display.setCursor(begin, line);
        for(typename Display::coor_type i = 0; i != length; ++i)
            display.write(eraser);
        display.restore_state();
}

template <typename Display>
class ScrollLine
{
public:
    typedef uint8_t pos_type;
    typedef timeout::time_type time_type;

    ScrollLine(
        Display& disp,
        pos_type line,
        pos_type begin,
        pos_type length,
        std::string text = "",
        bool visible = true)
        : m_disp(disp),
          m_line(line),
          m_begin(begin),
          m_length(length == 0 ? disp.width() - begin : length),
          m_visible(0),
          m_begin_time(msec(2000)),
          m_middle_time(msec(250)),
          m_end_time(msec(1000)),
          m_timeout(m_begin_time),
          m_text(text),
          m_modified(true)
    {
        if (visible)
            m_timeout.force();
        else
            m_timeout.cancel();
    }

    ScrollLine& set(const std::string& str) {
        m_text = str;
        m_modified = true;
        m_timeout.force();
        return *this;
    }

    ScrollLine& operator = (const std::string& str) {
        return set(str);
    }

    void clear() { *this = ""; }

    operator std::string() const {
        return m_text;
    }

    std::string text() const { return m_text; }

    void stop(bool Clear = false) {
        m_timeout.stop();
        process();
        if (Clear)
            clearLine(m_disp, m_line, m_begin, m_length);
    }
    void start() { m_timeout.start(); }
    void restart() {
        m_modified = true;
        m_timeout.restart();
        m_timeout.force();
        process();
    }

    void process() {
        if (m_timeout) {
            m_timeout.ack();
            if (m_text.length() > m_length) {
                if (m_modified) {
                    m_modified = false;
                    m_visible = 0;
                }
                auto cursor = m_disp.cursor_state();
                auto pos = m_disp.getCursor();
                m_disp.noCursor();
                m_disp.setCursor(m_begin, m_line);
                const pos_type end = m_visible + m_length;
                for (pos_type i = m_visible; i != end; ++i)
                    m_disp.write(m_text[i]);
                m_disp.setCursor(pos);
                m_disp.cursor(cursor);
                if (m_visible == 0) {
                    m_timeout.set_timeout(m_begin_time);
                    ++m_visible;
                } else {
                    if (m_visible == (m_text.length() - m_length)) {
                        m_timeout.set_timeout(m_end_time);
                        m_visible = 0;
                    } else {
                        m_timeout.set_timeout(m_middle_time);
                        ++m_visible;
                    }
                }
            } else {
                if (m_modified) {
                    m_modified = false;
                    auto cursor = m_disp.cursorState();
                    auto pos = m_disp.getCursor();
                    m_disp.noCursor();
                    m_disp.setCursor(m_begin, m_line);
                    m_disp.print(m_text.c_str());
                    for (pos_type i = m_text.length(); i != m_length; ++i)
                        m_disp.write(' ');
                    m_disp.setCursor(pos);
                    m_disp.cursor(cursor);
                }
            }
        }
    }
private:

    Display& m_disp;
    pos_type m_line;
    pos_type m_begin;
    pos_type m_length;
    pos_type m_visible;
    time_type m_begin_time;
    time_type m_middle_time;
    time_type m_end_time;
    timeout m_timeout;
    std::string m_text;
    bool m_modified;
};

template <typename Display>
class EditLine
{
public:
    typedef uint8_t pos_type;
    typedef timeout::time_type time_type;

    EditLine(
        Display& disp,
        pos_type line,
        pos_type begin = 0,
        pos_type length = 0,
        std::string text = "",
        bool visible = true)
        : m_disp(disp),
          m_line(line),
          m_begin(begin),
          m_length(length == 0 ? disp.width() - begin : length),
          m_visible(0),
          m_cursor(0),
          m_text(),
          m_blink(true),
          m_shown(visible)
    {
        *this = text;
    }

    EditLine& operator = (const std::string& str) {
        m_text = str;
        m_cursor = m_text.length();
        if (m_text.length() > m_length)
            m_visible = m_cursor - m_length;
        else
            m_visible = 0;
        show();
        return *this;
    }

    operator std::string() const {
        return m_text;
    }

    std::string text() const { return m_text; }

    void left() {
        if (m_cursor == 0)
            return;
        const bool last = m_cursor == m_text.length();
        if (--m_cursor < m_visible || (last && m_visible != 0)) {
            --m_visible;
            show();
        } else {
            m_disp.setCursor(m_begin + m_cursor - m_visible, m_line);
            if (m_blink)
                m_disp.blink();
        }
    }

    void right() {
        auto l = m_text.length();
        if (m_cursor == l)
            return;
        if (++m_cursor >= (m_visible + m_length)) {
            ++m_visible;
            show();
        } else {
            m_disp.setCursor(m_begin + m_cursor - m_visible, m_line);
            if (m_blink) {
                if (m_cursor == l)
                    m_disp.cursor();
                else
                    m_disp.blink();
            }
        }
    }

    void push(char c) {
        if (!isPrintable(c))
            return;
        if (m_cursor == m_text.length()) {
            m_text += c;
        } else {
            std::string tmp = m_text.substr(0, m_cursor);
            tmp += c;
            tmp += m_text.substr(m_cursor);
            m_text = tmp;
        }
        if ((++m_cursor - m_visible) >= m_length)
            ++m_visible;
        show();
    }

    void erase() {
        auto l = m_text.length();
        if (l == 0)
            return;
        if (m_cursor == l) {
            m_text.erase(--m_cursor, 1);
            if (m_visible > 0)
                --m_visible;
        } else if (m_cursor == 0) {
            m_text = m_text.substr(1);
            if (m_visible > 0)
                --m_visible;
        } else {
            m_text.erase(--m_cursor, 1);
            if ((--l - m_visible) < m_length && m_visible != 0)
                --m_visible;
        }
        show();
    }

    void clear() {
        *this = "";
    }

    void show() {
        if (!m_shown)
            return;
        auto l = m_text.length();
        m_disp.noCursor();
        m_disp.setCursor(m_begin, m_line);
        if (l >= m_length) {
            pos_type end = m_visible + m_length;
            if (end > l)
                end = l;
            for (pos_type i = m_visible; i != end; ++i) {
                m_disp.write(m_text[i]);
                Serial.write(m_text[i]);
            }
            if (m_cursor == l)
                m_disp.write(' ');
        } else {
            m_disp.print(m_text.c_str());
            for (pos_type i = l; i != m_length; ++i)
                m_disp.write(' ');
        }
        m_disp.setCursor(m_begin + m_cursor - m_visible, m_line);
        if (m_blink) {
            if (m_cursor >= l)
                m_disp.cursor();
            else
                m_disp.blink();
        }
    }

    void visible(bool v, bool Clear = true) {
        m_shown = v;
        if (!m_shown && Clear)
            clearLine(m_disp, m_line, m_begin, m_length);
    }

    bool is_visible() const { return m_shown; }

private:

    Display& m_disp;
    pos_type m_line;
    pos_type m_begin;
    pos_type m_length;
    pos_type m_visible;
    pos_type m_cursor;
    std::string m_text;
    bool m_blink;
    bool m_shown;
};

#endif
