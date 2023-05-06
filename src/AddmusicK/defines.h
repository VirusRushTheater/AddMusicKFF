#pragma once

// Grab yer ducktape, folks!

#ifdef _WIN32
#define strnicmp _strnicmp
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


#define AMKVERSION 		1
#define AMKMINOR 		0
#define AMKREVISION 	8			

#define PARSER_VERSION 	4			// Used to keep track of incompatible changes to the parser

#define DATA_VERSION 	0			// Used to keep track of incompatible changes to any and all compiled data, either to the SNES or to the PC

#define hex2 std::setw(2) << std::setfill('0') << std::uppercase << std::hex
#define hex4 std::setw(4) << std::setfill('0') << std::uppercase << std::hex
#define hex6 std::setw(6) << std::setfill('0') << std::uppercase << std::hex
