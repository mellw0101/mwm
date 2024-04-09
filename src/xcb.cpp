#include "xcb.hpp"
// #include "data.hpp"
#include "data.hpp"
#include "defenitions.hpp"
#include "tools.hpp"
#include "Log.hpp"
#include <cstdint>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

xcb_intern_atom_cookie_t xcb::intern_atom_cookie(const char *__name) {
    return xcb_intern_atom(
        conn,
        0,
        slen(__name),
        __name
        
    );

}
xcb_intern_atom_reply_t *xcb::intern_atom_reply(xcb_intern_atom_cookie_t __cookie) {
    return xcb_intern_atom_reply(conn, __cookie, nullptr);

}
xcb_atom_t xcb::intern_atom(const char *__name) {
    xcb_intern_atom_reply_t *rep = intern_atom_reply(intern_atom_cookie(__name));
    if (!rep) {
        loutE << "rep = nullptr could not get intern_atom_reply" << loutEND;
        free(rep);
        return XCB_ATOM_NONE;
    
    } xcb_atom_t atom(rep->atom);
    free(rep);
    return atom;

}
/* bool xcb::window_exists(uint32_t __w) {
    xcb_generic_error_t *err;
    free(xcb_query_tree_reply(_conn, xcb_query_tree(_conn, __w), &err));
    if (err != NULL) {
        free(err);
        return false;
    }
    return true;
} */
bool xcb::window_exists(uint32_t __w) {
    xcb_generic_error_t *err = NULL;
    auto *reply = xcb_query_tree_reply(conn, xcb_query_tree(conn, __w), &err);
    if (reply) free(reply); // Correctly free the reply if it's non-null

    if (!err) return true; // If there was no error, assume the window exists
    loutE << err->error_code << loutEND;
    free(err); // Free the error if it's non-null
    return false; // Return false if there was an error, assuming it indicates non-existence

}

uint32_t
xcb::gen_Xid()
{
    if (_flags & (1ULL << X_CONN_ERROR))
    {
        return -1;
    }
    uint32_t w = xcb_generate_id(conn);

    if (w == -1)
    {
        set_flag(X_ID_GEN_ERROR);
        loutE << "Failed to generate Xid" << loutEND;
    }
    else
    {
        clear_flag(X_ID_GEN_ERROR);
    }
    return w;
}

void xcb::window_stack(uint32_t __window1, uint32_t __window2, uint32_t __mode)
{
    if (__window2 == XCB_NONE) return;
    
    uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
    uint32_t values[] = {__window2, __mode};
    
    xcb_configure_window(conn, __window1, mask, values);   
}
bool xcb::is_flag_set(unsigned int __f)
{
    return ( _flags & ( 1ULL << __f )) != 0;
}
void xcb::set_flag(unsigned int __f)
{
    _flags |= 1ULL << __f;
}
void xcb::clear_flag(unsigned int __f)
{
    _flags &= ~(1ULL << __f);
}
void xcb::toggle_flag(unsigned int __f)
{
    _flags ^= 1ULL << __f;
}

xcb::xcb(/* xcb_connection_t *__conn,  */xcb_screen_t *__s) : /* _conn(__conn), */ _s(__s)
{
    if (xcb_connection_has_error(conn))
    {
        loutE << "could not connect to the exisiting 'xcb_connection_t *'" << loutEND;
        _flags |= 1ULL << X_CONN_ERROR;
        /* _conn = nullptr; */
    }
    else
    {
        _flags &= ~(1ULL << X_CONN_ERROR);
        loutI << "success x is now connected to the server" << loutEND;    
    }

}
// class x *connect_to_server(xcb_connection_t *__conn) {
//     return new class x(__conn);
    
// }
uint64_t &xcb::check_conn()
{
    return _flags;    
}

void
xcb::create_w(uint32_t __pw, uint32_t __w, int16_t __x, int16_t __y,
                      uint16_t __width, uint16_t __height)
{
    // Create the window
    _cookie = xcb_create_window(
        conn,
        24,
        __w,
        __pw,
        __x,
        __y,
        __width,
        __height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        _s->root_visual,
        0,
        nullptr
    );
    check_error();
    
    if ((_flags & 1ULL << X_REQ_ERROR) != 0)
    {
        loutE << "window creation failed" << loutEND;
        _flags |= 1ULL << X_W_CREATION_ERR;
    }      
    else
    {
        _flags &= ~(1ULL << X_W_CREATION_ERR);    
    }
}

void 
xcb::check_error()
{
    _err = xcb_request_check(conn, _cookie);
    if (_err)
    {
        loutE << ERRNO_MSG("XCB Error occurred. Error code:") << _err->error_code << loutEND;
        _flags |= 1ULL << X_REQ_ERROR; /* Set bit to true */
    }
    else
    {
        _flags &= ~(1ULL << X_REQ_ERROR); /* Set bit to false */    
    }
}

void
xcb::mapW(uint32_t __w)
{
    VoidC cookie = xcb_map_window(conn, __w);
    CheckVoidC(cookie, "window map failed");
    xcb_flush(conn);
}

// void atoms_t::fetch_atom_data(xcb_connection_t *conn, char *__name) {
//     // atom_t *atom = new atom_t;
//     atom_t *atom = Malloc<atom_t>().allocate();
//     intern_atom_cok_t cookie(conn, 0, __name);
//     xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, nullptr);
//     if (!reply) {
//         loutE << "reply = nullptr" << loutEND;
        
//     }

//     atom->response_type = reply->response_type;
//     atom->pad0          = reply->pad0;
//     atom->sequence      = reply->sequence;
//     atom->length        = reply->length;
//     atom->atom          = reply->atom;

//     // _data.push_back(std::move(atom));

// }
// atoms_t::atoms_t(xcb_connection_t *conn, char **__atoms){
//     for (int i = 0; __atoms[i]; ++i) {
//         fetch_atom_data(conn, __atoms[i]);

//     }

// }
// atoms_t::~atoms_t() {
//     for (const auto &atom : _data) Malloc<atom_t>().deallocate(atom); /* delete atom; */
    
// }

uint32_t
xcb::get_atom(uint8_t __only_if_exists, const char *__atom_name)
{
    iAtomR r(0, __atom_name);
    return (r.is_not_valid()) ? XCB_NONE : r.Atom();
}