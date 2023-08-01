#pragma once 
#include <vector>
#include <string>
#include <unordered_set>
#include "link.h"



std::vector<std::string> explode(const std::string& s);

std::string removeHtmlTags(const std::string& html);

std::vector<std::string> extractLinks(const std::string& html);

Link splitLink(const std::string& link);

std::unordered_set<Link> filterLinks(const std::vector<std::string>& rawLinks, ProtocolType protocol, const std::string& hostName);

std::string GetHtmlContent(const Link& link);