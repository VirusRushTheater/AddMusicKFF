/*
Copyright (c) 2018 Tom Henoch

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef TEMPUS_H_
#define TEMPUS_H_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WINDOWS_PLATFORM 1
#include <Windows.h>
#include <process.h>
#else
#define UNIX_PLATFORM 1
#define _GNU_SOURCE
#include <errno.h> // To access  program_invocation_name
#include <stdlib.h>
#include <unistd.h>
#endif

#ifndef TEMPUS_PREFIX
#define TEMPUS_PREFIX tempus
#endif

#define STRINGIFY(args) #args

#include <string>
#include <ctime>
#include <algorithm>

namespace tempus {

namespace _ {

extern std::string AppDirectory;

#ifdef WINDOWS_PLATFORM
         const char* Separator = "\\";
#else
         const char* Separator = "/";
#endif

    // internal helpers to deal with path
    template<class T>
    T base_name(T const & path, T const & delims = "/\\")
    {
      return path.substr(path.find_last_of(delims) + 1);
    }

    template<class T>
    T remove_extension(T const & filename)
    {
      typename T::size_type const p(filename.find_last_of('.'));
      return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
    }

    inline std::string getBaseName(std::string const & path)
    {
        return remove_extension<std::string>( base_name<std::string>( path ));
    }


    inline std::string encodedTime()
    {
        static int counter = std::rand(); // Counter
        std::time_t now = std::time(nullptr);
        char buffer[1024];
        int size = std::snprintf(buffer, 1024, "%llx%04x", now, counter++ );

        return std::string(buffer, size);
    }

    inline std::string getAppDirectory()
    {
        if (AppDirectory.length() > 0 )
            return AppDirectory;

        // Define varibale used in all platform
        int pid = 0;
        std::string base_name, full_name;

        // Retreive temp path
    #ifdef WINDOWS_PLATFORM

        // Get the temp root folder
        TCHAR tmp_path[MAX_PATH ];
        DWORD path_size = GetTempPathA(MAX_PATH , tmp_path);

        if (path_size == 0 || path_size > MAX_PATH ) {
            return std::string();
        }

        TCHAR app_name[MAX_PATH];
        GetModuleFileNameA(NULL, app_name, MAX_PATH );

        pid = _getpid();
        base_name = _::getBaseName(app_name);

    #else



    #endif

        // Generate temp directory
        char folder_name[1024];
        std::snprintf( folder_name, 1024, "%s%i%s", base_name.c_str(), pid, _::encodedTime().c_str());


        // Create directory
    #ifdef WINDOWS_PLATFORM
        full_name = std::string(tmp_path) + std::string(folder_name);
        CreateDirectoryA(full_name.c_str(), NULL);
        _::AppDirectory = full_name;
    #else



    #endif

        return AppDirectory;
    }

}


inline std::string tmpFile(std::string const& name = "")
{
    std::string filename = name;
    if (name.empty())
        filename = _::encodedTime();

    return _::getAppDirectory() + _::Separator + filename;
}

inline std::string tmpDir(std::string const& name = "")
{
    std::string filename = name;
    if (name.empty())
        filename = _::encodedTime();

    filename = _::getAppDirectory() + _::Separator + filename;

#ifdef WINDOWS_PLATFORM
    CreateDirectoryA(filename.c_str(), NULL);
#else

#endif

    return filename;
}


std::string uniqueName(std::string const& prefix = "")
{
    return prefix + _::encodedTime();
}

std::string appTempDirectory()
{
    return _::AppDirectory;
}


#ifdef TEMPUS_IMPL
std::string _::AppDirectory = "";
#endif


}
#endif