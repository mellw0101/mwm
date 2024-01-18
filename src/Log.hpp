/*** dwmLog.hpp ***/
#ifndef DWMLOG_HPP
#define DWMLOG_HPP

#include <cstdint>
#include <string>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <iostream>
#include <string>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <X11/X.h>
#include <vector>
#include <xcb/xproto.h>

class FileHandler 
{
	public:
		explicit FileHandler(std::string filename) : m_filename(std::move(filename)) {}
		
		bool
		append(const std::string &text) 
		{
			std::ofstream file(m_filename, std::ios::app);// Open in append mode

			if (!file.is_open()) 
			{
				return false;// Return false if file couldn't be opened
			}
			file << text;// Append text
			file.close ();
			return true;
		}

		bool
		open() 
		{
			std::ofstream file(m_filename, std::ios::app);// Open in append mode
			bool isOpen = file.is_open();
			file.close();
			return isOpen;
		}
	;

	private:
		std::string m_filename;
	;
};

class TIME 
{
	public:
		static const std::string get() 
		{
			// Get the current time
			const auto & now = std::chrono::system_clock::now();
			const auto & in_time_t = std::chrono::system_clock::to_time_t(now);

			// Convert time_t to tm as local time
			std::tm buf{};
			localtime_r(&in_time_t, &buf);

			// Use stringstream to format the time
			std::ostringstream ss;
			ss << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "]";
			return ss.str();
		}
	;
};

class Converter 
{
	public:
		static const char * 
		xcb_event_response_type_to_str(const uint8_t &response_type) 
		{
			switch (response_type) 
			{
				case XCB_KEY_PRESS:             return "KeyPress";
				case XCB_KEY_RELEASE:           return "KeyRelease";
				case XCB_BUTTON_PRESS:          return "ButtonPress";
				case XCB_BUTTON_RELEASE:        return "ButtonRelease";
				case XCB_MOTION_NOTIFY:         return "MotionNotify";
				case XCB_ENTER_NOTIFY:          return "EnterNotify";
				case XCB_LEAVE_NOTIFY:          return "LeaveNotify";
				case XCB_FOCUS_IN:              return "FocusIn";
				case XCB_FOCUS_OUT:             return "FocusOut";
				case XCB_KEYMAP_NOTIFY:         return "KeymapNotify";
				case XCB_EXPOSE:                return "Expose";
				case XCB_GRAPHICS_EXPOSURE:     return "GraphicsExpose";
				case XCB_NO_EXPOSURE:           return "NoExpose";
				case XCB_VISIBILITY_NOTIFY:     return "VisibilityNotify";
				case XCB_CREATE_NOTIFY:         return "CreateNotify";
				case XCB_DESTROY_NOTIFY:        return "DestroyNotify";
				case XCB_UNMAP_NOTIFY:          return "UnmapNotify";
				case XCB_MAP_NOTIFY:            return "MapNotify";
				case XCB_MAP_REQUEST:           return "MapRequest";
				case XCB_REPARENT_NOTIFY:       return "ReparentNotify";
				case XCB_CONFIGURE_NOTIFY:      return "ConfigureNotify";
				case XCB_CONFIGURE_REQUEST:     return "ConfigureRequest";
				case XCB_GRAVITY_NOTIFY:        return "GravityNotify";
				case XCB_RESIZE_REQUEST:        return "ResizeRequest";
				case XCB_CIRCULATE_NOTIFY:      return "CirculateNotify";
				case XCB_CIRCULATE_REQUEST:     return "CirculateRequest";
				case XCB_PROPERTY_NOTIFY:       return "PropertyNotify";
				case XCB_SELECTION_CLEAR:       return "SelectionClear";
				case XCB_SELECTION_REQUEST:     return "SelectionRequest";
				case XCB_SELECTION_NOTIFY:      return "SelectionNotify";
				case XCB_COLORMAP_NOTIFY:       return "ColormapNotify";
				case XCB_CLIENT_MESSAGE:        return "ClientMessage";
				case XCB_MAPPING_NOTIFY:        return "MappingNotify";			
				case XCB_GE_GENERIC:			return "GeneriqEvent";						            
				default:                        return "Unknown";
			}    
		}
		
