#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
  explicit RequestQueue(const SearchServer& search_server)
      : search_server_(search_server) {}

  template <typename DocumentPredicate>		/// реализию шаблонов лучше обрать за класс (ниже под классом)
  std::vector<Document> AddFindRequest(const std::string& raw_query,
                                       DocumentPredicate document_predicate) {
    IncrementTime();
    std::vector<Document> result =
        search_server_.FindTopDocuments(raw_query, document_predicate);

    QueryResult query_result;
    query_result.raw_query_ = raw_query;
    query_result.result_ = result;

    SearchProcessing(query_result);
    return requests_.front().result_;
  }

  std::vector<Document> AddFindRequest(const std::string& raw_query,	/// реализию инлайновых функций так же лучше обрать за класс (ниже под классом)
                                       DocumentStatus status) {
    return AddFindRequest(
        raw_query, [status](int document_id, DocumentStatus document_status,
                            int rating) { return document_status == status; });
  }

  std::vector<Document> AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
  }

  int GetNoResultRequests() const;

 private:
  struct QueryResult {
    std::vector<Document> result_;
    std::string raw_query_;
    // определите, что должно быть в структуре
  };

  static const int sec_in_day_ = 1440;
  std::deque<QueryResult> requests_;
  int time_ = 0;
  int empty_query_ = 0;
  const SearchServer& search_server_;
  void IncrementTime();
  void SearchProcessing(const QueryResult& query_result);
};
