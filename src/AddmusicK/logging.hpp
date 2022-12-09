#pragma once
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace virusrt
{

namespace fs = std::filesystem;

enum class LoggingLevel
{
	ZERO,
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	CRITICAL,
	NONE
};

/*
class SinkBase
{
	public:
	SinkBase(LoggingLevel lv) :
		_logginglevel{lv}
	{}
	virtual void __dump(const std::string& msg) = 0;

	private:
	LoggingLevel _logginglevel;
};

class SinkStderr : public SinkBase
{
	using SinkBase::SinkBase;
	void __dump(const std::string& msg) override
	{
		std::cerr << msg << std::endl;
	}
};

class SinkFile : public SinkBase
{
	SinkFile(LoggingLevel lv, fs::path fpath) :
		SinkBase(lv)
	{
		_file.open(fpath, std::ios::app);
	}
	~SinkFile()
	{
		_file.close();
	}

	void __dump(const std::string& msg) override
	{
		_file << msg << std::endl;
	}

	private:
	std::ofstream _file;
};
*/

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
	inline static std::map < std::string, std::shared_ptr<Logger> > _logger_map;

	/**
	 * @brief Gets a logger from the namespace. Creates one if it does not
	 * exist. Returns a reference to said logger.
	 * 
	 * @param logger_name	Logger name 
	 * @return Logger& 		Logger reference
	 */
	static Logger& getLogger(const std::string& logger_name)
	{
		// We have to register such logger if there isn't any.
		std::shared_ptr<Logger> newlg(new Logger(logger_name));

		if (_logger_map.count(logger_name) == 0)
		{
			_logger_map.insert(std::make_pair(logger_name, newlg));
		}
		
		return *_logger_map[logger_name];
	}

	/**
	 * @brief Sets the minimum level that will be reported on the console.
	 * 
	 * @param lv 
	 */
	void setLevel(LoggingLevel lv)
	{
		log_level = lv;
	}

	/**
	 * @brief Sets the minimum level which will be thrown as an exception.
	 * 
	 * @param lv 
	 */
	void setThrowLevel(LoggingLevel lv)
	{
		exception_level = lv;
	}

	/**
	 * @brief Returns the minimum level that will be reported on the console.
	 * 
	 * @return LoggingLevel 
	 */
	LoggingLevel getLevel() const
	{
		return log_level;
	}

	/**
	 * @brief Returns the minimum level that will be thrown as an exception.
	 * 
	 * @return LoggingLevel 
	 */
	LoggingLevel getThrowLevel() const
	{
		return log_level;
	}

	inline void __msg(LoggingLevel SEVERITY, const std::string& msg)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << msg << std::endl;
		if (exception_level <= SEVERITY)
			throw std::runtime_error(msg);
	}

	inline void __msg(LoggingLevel SEVERITY, const char* msg)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << msg << std::endl;
		if (exception_level <= SEVERITY)
			throw std::runtime_error(msg);
	}

	inline void __msg(LoggingLevel SEVERITY, const std::stringstream& msg)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << msg.str() << std::endl;
		if (exception_level <= SEVERITY)
			throw std::runtime_error(msg.str());
	}

	template <typename T>
	inline void __msg(LoggingLevel SEVERITY, const T& exc)
	{
		if (log_level <= SEVERITY)
			std::cerr << "[" << _name << "] " << exc.what() << std::endl;
		if (exception_level <= SEVERITY)
			throw exc;
	}

	#define __LEVEL(LNAMEUPPER,LNAMELOWER) \
	inline void LNAMELOWER(const char* msg) {__msg(LoggingLevel::LNAMEUPPER, msg);} \
	inline void LNAMELOWER(const std::string& msg) {__msg(LoggingLevel::LNAMEUPPER, msg);} \
	inline void LNAMELOWER(const std::stringstream& msg) {__msg(LoggingLevel::LNAMEUPPER, msg);} \
	template <typename T> inline void LNAMELOWER(const T& exc) {__msg<T>(LoggingLevel::LNAMEUPPER, exc);}

	__LEVEL(DEBUG, debug);
	__LEVEL(INFO, info);
	__LEVEL(WARNING, warning);
	__LEVEL(ERROR, error);
	__LEVEL(CRITICAL, critical);

	#undef __LEVEL

	private:
	Logger(std::string name) :
		_name {name} {}
	
	std::string _name;
	std::string _formatter;
	LoggingLevel log_level {LoggingLevel::INFO};
	LoggingLevel exception_level {LoggingLevel::ERROR};
};

}
