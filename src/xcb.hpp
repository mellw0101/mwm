#ifndef XCB__HPP
#define XCB__HPP

#include <cstdint>
#define X_CONN_ERROR 1  

#include <xcb/xcb.h>

class x {
    private:
        xcb_connection_t *_conn = nullptr;
        uint64_t _flags = 0;

        bool is_flag_set(unsigned int __f);
        void set_flag(unsigned int __f);
        void clear_flag(unsigned int position);
        void toggle_flag(unsigned int position);
        

    public:
        xcb_intern_atom_cookie_t intern_atom_cookie(const char *__name);
        xcb_intern_atom_reply_t *intern_atom_reply(xcb_intern_atom_cookie_t __cookie);
        xcb_atom_t intern_atom(const char *__name);
        bool window_exists(uint32_t __w);
        uint32_t gen_Xid();
        void window_stack(uint32_t __window1, uint32_t __window2, uint32_t __mode);

        uint64_t &check_conn();
        operator uint64_t&() { return _flags; }

        x(xcb_connection_t *__conn);

}; static x *x(nullptr);

inline class x *connect_to_server(xcb_connection_t *__conn) {
    return new class x(__conn);
    
}


#endif/* XCB__HPP */