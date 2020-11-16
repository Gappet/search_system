#include "remove_duplicates.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
  std::set<int> dublicates_docs;
  for (auto it = search_server.begin(); it != search_server.end(); ++it) {
    const std::map<std::string, double> doc_words =
        search_server.GetWordFrequencies(*it);
    for (auto it_int = next(it, 1); it_int != search_server.end(); ++it_int) {
      const std::map<std::string, double> comp_doc_words =
          search_server.GetWordFrequencies(*it_int);
      if (doc_words.size() == comp_doc_words.size() &&
          std::equal(doc_words.begin(), doc_words.end(), comp_doc_words.begin(),
                     [](const auto& a, const auto& b) {
                       return a.first == b.first;
                     })) {
        dublicates_docs.insert(*it_int);
        break;
      }
    }
  }

  for (int i : dublicates_docs) {
    std::cout << "Found duplicate document id "s << i << std::endl;
    search_server.RemoveDocument(i);
  }
}
