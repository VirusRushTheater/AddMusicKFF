#pragma once

#include <algorithm>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>
#include <regex>
#include <cstdlib>

#include "defines.h"
#include "AddmusicLogging.h"

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief 24-bit number datatype that mimics a native integer data type. Useful for hex replacements and bounds.
 */
class uint24_t {
public:
    constexpr uint24_t() : value_(0) {}
    constexpr uint24_t(uint32_t value) : value_(value & 0xFFFFFF) {}
    constexpr operator uint32_t() const { return value_; }

	constexpr uint24_t& operator&=(const uint24_t& A) {  return *this = (*this &  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator|=(const uint24_t& A) {  return *this = (*this |  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator^=(const uint24_t& A) {  return *this = (*this ^  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator+=(const uint24_t& A) {  return *this = (*this +  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator-=(const uint24_t& A) {  return *this = (*this -  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator*=(const uint24_t& A) {  return *this = (*this *  A.value_) & 0xffffff;   }
    constexpr uint24_t& operator<<=(const uint24_t& A) { return *this = (*this << A.value_) & 0xffffff;   }
    constexpr uint24_t& operator>>=(const uint24_t& A) { return *this = (*this >> A.value_) & 0xffffff;   }
    constexpr uint24_t& operator/=(const uint24_t& A) {  return *this = *this / A.value_; }
    constexpr uint24_t& operator%=(const uint24_t& A) {  return *this = *this % A.value_; }
private:
    uint32_t value_;
};

/**
 * @brief Syntactic sugar that allows us to define number literals of a certain width.
 */
constexpr uint8_t operator"" _u8(unsigned long long int value) {
    return static_cast<uint8_t>(value);
}

constexpr uint16_t operator"" _u16(unsigned long long int value) {
    return static_cast<uint16_t>(value);
}

constexpr uint24_t operator"" _u24(unsigned long long int value) {
    return static_cast<uint24_t>(value);
}

constexpr uint32_t operator"" _u32(unsigned long long int value) {
    return static_cast<uint32_t>(value);
}

/**
 * @brief Reads a binary file and stores its contents in a vector.
 */
template <typename T>
inline void readBinaryFile(const fs::path &fileName, std::vector<T> &v)
{
	std::ifstream is (fileName, std::ios::binary);
	if (!is)
		throw fs::filesystem_error("File cannot be read.", fileName, std::make_error_code(std::errc::no_such_file_or_directory));
	const size_t file_size {fs::file_size(fileName)};
	v.resize(file_size / sizeof(T));
	is.read(reinterpret_cast<char*>(v.data()), file_size);
	is.close();
}

/**
 * @brief Reads a text file and stores its contents in a string.
 */
inline void readTextFile(const fs::path &fileName, std::string &str)
{
	std::ifstream is (fileName);
	if (!is)
		throw fs::filesystem_error("File cannot be read.", fileName, std::make_error_code(std::errc::no_such_file_or_directory));
	const size_t file_size {fs::file_size(fileName)};
	str.resize(file_size);
	is.read(str.data(), file_size);
	is.close();
}

/**
 * @brief Writes the contents of a vector in a binary file. 
 */
template <typename T>
inline void writeBinaryFile(const fs::path &fileName, std::vector<T> &v)
{
	std::ofstream ofs (fileName, std::ios::binary);
	if (!ofs)
		throw fs::filesystem_error("File cannot be written.", fileName, std::make_error_code(std::errc::no_such_file_or_directory));

	const char* data = reinterpret_cast<const char*>(v.data());
    size_t size = v.size() * sizeof(T);
	std::copy(data, data + size, std::ostream_iterator<char>(ofs));
	ofs.close();
}

/**
 * @brief Writes a string into a text file. 
 */
inline void writeTextFile(const fs::path &fileName, const std::string &str)
{
	std::ofstream ofs (fileName);
	if (!ofs)
		throw fs::filesystem_error("File cannot be written.", fileName, std::make_error_code(std::errc::no_such_file_or_directory));
	std::copy(str.begin(), str.end(), std::ostream_iterator<char>(ofs));
	ofs.close();
}

/**
 * @brief Search a $XXXX (hexadecimal) formatted number after a "needle" string.
 * The needle can also be a regular expression pattern.
 */
inline unsigned int scanInt(const std::string &str, const std::string &tag)
{
	std::string needle_concat = tag + R"(\s*\$([A-Fa-f0-9]+))";
	std::regex pattern (needle_concat);
	std::smatch matches;

	// We could use a not-null end_ptr on strtoul but it would only be useful on
	// undefined behavior.
	if (std::regex_search(str, matches, pattern))
		return std::strtoul(matches[1].str().c_str(), NULL, 16);
	else
		throw std::runtime_error(std::string("Error: Could not find \"") + tag + "\" inside your string.");
}

/**
 * @brief Some way to dump some binary number into hexadecimal.
 * Basically the former hex2, hex4 and hex6 macros, but relying on datatypes
 * instead of explicit width of those numbers.
 */
template<typename T, size_t t_size = sizeof(T) * 2>
inline std::string hexDump(T value)
{
	constexpr size_t MAX_LEN = 16;
	static_assert(t_size > 0 && t_size <= MAX_LEN, "Invalid size");
	return (std::stringstream() << std::hex << std::uppercase << std::setfill('0') << std::setw(t_size) << (uint32_t)value).str();
}

/**
 * @brief Specialization for 24-bit numbers.
 */
template<>
inline std::string hexDump<uint24_t>(uint24_t value)
{
    return (std::stringstream() << std::hex << std::uppercase << std::setfill('0') << std::setw(6) << (value & 0xffffff)).str();
}

/**
 * @brief Other way to dump some binary number into hexadecimal.
 * Basically the former hex2, hex4 and hex6 macros, but templated.
 * Put here besides hexDump in order to check which is more comfortable to use.
 */
template<size_t len>
inline std::string hex(unsigned long long value)
{
	return (std::stringstream() << std::hex << std::uppercase << std::setfill('0') << std::setw(len) << value).str();
}

/**
 * @brief Replaces the value of a hexadecimal value located after a certain
 * tag to be found inside a string.
 * The zero-padding will depend on the data type of value. Cast to uint##_t accordingly.
 */
template<typename T>
inline void replaceHexValue(T value, const std::string &tag, std::string &str)
{
	std::string needle_concat = tag + R"(\s*[\$#]*([A-Fa-f0-9]+))";
	std::regex pattern (needle_concat);
	std::smatch matches;

	if (std::regex_search(str, matches, pattern))
		str.replace(matches[1].first, matches[1].second, hexDump(value));
	else
		throw std::runtime_error(std::string("Error: Could not find \"") + tag + "\" inside your string.");
}

/**
 * @brief Recursively copies a folder and its contents. Will overwrite files
 * and directory contents, but won't delete any new files in dst.
 */
void copyDir(const fs::path& src, const fs::path& dst);

/**
 * @brief Recursively deletes a folder and its contents. 
 */
void deleteDir(const fs::path& dir_path);

}