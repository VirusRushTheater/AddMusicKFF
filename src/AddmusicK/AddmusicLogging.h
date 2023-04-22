#pragma once

#include <exception>
#include <stdexcept>
#include <sstream>
#include <string>
#include <filesystem>
#include <iostream>

namespace AddMusic {
class MMLBase;

/**
 * @brief Exception throwable by a component of the AddMusic program or library,
 * including file name and line, if necessary.
 * 
 * The intent is to emulate the warning, error and printError macros with
 * catchable exceptions.
 */
class AddmusicException : public std::exception
{
	public:
	/**
	 * Default constructor.
	 */
	
	AddmusicException(
		const std::string& message,
		MMLBase* mmlfile_ref = nullptr
	) :
		_errormsg(message),
		mmlref(mmlfile_ref)
	{
		error_count++;
		// if (mmlfile_ref != nullptr)
		// 	_locref = std::string(" [@") + mmlfile_ref->filename.filename().string() + " L" + std::to_string(mmlfile_ref->line) + "]";
		_completemsg = (_errormsg + _locref);
		std::exception();
	}

	/**
	 * @brief Gets the full message, with parser info if it applies.
	 */
	const std::string& fullMessage() const
	{
		return _completemsg;
	}

	/**
	 * @brief Gets the error message, without parser info.
	 */
	const std::string& message() const
	{
		return _errormsg;
	}

	/**
	 * @brief Returns the amount of errors thrown by the different modules
	 * of this program. 
	 */
	unsigned int errorCount() const
	{
		return error_count;
	}

	/**
	 * @brief Message that will be thrown to the console if the exception is
	 * not caught and crashes the program.
	 */
	const char *what() const noexcept
	{
		return _completemsg.c_str();
	}

	MMLBase* 	mmlref;				// Reference to a MML parsing file to report line and file name, if it applies.
	
	private:
	inline static unsigned int error_count {0};	// Error count. Reimplementation from older version.
	std::string _completemsg;			// Complete message, with file details.
	std::string	_errormsg;				// Raw error message
	std::string _locref 	{""};		// Error message formatted with parser info
};

/**
 * Facility to print messages to the stderr console, so it does not interfere with any
 * other useful information this program might throw through stdout.
 * 
 * If you don't like how the message is formatted, mess with the _printmessage
 * method.
 */
class Logging
{
public:
	enum Levels
	{
		LEVEL_LOWEST,
		DEBUG,
		INFO,
		WARNING,
		ERROR,
		CRITICAL,
		LEVEL_HIGHEST
	};

    static Logging& getInstance() {
        static Logging instance; // Guaranteed to be destroyed.
                                    // Instantiated on first use.
        return instance;
    }

    // Delete copy constructor and assignment operator
    Logging(const Logging&) = delete;
    Logging& operator=(const Logging&) = delete;

	/**
	 * Sets which logging info gets printed. By default it's Levels::INFO, which
	 * ignores the "debug" messages.
	 */
	static void setVerbosity(Levels verbosity_level)
	{
		getInstance()._verbosity_level = verbosity_level;
	}

	/**
	 * Mute the logger completely. Will still throw exceptions when an error
	 * happens.
	 */
	static void mute()
	{
		getInstance()._verbosity_level = Levels::LEVEL_HIGHEST;
	}

	// Different messaging levels. Designed for parser outputs to be referenced
	// only by passing the "this" keyword, and the logger will print the file
	// name and line.
	static void debug(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::DEBUG); }
	static void verbose(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::DEBUG); }
	static void info(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::INFO); }
	static void warning(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::WARNING); }
	static void error(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::ERROR); }
	static void critical(const std::string& msg, MMLBase* mmlfile_ref = nullptr) { _printmessage(msg, mmlfile_ref, Levels::CRITICAL); }

	// Messaging using stringstreams if you love the << operators.
	static void debug(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::DEBUG); }
	static void verbose(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::DEBUG); }
	static void info(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::INFO); }
	static void warning(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::WARNING); }
	static void error(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::ERROR); }
	static void critical(const std::stringstream& msgstream, MMLBase* mmlfile_ref = nullptr) { _printmessage(msgstream.str(), mmlfile_ref, Levels::CRITICAL); }

private:
    Logging() {} // Private constructor

	bool _show_level = true;
	Levels _exception_level = Levels::ERROR;	// Minimum error level that will throw an exception.
	Levels _verbosity_level = Levels::INFO;		// Minimum error level that will be printed.

	/**
	 * Internal method that formats and prints the message or throw exceptions,
	 * if these apply.
	 */
	static void _printmessage(const std::string& msg, MMLBase* mmlfile_ref, Levels lv)
	{
		std::string level_str;
		Levels _exc_level = getInstance()._exception_level;
		Levels _vb_level = getInstance()._verbosity_level;
		
		// Ignore this method completely if the levels are not high enough.
		if (((int)lv < (int)_vb_level) && ((int)lv < (int)_exc_level))
			return;
		
		switch(lv)
		{
			case Levels::DEBUG: 	level_str = ""; 		break;
			case Levels::INFO: 		level_str = ""; 		break;
			case Levels::WARNING: 	level_str = "[WW] "; 	break;
			case Levels::ERROR: 	level_str = "[EE] "; 		break;
			case Levels::CRITICAL: 	level_str = "[CC] "; 	break;
		}

		if (mmlfile_ref)
		{
			// TODO: References to file name (mmlfile_ref->filename.filename().string()) and line (std::to_string(mmlfile_ref->line))
		}
		else
		{
			// Did not reference a file. Proceed as always.
		}

		// Print the message if it applies.
		if (((int)lv >= (int)_vb_level) && ((int)lv < (int)_exc_level))
			std::cerr << level_str << msg << std::endl;
		
		// Throw an exception if it applies.
		if ((int)lv >= (int)_exc_level)
			throw AddmusicException(msg);
	}
};
}