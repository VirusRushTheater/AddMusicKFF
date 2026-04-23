#include <cstdio>
#include <fstream>
#include <exception>
#include "Utility.h"

#define BASE64_PAD '='
#define BASE64DE_FIRST '+'
#define BASE64DE_LAST 'z'

using namespace AddMusic;

void AddMusic::copyDir(const fs::path& src, const fs::path& dst)
{
    if (!fs::exists(src) || !fs::is_directory(src)) {
        throw fs::filesystem_error("Source directory does not exist or is not a directory", src, std::error_code());
        return;
    }

	if (!fs::exists(dst))
		fs::create_directory(dst);

    for (auto& entry : fs::directory_iterator(src)) {
        const fs::path& path = entry.path();
        const fs::path& new_path = dst / path.filename();
        if (fs::is_directory(path))
		{
			// To avoid recursive creating of destination folders if the destination
			// is within the source.
			if (entry != dst)
	            AddMusic::copyDir(path, new_path);
        }
		else
		{
            fs::copy_file(path, new_path, fs::copy_options::overwrite_existing);
        }
    }
}

void AddMusic::deleteDir(const fs::path& dir_path)
{
    if (fs::exists(dir_path)) {
        fs::remove_all(dir_path);
    }
}

bool AddMusic::isPathEquivalent(const fs::path& p1, const fs::path& p2)
{
    fs::path abs_p1 = p1.lexically_normal();
    fs::path abs_p2 = p2.lexically_normal();
#ifdef _WIN32
    std::string str_p1 = abs_p1.string();
    std::string str_p2 = abs_p2.string();
    std::transform(str_p1.begin(), str_p1.end(), str_p1.begin(), ::tolower);
    std::transform(str_p2.begin(), str_p2.end(), str_p2.begin(), ::tolower);
    return str_p1 == str_p2;
#else
    return abs_p1 == abs_p2;
#endif
}