/*
 * plogger.h
 *
 */

#ifndef PLOGGER_H_
#define PLOGGER_H_

#include <string>

enum log_level
{
	DEBUG,
	VERBOSE,
	WARNING,
	ERROR
};

class logger_t
{
public:
	logger_t()
	: m_enabled{false,false,true,true}
	{
	}

	virtual ~logger_t() = default;

	void log(log_level level, const char *s)
	{
		if (m_enabled[level])
			vlog(level_str(level) + ": " + s);
	}

	void log(log_level level, std::string s)
	{
		if (m_enabled[level])
			vlog(level_str(level) + ": " + s);
	}

	template<typename... Args>
	void log(log_level level, const char *fmt, Args&&... args)
	{
		if (m_enabled[level])
		{
			char buf[1024];
			std::snprintf(buf, 1024, fmt, std::forward<Args>(args)...);
			vlog(level_str(level) + ": " + buf);
		}
	}

	void set_level(log_level level, bool val) { m_enabled[level] = val; }

protected:
	virtual void vlog(std::string msg) = 0;

	std::string level_str(log_level level)
	{
		switch (level)
		{
			case DEBUG:
				return "DEBUG";
			case VERBOSE:
				return "VERBOSE";
			case WARNING:
				return "WARNING";
			case ERROR:
				return "ERROR";
		}
		return ""; // please compiler
	}

	bool m_enabled[4];
};

class std_logger_t : public logger_t
{
public:
	std_logger_t()
	{
	}

protected:
	void vlog(std::string msg) override
	{
		printf("%s\n", msg.c_str());
	}
};




#endif /* PLOGGER_H_ */
