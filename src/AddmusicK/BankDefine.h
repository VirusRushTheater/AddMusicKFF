#ifndef _BANKDEFINE_H
#define _BANKDEFINE_H

#include <vector>
#include <memory>
#include <string>

class BankDefine
{
public:
	std::string name;
	std::vector<std::unique_ptr<const std::string>> samples;
	std::vector<bool> importants;
};

#endif
