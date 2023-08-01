#include "common.h"
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