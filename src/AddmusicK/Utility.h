#pragma once

#include <vector>
#include <string>
#include <filesystem>

#define hex2 std::setw(2) << std::setfill('0') << std::uppercase << std::hex
#define hex4 std::setw(4) << std::setfill('0') << std::uppercase << std::hex
#define hex6 std::setw(6) << std::setfill('0') << std::uppercase << std::hex

namespace AddMusic
{

/**
 * @brief Opens a file and fills a vector with its contents.
 * 
 * @param fileName 	File name
 * @param v 		Vector to be filled
 */
void openFile(const std::filesystem::path &fileName, std::vector<uint8_t> &v);

/**
 * @brief Opens a file and fills a string with its contents.
 * 
 * @param fileName 	File name
 * @param s 		String to be filled
 */
void openTextFile(const std::filesystem::path &fileName, std::string &s);

/**
 * @brief Get a file's time stamp.
 * 
 * @param file 		File name
 * 
 * @return time_t 	Timestamp of the file.
 */
time_t getTimeStamp(const File &file);

int execute(const std::filesystem::path &command, bool prepend);

/**
 * @brief Scans an integer value that comes after the specified string within
 * another string.  Must be in $XXXX format (or $XXXXXX, etc.).
 * 
 * @param str 		String to be scanned
 * @param value 	
 * @return int 
 */
int scanInt(const std::string &str, const std::string &value);

/**
 * @brief Dumps a file with the contents of a certain string.
 * 
 * @param fileName 	Name of the file to be written.
 * @param string 	Contents of such file.
 */
void writeTextFile(const std::filesystem::path &fileName, const std::string &string);

/**
 * @brief Dumps a binary file with the contents of a certain vector.
 * 
 * @tparam T 		Vector type
 * @param fileName 	Name of the new file
 * @param vector 	Vector to be dumped
 */
template <typename T>
void writeFile(const std::filesystem::path &fileName, const std::vector<T> &vector)
{
	std::ofstream ofs;
	ofs.open(fileName, std::ios::binary);
	ofs.write((const char *)vector.data(), vector.size() * sizeof(T));
	ofs.close();
}

/**
 * @brief ?
 * 
 * @param value 
 * @param length 
 * @param find 
 * @param str 
 */
void insertValue(int value, int length, const std::string &find, std::string &str);

}
