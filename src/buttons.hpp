#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include "button.hpp"

class buttons
{
    public: // public constructors
        buttons() {}
    ;
    public: // public variables
        std::vector<button> list;
    ;
    public: // public methods
        void add(const char * name, std::function<void()> action)
        {
            button button;
            button.name = name;
            button.action(action);
            list.push_back(button);
        }
        int size()
        {
            return list.size();
        }
        int index()
        {
            return list.size() - 1;
        }
        void run_action(const uint32_t & window)
        {
            for (const auto & button : list) 
            {
                if (window == button.window)
                {
                    button.activate();
                    return;
                }
            }
        }
    ;
};

#endif // BUTTONS_HPP