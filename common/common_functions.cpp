#include "common_functions.h"
#include <boost/locale.hpp>


std::string wordToLowerCase(const std::string& word)
{
	using namespace boost::locale;
	using namespace std;

	generator gen;
	locale loc = gen("");
	locale::global(loc);

	auto result = to_lower(word);

	return result;

}

std::vector<std::string> explode(const std::string& s)
{
	std::vector<std::string> result;
	std::string delimiters(" \r\n\t.,;:!&()[]{}\"/=+-*'");

	size_t index = 0;
	std::string tempString;

	while (index < s.size())
	{
		if (delimiters.find(s[index]) == std::string::npos)
		{
			tempString += s[index];
		}
		else
		{
			if (tempString.size() > 0)
			{
				result.push_back(tempString);
			}
			tempString = "";
		}
		index++;
	}

	if (tempString.size() > 0)
	{
		result.push_back(tempString);
	}

	return result;
}