#pragma once

#include <exception>
#include <stdexcept>
#include <sstream>
#include <string>
#include <filesystem>

// #include "MMLBase.h"

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
	 * Constructor with just an error message and fatality.
	 */
	
	AddmusicException(
		const std::string& message,
		bool is_fatal = true,
		MMLBase* mmlfile_ref = nullptr
	) :
		_errormsg(message),
		_is_fatal(is_fatal),
		_mmlref(mmlfile_ref)
	{
		error_count++;
		// if (mmlfile_ref != nullptr)
		// 	_locref = std::string(" [@") + mmlfile_ref->filename.filename().string() + " L" + std::to_string(mmlfile_ref->line) + "]";
		_completemsg = (_errormsg + _locref);
		std::exception();
	}

	/**
	 * Ignoring fatality, it's considered true.
	 */
	AddmusicException(
		const std::string& message,
		MMLBase* mmlfile_ref
	) :
		AddmusicException(message, true, mmlfile_ref)
	{}

	/**
	 * @brief Returns the fatality of this error.
	 */
	bool isFatal() const
	{
		return _is_fatal;
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

	private:
	inline static unsigned int error_count {0};	// Error count. Reimplementation from older version.
	MMLBase* 	_mmlref;				// Reference to a MML parsing file to report line and file name, if corresponds.
	std::string _completemsg;			// Complete message, with file details.
	std::string	_errormsg;				// Raw error message
	std::string _locref 	{""};		// Error message formatted with parser info
	bool 		_is_fatal 	{true};		// Is fatal? Use this to manage how to manage caught exceptions.
};
}