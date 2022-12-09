#pragma once

#include <algorithm>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief Reads a binary file and stores its contents in a vector.
 */
template <typename T>
inline void readBinaryFile(const fs::path &fileName, std::vector<T> &v)
{
	std::ifstream is (fileName, std::ios::binary);
	v.assign(std::istream_iterator<T>(is), std::istream_iterator<T>());
	is.close();
}

/**
 * @brief Reads a text file and stores its contents in a string.
 */
inline void readTextFile(const fs::path &fileName, std::string &str)
{
	std::ifstream is (fileName);
	str.assign(std::istream_iterator<char>(is), std::istream_iterator<char>());
	is.close();
}

/**
 * @brief Writes the contents of a vector in a binary file. 
 */
template <typename T>
inline void writeBinaryFile(const fs::path &fileName, std::vector<T> &v)
{
	std::ofstream ofs (fileName, std::ios::binary);
	std::copy(v.begin(), v.end(), std::ostream_iterator<T>(ofs));
	ofs.close();
}

/**
 * @brief Writes a string into a text file. 
 */
inline void writeTextFile(const fs::path &fileName, const std::string &str)
{
	std::ofstream ofs (fileName);
	std::copy(str.begin(), str.end(), std::ostream_iterator<char>(ofs));
	ofs.close();
}

/**
 * @brief Function that encodes a byte stream into a null-terminated, base64
 * string.
 * Returns the size of the resulting string, not counting the trailing zero.
 * 
 * Comes from the public domain repository https://github.com/zhicheng/base64
 */
size_t base64_encode(const uint8_t *in, size_t inlen, char *out);

/**
 * @brief Function that decodes a base-64 string into a byte stream.
 * Returns the size of the resulting byte stream.
 * 
 * Comes from the public domain repository https://github.com/zhicheng/base64
 */
size_t base64_decode(const char *in, size_t inlen, uint8_t *out);

}