		static const char *
		xcb_event_detail_to_str(const uint8_t &detail) 
		{
			switch (detail) 
			{
				case XCB_NOTIFY_DETAIL_ANCESTOR: 			return "Ancestor";
				case XCB_NOTIFY_DETAIL_VIRTUAL: 			return "Virtual";
				case XCB_NOTIFY_DETAIL_INFERIOR: 			return "Inferior";
				case XCB_NOTIFY_DETAIL_NONLINEAR: 			return "Nonlinear";
				case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL: 	return "NonlinearVirtual";
				case XCB_NOTIFY_DETAIL_POINTER: 			return "Pointer";
				case XCB_NOTIFY_DETAIL_POINTER_ROOT: 		return "PointerRoot";
				case XCB_NOTIFY_DETAIL_NONE: 				return "None";
				default: 									return "Unknown";
			}
		}

		static const char *
		xcb_event_mode_to_str(const uint8_t &mode)
		{
			switch (mode) 
			{
				case XCB_NOTIFY_MODE_NORMAL: 		return "Normal";
				case XCB_NOTIFY_MODE_GRAB: 			return "Grab";
				case XCB_NOTIFY_MODE_UNGRAB: 		return "Ungrab";
				case XCB_NOTIFY_MODE_WHILE_GRABBED: return "WhileGrabbed";
				default: 							return "Unknown";
			}
		}
	;
};

class Log 
{
	public:
		static void
		xcb_event_response_type(const char* FUNC, const uint8_t &response_type) 
		{
			const char* ev = Converter::xcb_event_response_type_to_str(response_type);
			logMessage 
			( 
				toString
				(
					":[xcb_event_response_type]:[", FUNC,"]:[",ev,"]"
				)
			);
		}

		static void
		xcb_event_detail(const char* FUNC, const uint8_t &detail) 
		{
			const char* ev = Converter::xcb_event_detail_to_str(detail);
			logMessage 
			( 
				toString
				(
					":[xcb_event_detail]:[", FUNC, "]:[", ev, "]"
				)
			);
		}
		
		static void
		FUNC(const std::string &message) 
		{
			logMessage(":[" + message + "]");
		}
		
		static void
		Start(const std::string &message) 
		{
			StartMessage( ":[Start]  :["+message+"]:[STARTED]" );
		}
		
		static void
		End() 
		{
			EndMessage( ":[End]    :[dwm-M]:[KILLED]\n" );
		}
		
		static void
		ENDFUNC(const std::string &message)
		{
			logMessage ( ":[ENDFUNC]:[" + message + "]" + "\n");
		}

		static void
		INFO(const std::string &message) 
		{
			logMessage ( ":[INFO]:[" + message + "]" );
		}

		template<typename T, typename... Args>
		static void
		INFO(const T &message, Args... args) 
		{
			logMessage ( ":[INFO]   :[" + toString ( message, args... ) + "]" );
		}

		template<typename T, typename... Args>
		static void
		FUNC_INFO(const T &message, Args... args) 
		{
			logMessage ( ":----[INFO]:[" + toString ( message, args... ) + "]" );
		}

		static void
		WARNING(const std::string &message) 
		{
			logMessage ( ":[WARNING]:[" + message + "]" );
		}

		template<typename T, typename... Args>
		static void
		WARNING(const T &first, Args... args) 
		{
			logMessage ( ":[WARNING]:[" + toString ( first, args... ) + "]" );
		}

		static void
		ERROR(const std::string &message) 
		{
			logMessage ( ":[ERROR]  :[" + message + "]" );
		}

		template<typename T, typename... Args>
		static void
		ERROR(const T &message, Args... args) 
		{
			logMessage ( ":[ERROR]  :[" + toString ( message, args... ) + "]" );
		}
	;

	private:
		// Static function for conversion
		template<typename T>
		static std::string
		toString(const T &arg) 
		{
			std::ostringstream stream;
			stream << arg;
			return stream.str();
		}

		template<typename T, typename... Args>
		static std::string
		toString(const T &message, Args... args) 
		{
			return toString ( message ) + toString ( args... );
		}

		static void
		logMessage(const std::string &message) 
		{
			FileHandler file ( "/home/mellw/nlog" );

			if (file.open())
			{
				file.append(TIME::get() + message + "\n");
			}
		}

		static void
		StartMessage(const std::string &message) 
		{
			FileHandler file ("/home/mellw/nlog");

			if (file.open()) 
			{
				file.append("\n" + TIME::get() + message + "\n");
			}
		}

