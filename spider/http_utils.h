#pragma once 
#include <vector>
#include <string>
#include <unordered_set>
#include <map>
#include "link.h"


std::vector<std::string> explode(const std::string& s);

std::string removeHtmlTags(const std::string& html);

std::map<std::string, int> countWords(const std::vector<std::string>& words);

std::vector<std::string> extractLinks(const std::string& html);

Link splitLink(const std::string& link);

std::unordered_set<Link> filterLinks(const std::vector<std::string>& rawLinks, ProtocolType protocol, const std::string& hostName);

std::string getHtmlContent(const Link& link);

