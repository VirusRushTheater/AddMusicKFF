#ifndef A19FB42A_015E_4F89_AEB5_F7F45463F2B3
#define A19FB42A_015E_4F89_AEB5_F7F45463F2B3

#include <vector>
#include <string>

#include "Directory.h"	// File class

/**
 * @brief Opens a file and fills a vector with its contents.
 * 
 * @param fileName 	File name
 * @param v 		Vector to be filled
 */
void openFile(const File &fileName, std::vector<uint8_t> &v);

/**
 * @brief Opens a file and fills a string with its contents.
 * 
 * @param fileName 	File name
 * @param s 		String to be filled
 */
void openTextFile(const File &fileName, std::string &s);

/**
 * @brief Get a file's time stamp.
 * 
 * @param file 		File name
 * 
 * @return time_t 	Timestamp of the file.
 */
time_t getTimeStamp(const File &file);

void printError(const std::string &error, bool isFatal, const std::string &fileName, int line);
void printWarning(const std::string &error, const std::string &fileName, int line = 0);
void quit(int code);
int execute(const File &command, bool prepend);

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
 * @brief Checks whether a file exists or not
 * 
 * @param fileName 	Name of the file to be checked.
 * @return 
 */
bool fileExists(const File &fileName);

/**
 * @brief Gets the size, in bytes, of a certain file.
 * 
 * @param fileName 	Name of the file to be checked.
 * @return unsigned int 
 */
unsigned int getFileSize(const File &fileName);

/**
 * @brief Removes a file.
 * 
 * @param fileName 	Name of the file to be removed.
 */
void removeFile(const File &fileName);

/**
 * @brief Writes a file with the contents of a certain string.
 * 
 * @param fileName 	Name of the file to be written.
 * @param string 	Contents of such file.
 */
void writeTextFile(const File &fileName, const std::string &string);

/**
 * @brief ?
 * 
 * @param value 
 * @param length 
 * @param find 
 * @param str 
 */
void insertValue(int value, int length, const std::string &find, std::string &str);

#endif /* A19FB42A_015E_4F89_AEB5_F7F45463F2B3 */