		static void
		EndMessage(const std::string &message) 
		{
			FileHandler file("/home/mellw/nlog");

			if (file.open()) 
			{
				file.append(TIME::get() + message + "\n");
			}
		}
	;
};

class toString 
{
	public:
		template<typename T>
		toString(const T &input)
		{
			result = convert(input);
		}

		operator std::string() const 
		{
			return result;
		}
	;

	private:
		std::string result;

		std::string 
		convert(const int16_t &in)
		{
			std::string str;
			str = std::to_string(in);
			return str;
		}

		std::string
		convert(const std::string &in)
		{
			return in;
		}

		std::string
		convert(const std::vector<uint32_t> &in)
		{
			std::string str;
			for (auto &i : in) 
			{
				str += std::to_string(i) + " ";
			}
			return str;
		}

		std::string
		convert(const std::vector<const char *> &in)
		{
			std::string str;
			for (auto &i : in) 
			{
				str += std::string(i) + " ";
			}
			return str;
		}

		std::string
		convert(const std::vector<std::string> &in)
		{
			std::string str;
			for (auto &i : in) 
			{
				str += i + " ";
			}
			return str;
		}

		std::string
		convert(const std::vector<xcb_event_mask_t> &in)
		{
			std::string str;
			for (auto &i : in) 
			{
				str += std::to_string(i) + " ";
			}
			return str;
		}
	;
};

// ANSI escape codes for colors
#define log_RED 		"\033[1;31m" 
#define log_GREEN 		"\033[1;32m" 
#define log_YELLOW 		"\033[1;33m"
#define log_BLUE 		"\033[1;34m"
#define log_MEGENTA 	"\033[1;35m"
#define log_CYAN 		"\033[1;36m"
#define log_WHITE 		"\033[1;37m"
#define log_BOLD 		"\033[1m"
#define log_UNDERLINE 	"\033[4m"
#define log_RESET 		"\033[0m"


enum LogLevel 
{
    INFO,
	INFO_PRIORITY,
    WARNING,
    ERROR,
	FUNC
};

class Logger 
{
	public:
		template<typename T, typename... Args>
		void 
		log(LogLevel level, const std::string &function, const T& message, Args&&... args) 
		{
			FileHandler file("/home/mellw/nlog");
			log_arragnge(list, message, args...);
			file.append(TIME::get() + ":" + getLogPrefix(level) + ":" + log_MEGENTA + "[" + function + "]" + log_RESET + ":" + str + "\n");
		}

		void 
		log(LogLevel level, const std::string &function) 
		{
			FileHandler file("/home/mellw/nlog");
			file.append(TIME::get() + ":" + getLogPrefix(level) + ":[" + log_MEGENTA + function + log_RESET + "]\n");
		}
	;

	private:
		std::vector<std::string> list;
		std::string str;

		std::string 
		getLogPrefix(LogLevel level) const 
		{
			switch (level) 
			{
				case INFO:
				{
					return log_GREEN 	"[INFO]" 		  log_RESET;
				}
				case INFO_PRIORITY:
				{
					return log_CYAN		"[INFO_PRIORITY]" log_RESET;
				}
				case WARNING:
				{
					return log_YELLOW 	"[WARNING]" 	  log_RESET;
				}
				case ERROR:
				{
					return log_RED 		"[ERROR]" 		  log_RESET;
				}
				case FUNC:
				{
					return log_MEGENTA	"[FUNC]"		  log_RESET;
				}
				default:
				{
					return std::to_string(level);
				}
			}
		}

		template<typename T, typename... Args>
		void
		log_arragnge(std::vector<std::string> list, const T &message, const Args&... args)
		{
			list.push_back(toString(message));
			if constexpr (sizeof...(args) > 0)
			{
				log_arragnge(list, args...);
			}
			else
			{
				log_append(list);
			}
		}

		void
		log_append(std::vector<std::string> list)
		{
			std::string result;
			int current = 0;
			for (auto &i : list)
			{
				result += "[" + i + "]";
				if (current != list.size() - 1) 
				{
					result += ":";
				}
				current++;
			}
			str = result;
		}
	;
};

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
#define log_error_long(message, message_2)                  log.log(ERROR, __FUNCTION__, message, message_2)
#define log_error_code(message, err_code)   log.log(ERROR, __FUNCTION__, message, err_code)
#define log_win(win_name ,window)           log.log(INFO, __func__, win_name + std::to_string(window))
#define log_func                            log.log(FUNC, __func__)

#endif // DWMLOG_HPP
