/* DEFENITIONS_HPP */
#ifndef DEFENITIONS_HPP
#define DEFENITIONS_HPP

/* main.cpp */
#define conn_err()  error::conn_error(conn ,__func__)                     
#define check_conn  error::conn_error(conn ,__func__)
#define check_cookie(cookie)  error::cookie_error(cookie, __func__)

#define check_error(cookie, err_msg) check_err(conn, cookie, __func__, err_msg)

/* LOG DEFENITIONS */
#define LOG_ev_response_type(ev)    	    Log::xcb_event_response_type(__func__, ev);
#define LOG_func                    	    Log::FUNC(__func__);
#define LOG_error(message)          	    Log::ERROR(__FUNCTION__, "]:[", message);
#define LOG_info(message)           	    Log::INFO(__FUNCTION__, "]:[", message);
#define LOG_f_info(message)         	    Log::FUNC_INFO(__FUNCTION__, "]:[", message);
#define LOG_warning(message)        	    Log::WARNING(__FUNCTION__, "]:[", message);
#define LOG_start()                 	    Log::Start("mwm");
#define log_event_response_typ()		    Log::xcb_event_response_type(__func__, e->response_type)
#define log_info(message)                   log.log(INFO, __func__, message)
#define log_error(message)                  log.log(ERROR, __FUNCTION__, message)
#define log_error_code(message, err_code)   log.log(ERROR, __FUNCTION__, message, err_code)
#define log_win(win_name ,window)           log.log(INFO, __func__, win_name + std::to_string(window))

/* XCB DEFENITIONS */
#define XCB_map_window(win)             xcb.map_window(conn, win, __FUNCTION__)
#define XCB_flush()                     xcb.flush(conn, __FUNCTION__)

/* MOD_MASK DEFENITIONS */
#define SHIFT   XCB_MOD_MASK_SHIFT
#define ALT     XCB_MOD_MASK_1
#define CTRL    XCB_MOD_MASK_CONTROL
#define SUPER   XCB_MOD_MASK_4

/* KEY DEFENITIONS */
#define T       0x74
#define Q       0x71
#define F       0x66
#define F11     0xffc8
#define N_1     0x31
#define N_2     0x32
#define N_3     0x33
#define N_4     0x34
#define N_5     0x35
#define L_ARROW 0xff51
#define U_ARROW 0xff52
#define R_ARROW 0xff53
#define D_ARROW 0xff54
#define TAB     0xff09
#define K       0x6b

/* BUTTON DEFENITIONS */
#define L_MOUSE_BUTTON XCB_BUTTON_INDEX_1
#define R_MOUSE_BUTTON XCB_BUTTON_INDEX_3

#endif/*DEFENITIONS_HPP*/