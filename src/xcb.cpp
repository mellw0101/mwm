#include "xcb.hpp"
#include "tools.hpp"
#include "Log.hpp"
#include <cstdint>
#include <xcb/xcb.h>

xcb_intern_atom_cookie_t x::intern_atom_cookie(const char *__name) {
    return xcb_intern_atom(
        _conn,
        0,
        slen(__name),
        __name
        
    );

}
xcb_intern_atom_reply_t *x::intern_atom_reply(xcb_intern_atom_cookie_t __cookie) {
    return xcb_intern_atom_reply(_conn, __cookie, nullptr);

}
xcb_atom_t x::intern_atom(const char *__name) {
    xcb_intern_atom_reply_t *rep = intern_atom_reply(intern_atom_cookie(__name));
    if (!rep) {
        loutE << "rep = nullptr could not get intern_atom_reply" << loutEND;
        free(rep);
        return XCB_ATOM_NONE;
    
    } xcb_atom_t atom(rep->atom);
    free(rep);
    return atom;

}
bool x::window_exists(uint32_t __w) {
    xcb_generic_error_t *err;
    free(xcb_query_tree_reply(_conn, xcb_query_tree(_conn, __w), &err));
    if (err != NULL) {
        free(err);
        return false;

    }
    return true;

}
uint32_t x::gen_Xid() {
    return xcb_generate_id(_conn);

}
void x::window_stack(uint32_t __window1, uint32_t __window2, uint32_t __mode) {
    if (__window2 == XCB_NONE) return;
    
    uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
    uint32_t values[] = {__window2, __mode};
    
    xcb_configure_window(_conn, __window1, mask, values);
    
}
bool x::is_flag_set(unsigned int __f) {
    return (_flags & (1ULL << __f)) != 0;

}
void x::set_flag(unsigned int __f) {
    _flags |= 1ULL << __f;

}
void x::clear_flag(unsigned int __f) {
    _flags &= ~(1ULL << __f);

}
void x::toggle_flag(unsigned int __f) {
    _flags ^= 1ULL << __f;

}
x::x(xcb_connection_t *__conn) : _conn(__conn) {
    if (xcb_connection_has_error(_conn)) {
        loutE << "could not connect to the exisiting 'xcb_connection_t *'" << loutEND;
        _conn = nullptr;
        set_flag(X_CONN_ERROR);
    
    }
    else {
        loutI << "success x is now connected to the server" << loutEND;
        
    }

}
// class x *connect_to_server(xcb_connection_t *__conn) {
//     return new class x(__conn);
    
// }
uint64_t &x::check_conn() {
    return _flags;
    
}