#include "remove_duplicates.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
  std::set<int> dublicates_docs;
  for (auto it = search_server.begin(); it < search_server.end(); ++it) {
    const std::map<std::string, double> doc_words =
        search_server.GetWordFrequencies(*it);

    auto doc_id =
        std::find_if(next(it, 1), search_server.end(),
                     [doc_words, search_server](auto left) {			/// при захвате "по-значению" происходит копирование, правильней использовать ссылки
                       const std::map<std::string, double> comp_doc_word =
                           search_server.GetWordFrequencies(left);
                       return doc_words.size() == comp_doc_word.size() &&
                              std::equal(doc_words.begin(), doc_words.end(),
                                         comp_doc_word.begin(),
                                         [](const auto& a, const auto& b) {
                                           return a.first == b.first;
                                         });
                     });
    if (doc_id != search_server.end()) {
      dublicates_docs.insert(*doc_id);
    }
  }

  for (const int& i : dublicates_docs) {
    std::cout << "Found duplicate document id "s << i << std::endl;
    search_server.RemoveDocument(i);
  }
}
