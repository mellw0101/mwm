// // #include <cstddef>
// #include <cstdint>
// #include "Log.hpp"
// #include <xcb/xcb.h>
// #include <xcb/xproto.h>
// #include <xcb/xcb_ewmh.h>
// #include "tools.hpp"
// #include "xcb.hpp"

// xcb_connection_t *conn = nullptr;

// namespace xcb {
//     xcb_intern_atom_cookie_t intern_atom_cookie(xcb_connection_t *__c, const char *__name) {
//         return xcb_intern_atom(
//             __c,
//             0,
//             slen(__name),
//             __name
            
//         );

//     }
//     xcb_intern_atom_reply_t *intern_atom_reply(xcb_connection_t *__c, xcb_intern_atom_cookie_t __cookie) {
//         return xcb_intern_atom_reply(__c, __cookie, nullptr);

//     }
//     xcb_atom_t intern_atom(xcb_connection_t *__c, const char *__name) {
//         xcb_intern_atom_reply_t *rep = intern_atom_reply(__c, intern_atom_cookie(__c, __name));
//         if (!rep) {
//             loutE << "rep = nullptr could not get intern_atom_reply" << loutEND;
//             return XCB_ATOM_NONE;
        
//         } xcb_atom_t atom(rep->atom);
//         free(rep);
//         return atom;

//     }
//     bool window_exists(xcb_connection_t *__c, uint32_t __w) {
//         xcb_generic_error_t *err;
//         free(xcb_query_tree_reply(__c, xcb_query_tree(__c, __w), &err));
//         if (err != NULL) {
//             free(err);
//             return false;

//         }
//         return true;

//     }
//     uint32_t gen_Xid() {
//         uint32_t w = xcb_generate_id(conn);
//         return (window_exists(conn, w)) ? w : XCB_NONE;

//     }
//     void window_stack(xcb_connection_t *__c, uint32_t __window1, uint32_t __window2, uint32_t __mode) {
//         if (__window2 == XCB_NONE) return;
        
//         uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
//         uint32_t values[] = {__window2, __mode};
        
//         xcb_configure_window(__c, __window1, mask, values);
        
//     }

// }