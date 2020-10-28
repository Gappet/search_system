#include <iostream>

#include "test_functions.h"

using namespace std::string_literals;


void PrintMatchDocumentResult(int document_id,
                              const std::vector<std::string>& words,
                              DocumentStatus status) {
  std::cout << "{ "s
            << "document_id = "s << document_id << ", "s
            << "status = "s << static_cast<int>(status) << ", "s
            << "words ="s;
  for (const std::string& word : words) {
    std::cout << ' ' << word;
  }
  std::cout << "}" << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id,
                 const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
  try {
    search_server.AddDocument(document_id, document, status, ratings);
  } catch (const std::invalid_argument& e) {
    std::cout << "Ошибка добавления документа "s << document_id << ": "s
              << e.what() << std::endl;
  }
}

void FindTopDocuments(const SearchServer& search_server,
                      const std::string& raw_query) {
  std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
  try {
    for (const Document& document : search_server.FindTopDocuments(raw_query)) {
      std::cout << document << std::endl;
    }
  } catch (const std::invalid_argument& e) {
    std::cout << "Ошибка поиска: "s << e.what() << std::endl;
  }
}

void MatchDocuments(const SearchServer& search_server,
                    const std::string& query) {
  try {
    std::cout << "Матчинг документов по запросу: "s << query << std::endl;
    const int document_count = search_server.GetDocumentCount();
    for (int index = 0; index < document_count; ++index) {
      const int document_id = search_server.GetDocumentId(index);
      const auto [words, status] =
          search_server.MatchDocument(query, document_id);
      PrintMatchDocumentResult(document_id, words, status);
    }
  } catch (const std::invalid_argument& e) {
    std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s
              << e.what() << std::endl;
  }
}


