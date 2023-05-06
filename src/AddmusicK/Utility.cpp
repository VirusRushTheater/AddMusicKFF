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
        for (auto& file : fs::directory_iterator(dir_path)) {
            if (fs::is_directory(file)) {
                AddMusic::deleteDir(file.path());
            } else {
                fs::remove(file);
            }
        }
        fs::remove(dir_path);
    }
}
