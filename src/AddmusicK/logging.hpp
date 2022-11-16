#pragma once
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <iostream>

namespace virusrt
{

enum class LoggingSeverity
{
	LOGGING_NULL,
	LOGGING_DEBUG,
	LOGGING_INFO,
	LOGGING_WARNING,
	LOGGING_ERROR,
	LOGGING_CRITICAL	
};

/**
 * @brief Logging class implemented as a named-singleton, to emulate Python's
 * native logging capabilities.
 * 
 * Usage: DON'T USE CONSTRUCTOR. You get a logger instance with a global
 * namespace by retrieving a Logger reference 
 */

// void (*msgCallback)(const std::string&)

class Logger
{
	public:
	Logger() = delete;
	static std::map < std::string, std::shared_ptr<Logger> > _logger_map;

	/**
	 * @brief Gets a logger from the namespace. Creates one if it does not
	 * exist. Returns a reference to said logger.
	 * 
	 * @param logger_name	Logger name 
	 * @return Logger& 		Logger reference
	 */
	static Logger& getLogger(const std::string& logger_name);

	void setLevel(LoggingSeverity lv);
	LoggingSeverity getLevel() const;
	const char* getLevelAsStr() const;

	template <LoggingSeverity SEVERITY>
	void __msg(const std::string& msg)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << msg << std::endl;
	}

	template <LoggingSeverity SEVERITY>
	void __msg(const std::stringstream& msg)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << msg.str() << std::endl;
	}

	void debug(const std::string& msg);
	void debug(const std::stringstream& msg);
	void info(const std::string& msg);
	void info(const std::stringstream& msg);
	void warning(const std::string& msg);
	void warning(const std::stringstream& msg);
	void error(const std::string& msg);
	void error(const std::stringstream& msg);
	void critical(const std::string& msg);
	void critical(const std::stringstream& msg);

	private:
	Logger(std::string name) :
		_name {name} {}
	
	std::string _name;
	std::string _formatter;
	LoggingSeverity log_level {LoggingSeverity::LOGGING_WARNING};
};

}
