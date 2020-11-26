#include "remove_duplicates.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
  std::set<int> dublicates_docs;

    std::set<std::set<std::string>> cash;
    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
      const std::map<std::string, double> doc_words =
          search_server.GetWordFrequencies(*it);
      std::set<std::string> words;
      for (const auto& [key, _] : doc_words) {
        words.insert(key);
      }
      if (cash.count(words)) {
        dublicates_docs.insert(*it);
      } else {
        cash.insert(words);
      }
    }

  for (int i : dublicates_docs) {
    std::cout << "Found duplicate document id "s << i << std::endl;
    search_server.RemoveDocument(i);
  }
}
