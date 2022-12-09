#pragma once

#include <exception>
#include <stdexcept>
#include <sstream>
#include <string>
#include <filesystem>

namespace AddMusic {

/**
 * @brief Type of errors that can be thrown by AddmusicException.
 */
enum class AddmusicErrorcode
{
	SUCCESS = 0,				// No problem
	UNKNOWN,					// Unknown error

	PARSING_ERROR,				// Error when parsing
	PARSEASM_ERROR,				// Error when parsing ASM
	COMPILEASM_ERROR,			// Error when compiling ASM
	PREPROCESSING_ERROR,		// Error when preprocessing
	GETQUOTED_ERROR,			// Error on the getQuotedString function
};

/**
 * @brief Handmade method to turn an AddmusicException to a string.
 */
constexpr const char* addmusicErrorcodeToString(AddmusicErrorcode e) throw ()
{
    switch (e)
    {
        case AddmusicErrorcode::SUCCESS: 				return "SUCCESS";
        case AddmusicErrorcode::UNKNOWN: 				return "UNKNOWN";

        case AddmusicErrorcode::PARSING_ERROR: 			return "PARSING_ERROR";
        case AddmusicErrorcode::PARSEASM_ERROR: 		return "PARSEASM_ERROR";
        case AddmusicErrorcode::COMPILEASM_ERROR: 		return "COMPILEASM_ERROR";
        case AddmusicErrorcode::PREPROCESSING_ERROR: 	return "PREPROCESSING_ERROR";
        case AddmusicErrorcode::GETQUOTED_ERROR: 		return "GETQUOTED_ERROR";

        default: throw std::invalid_argument("Unimplemented item");
    }
}

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
	 * @brief Constructs a new Addmusic Exception.
	 * 
	 * It consists of a message detailing the nature of the error, a code for
	 * better catching, and a "is_fatal" boolean for the user to decide what
	 * to do with it.
	 */
	AddmusicException(
		const std::string& message,
		AddmusicErrorcode code = AddmusicErrorcode::UNKNOWN,
		bool is_fatal = true,
		const std::filesystem::path& filename = "",
		unsigned int line = 0
	) : _msg(message),
		_code(code),
		_fullmsg(message),
		_path(filename),
		_line(line),
		_is_fatal(is_fatal)
	{
		error_count++;
		if (filename != "")
			_fullmsg = std::string("[@") + filename.filename().string() + ", L" + std::to_string(line) + "] " + _fullmsg;
			
		std::exception();
	}

	/**
	 * @brief Overloaded constructor, allows a stringstream instead of a string.
	 */
	AddmusicException(
		std::stringstream message,
		AddmusicErrorcode code = AddmusicErrorcode::UNKNOWN,
		bool is_fatal = true,
		const std::filesystem::path& filename = "",
		unsigned int line = 0
	) : AddmusicException(message.str(), code, is_fatal, filename, line)
	{
	}

	/**
	 * @brief Gets the fully formatted message.
	 */
	const std::string getMessage() const
	{
		return std::string {_msg};
	}

	/**
	 * @brief Gets the error code of this exception.
	 */
	AddmusicErrorcode getCode() const
	{
		return _code;
	}

	/**
	 * @brief Gets the amount of non-lethal errors reported so far.
	 */
	unsigned int getErrorCount() const
	{
		return error_count;
	}

	/**
	 * @brief Gets the name of the file related to this error.
	 */
	std::filesystem::path getFilename() const
	{
		return _path;
	}

	/**
	 * @brief Gets the line number of the file that produced this error.
	 */
	unsigned int getLineNumber() const
	{
		return _line;
	}

	/**
	 * @brief Returns the fatality of this error.
	 */
	bool getFatal() const
	{
		return _is_fatal;
	}

	/**
	 * @brief Message that will be thrown to the console if the exception is
	 * not caught and crashes the program.
	 */
	const char *what() const noexcept
	{
		std::stringstream rtext;
		rtext << "[" << addmusicErrorcodeToString(_code) << "] " << _fullmsg << std::endl;
		return rtext.str().c_str();
	}

	private:
	inline static unsigned int error_count {0};

	std::filesystem::path _path;
	bool _is_fatal;
	unsigned int _line;
	std::string _fullmsg;
	std::string _msg;
	AddmusicErrorcode _code;
};
}