#ifndef KEYBINDING_HPP
#define KEYBINDING_HPP

#include <xcb/xcb.h>
#include <vector>
#include <functional>
#include <initializer_list>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <X11/X.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <xcb/xcb.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>     // For fork() and exec()
#include <sys/wait.h>   // For waitpid()
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xcb/xcb_cursor.h> /* For cursor */
#include <xcb/xcb_icccm.h>
#include <iostream>

struct Keybinding {
    xcb_keycode_t keycode;
    uint16_t modifiers;
    std::function<void()> action;

    Keybinding(xcb_keycode_t kc, uint16_t mods, std::function<void()> act)
        : keycode(kc), modifiers(mods), action(std::move(act)) {}
};

class KeybindManager {
public:
    KeybindManager(std::initializer_list<Keybinding> bindings) 
    {
        for (const auto& binding : bindings) 
        {
            keybindings.push_back(binding);
        }
    }

    void addKeybinding(const Keybinding& keybinding) 
    {
        keybindings.push_back(keybinding);
    }

    void handleKeyPress(xcb_key_press_event_t* key_event) 
    {
        for (const auto& kb : keybindings) 
        {
            if (key_event->detail == kb.keycode && (key_event->state & kb.modifiers) == kb.modifiers) 
            {
                kb.action();
                break;  // Remove this if you want to allow multiple actions per key press
            }
        }
    }

private:
    std::vector<Keybinding> keybindings;
};

/************************* USAGE *************************

KeybindManager({
    {   q,      SHIFT | CTRL, []() { launchTerminal(); }},
    {}
})

*/

// // Function to handle key press events based on the initialization list
// void handleKeyPressEvents(xcb_generic_event_t *ev, std::initializer_list<std::tuple<int, int, KeyPressHandlerFunc>> keyBindings) {
//     xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)ev;

//     for (const auto& binding : keyBindings) {
//         int key = std::get<0>(binding);
//         int mod = std::get<1>(binding);
//         KeyPressHandlerFunc func = std::get<2>(binding);

//         if (key_event->detail == key && (key_event->state & mod)) {
//             func();  // Invoke the function associated with the key press
//         }
//     }
// }

// // Example usage:
// int main() {
//     // Define key bindings
//     std::initializer_list<std::tuple<int, int, KeyPressHandlerFunc>> bindings = {
//         {t, CTRL | ALT, launchTerminal},
//         {q, SHIFT | ALT, killSession},
//         {f, SUPER, [](){ borrowed::maxwin(0, 0); }}  // Lambda function for borrowed::maxwin
//     };

//     // Use the function to handle key press events
//     handleKeyPressEvents(/* your xcb event */, bindings);

//     return 0;
// }

#endif/*KEYBINDING_HPP*/