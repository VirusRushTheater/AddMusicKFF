#include <memory>
#include "logging.hpp"

using namespace virusrt;

std::map<std::string, std::shared_ptr<Logger> > Logger::_logger_map;

Logger& Logger::getLogger(const std::string& logger_name)
{	
	// We have to register such logger if there isn't any.
	std::shared_ptr<Logger> newlg(new Logger(logger_name));

	if (_logger_map.count(logger_name) == 0)
	{
		_logger_map.insert(std::make_pair(logger_name, newlg));
	}
	
	return *_logger_map[logger_name];
}

void Logger::setLevel(LoggingSeverity lv)
{
	log_level = lv;
}

LoggingSeverity Logger::getLevel() const
{
	return log_level;
}

const char* Logger::getLevelAsStr() const
{
	switch (log_level)
	{
		case LoggingSeverity::LOGGING_NULL:
			return "NULL";
		case LoggingSeverity::LOGGING_DEBUG:
			return "DEBUG";
		case LoggingSeverity::LOGGING_INFO:
			return "INFO";
		case LoggingSeverity::LOGGING_WARNING:
			return "WARN";
		case LoggingSeverity::LOGGING_ERROR:
			return "ERROR";
		case LoggingSeverity::LOGGING_CRITICAL:
			return "CRITICAL";
		default:
			return "?";
	}
}

void Logger::debug(const std::string& msg)
{
	this->__msg <LoggingSeverity::LOGGING_DEBUG>(msg);
}

void Logger::debug(const std::stringstream& msg)
{
	this->__msg <LoggingSeverity::LOGGING_DEBUG>(msg);
}

void Logger::info(const std::string& msg)
{
	this->__msg <LoggingSeverity::LOGGING_INFO>(msg);
}

void Logger::info(const std::stringstream& msg)
{
	this->__msg<LoggingSeverity::LOGGING_INFO>(msg);
}

void Logger::warning(const std::string& msg)
{
	this->__msg<LoggingSeverity::LOGGING_WARNING>(msg);
}

void Logger::warning(const std::stringstream& msg)
{
	this->__msg<LoggingSeverity::LOGGING_WARNING>(msg);
}

void Logger::error(const std::string& msg)
{
	this->__msg<LoggingSeverity::LOGGING_ERROR>(msg);
}

void Logger::error(const std::stringstream& msg)
{
	this->__msg<LoggingSeverity::LOGGING_ERROR>(msg);
}

void Logger::critical(const std::string& msg)
{
	this->__msg<LoggingSeverity::LOGGING_CRITICAL>(msg);
}

void Logger::critical(const std::stringstream& msg)
{
	this->__msg<LoggingSeverity::LOGGING_CRITICAL>(msg);
}
