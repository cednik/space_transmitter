#ifndef SCREENS_HPP
#define SCREENS_HPP

#include <functional>
#include <vector>
#include <memory>

#include "lcd_ui.hpp"

template <typename Display, typename Keyboard>
class Screen {
    Screen() = delete;
    Screen(Screen&) = delete;
public:
    typedef Screen<Display, Keyboard> Screen_t;

    Screen(Display& display, Keyboard& keyboard)
        : m_disp(display),
          m_keyboard(keyboard)
        {}
    
    virtual void show() = 0;
    virtual void process() = 0;
protected:
    Display& m_disp;
    Keyboard& m_keyboard;
};

template <typename Display, typename Keyboard>
class TextScreen
    :public Screen<Display, Keyboard>
{
    TextScreen() = delete;
    TextScreen(TextScreen&) = delete;
public:
    typedef std::function<void()> onFinish_t;
    typedef timeout::time_type time_type;

    TextScreen(
        Display& display,
        Keyboard& keyboard,
        onFinish_t on_finish,
        time_type show_time,
        bool interruptible,
        String text,
        bool show_now = true)
        : Screen<Display, Keyboard> (display, keyboard),
          m_on_finish(on_finish),
          m_timeout(show_time),
          m_interruptible(interruptible),
          m_text(text)
        {
            if (show_now)
                show();
        }
    
    virtual void show() {
        this->m_disp.clear();
        this->m_disp.noCursor();
        this->m_disp.enable_scrolling(false);
        send(this->m_disp, m_text);
        if (m_timeout.get_timeout() == 0)
            m_timeout.cancel();
        else
            m_timeout.restart();
    }
    virtual void process() {
        bool interrupt = false;
        if (this->m_keyboard.available()) {
            this->m_keyboard.read();
            interrupt = true;
        }
        if (m_timeout || interrupt) {
            m_on_finish();
        }
    }

    void on_finish(onFinish_t fcn) {
        m_on_finish = fcn;
    }
protected:
    onFinish_t m_on_finish;
    timeout m_timeout;
    bool m_interruptible;
    String m_text;
};

template <typename Display, typename Keyboard>
class InitScreen
    :public TextScreen<Display, Keyboard>
{
    typedef TextScreen<Display, Keyboard> Parent;
    InitScreen() = delete;
    InitScreen(InitScreen&) = delete;
public:
    InitScreen(
        Display& display,
        Keyboard& keyboard,
        typename Parent::onFinish_t on_finish,
        typename Parent::time_type show_time,
        bool interruptible = true,
        bool show_now = true)
        : TextScreen<Display, Keyboard> (display, keyboard, on_finish, show_time, interruptible, 
            String("                    ") + 
            String("     ESP32 chat     ") + 
            String("               v1.0 ") + 
            String(" by Kubas           "),
            show_now)
        {}
};

template <typename Display, typename Keyboard>
class MenuScreen
    :public Screen<Display, Keyboard>
{
    MenuScreen() = delete;
    MenuScreen(MenuScreen&) = delete;
public:
    typedef std::function<void()> onFinish_t;
    typedef timeout::time_type time_type;

    MenuScreen(
        Display& display,
        Keyboard& keyboard)
        : Screen<Display, Keyboard> (display, keyboard)
        {}
    
    virtual void show() {
        this->m_disp.clear();
        this->m_disp.noCursor();
        this->m_disp.enable_scrolling(false);
    }
    virtual void process() {
    }
};

template <typename Display, typename Keyboard>
class TerminalScreen
    :public Screen<Display, Keyboard>
{
    typedef ScrollLine<Display> TextLine;

    class InputLine {
        typedef ScrollLine<Display> Prompt;
        typedef typename Prompt::pos_type pos_type;
        typedef EditLine<Display> Line;
    public:
        InputLine(
            Display& display,
            pos_type line,
            pos_type begin,
            pos_type length,
            std::string prompt,
            std::string input = "",
            bool visible = true )
                : m_disp(display),
                  m_line(line),
                  m_begin(begin),
                  m_prompt(
                      display,
                      line,
                      begin,
                      prompt.length(),
                      prompt,
                      visible ),
                  m_input(
                      display,
                      line,
                      prompt.length(),
                      length - prompt.length(),
                      input,
                      visible ),
                  m_visible(visible)
            {
                show();
            }

            void show() {
                if (!m_visible)
                    return;
                m_prompt.restart();
                m_input.show();
                process();
            }

            void process() {
                m_prompt.process();
            }

            void visible(bool v = true, bool Clear = true) {
                m_visible = v;
                m_input.visible(m_visible, Clear);
                if (m_visible)
                    m_prompt.restart();
                else
                    m_prompt.stop(Clear);
            }

            bool is_visible() const { return m_visible; }

            Line& line() { return m_input; }
    private:
        Display& m_disp;
        pos_type m_line;
        pos_type m_begin;
        Prompt m_prompt;
        Line m_input;
        bool m_visible;
    };

    TerminalScreen() = delete;
    TerminalScreen(TerminalScreen&) = delete;
public:
    typedef std::function<void(std::string, TerminalScreen<Display, Keyboard>&)> Callback;

    TerminalScreen(
        Display& display,
        Keyboard& keyboard,
        std::string prompt,
        Callback onEnter = nullptr,
        int x = 0, int y = 0, int w = -1, int h = -1)
            : Screen<Display, Keyboard> (display, keyboard),
              m_x(clamp(x, 0, this->m_disp.width()-1)),
              m_y(clamp(y, 0, this->m_disp.height()-1)),
              m_w(clamp(w == -1 ? this->m_disp.width() : w, 1, this->m_disp.width()-m_x)),
              m_h(clamp(h == -1 ? this->m_disp.height() : h, 2, this->m_disp.height()-m_y)),
              m_prompt(prompt),
              m_output(),
              m_input(this->m_disp, m_y + m_h - 1, m_x, m_w, m_prompt, "", false),
              m_onEnter(onEnter)
        {
            for(size_t i = 0; i != m_h-1; ++i)
                m_output.emplace_back(new TextLine(
                    this->m_disp, m_y+i, m_x, m_w, "", false));
        }
    
    virtual void show() {
        this->m_disp.enable_scrolling(false);
        for(auto& l: m_output)
            l->restart();
        m_input.visible(true);
        m_input.show();
    }

    virtual void onEnter(Callback fcn) { m_onEnter = fcn; }

    virtual void printLine(const std::string& line) {
        for (auto p = m_output.begin() + 1; p != m_output.end(); ++p)
            (p-1)->get()->set(p->get()->text());
        (m_output.end()-1)->get()->set(line);
        process();
    }

    virtual void process() {
        for(auto& l: m_output)
            l->process();
        if (this->m_keyboard.available()) {
            char c = this->m_keyboard.read();
            auto& input = m_input.line();
            switch(c) {
                case PS2_LEFTARROW: input.left(); break;
                case PS2_RIGHTARROW: input.right(); break;
                case PS2_BACKSPACE: input.erase(); break;
                case PS2_ESC: input.clear(); break;
                case PS2_ENTER:
                    printLine(m_prompt + m_input.line().text());
                    if (m_onEnter)
                        m_onEnter(m_input.line().text(), *this);
                    m_input.line().clear();
                    break;
                default: input.push(c); break;
            }
        }
    }

private:

    int m_x;
    int m_y;
    int m_w;
    int m_h;
    std::string m_prompt;
    std::vector<std::unique_ptr<TextLine> > m_output;
    InputLine m_input;
    Callback m_onEnter;
};

#endif
