#pragma once 

enum class ProtocolType
{
	HTTP = 0,
	HTTPS = 1
};

struct Link
{
	ProtocolType protocol;
	std::string hostName;
	std::string query;

	bool operator==(const Link& l) const
	{
		return protocol == l.protocol
			&& hostName == l.hostName
			&& query == l.query;
	}
};

namespace std {

	template <>
	struct hash<Link> {
		std::size_t operator()(const Link& link) const noexcept {
			std::size_t h1 = std::hash<int>{}(static_cast<int>(link.protocol));
			std::size_t h2 = std::hash<std::string>{}(link.hostName);
			std::size_t h3 = std::hash<std::string>{}(link.query);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

} 
