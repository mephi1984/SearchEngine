#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "../common/ini_parser.h"
#include "parser_database_worker.h"
#include "http_utils.h"
#include "../common/common_functions.h"
#include <functional>


std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;


void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}
void parseLink(ParserDatabaseWorker& databaseWorker, Link link, int depth, int index, int ofTotalCount)
{
	try {

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::string html = getHtmlContent(link);

		if (html.size() == 0)
		{
			std::cout << "Failed to parse: " << link.hostName << " " << link.query << std::endl;

			return;
		}

		std::vector<std::string> rawLinks = extractLinks(html);

		std::unordered_set<Link> links = filterLinks(rawLinks, link.protocol, link.hostName);

		std::string text = removeHtmlTags(html);

		std::vector<std::string> words = explode(text);

		std::map<std::string, int> wordCount = countWords(words);

		databaseWorker.addDocumentWithWordsIfNotExists(link, wordCount);

		std::cout << "Parsed: " << link.hostName << " " << link.query << " with " << words.size() << " words, depth: " << depth << " item " << index << " of " << ofTotalCount << std::endl;



		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				bool docExists = databaseWorker.documentExists(subLink);
				if (!docExists)
				{
					tasks.push([&databaseWorker, subLink, depth, index, count]() { parseLink(databaseWorker, subLink, depth - 1, index, count); });
					index++;
				}
			}
			cv.notify_one();
		}

	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}



int main()
{
	try {

		IniParser iniParser("settings.ini");

		ParserDatabaseWorker databaseWorker(iniParser);

		std::string startPage = iniParser.getStartPage();
		int level = iniParser.getLevel();

		Link link = splitLink(startPage);

		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}


		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([&databaseWorker, link, level]() { parseLink(databaseWorker, link, level, 1, 1); });
			cv.notify_one();
		}


		std::this_thread::sleep_for(std::chrono::seconds(2));


		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
