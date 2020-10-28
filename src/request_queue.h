#pragma once

#include <deque>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

class RequestQueue {
 public:
  explicit RequestQueue(const SearchServer& search_server)
      : search_server_(search_server) {}

  template <typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string& raw_query,
                                       DocumentPredicate document_predicate);

  inline std::vector<Document> AddFindRequest(const std::string& raw_query,
                                              DocumentStatus status);

  inline std::vector<Document> AddFindRequest(const std::string& raw_query);

  int GetNoResultRequests() const;

 private:
  struct QueryResult {
    std::vector<Document> result_;
    std::string raw_query_;
  };

  static const int sec_in_day_ = 1440;
  std::deque<QueryResult> requests_;
  int time_ = 0;
  int empty_query_ = 0;
  const SearchServer& search_server_;

  void IncrementTime();

  void SearchProcessing(const QueryResult& query_result);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(
    const std::string& raw_query, DocumentPredicate document_predicate) {
  IncrementTime();
  std::vector<Document> result =
      search_server_.FindTopDocuments(raw_query, document_predicate);

  QueryResult query_result;
  query_result.raw_query_ = raw_query;
  query_result.result_ = result;

  SearchProcessing(query_result);
  return requests_.front().result_;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentStatus status) {
  return RequestQueue::AddFindRequest(
      raw_query, [status](int document_id, DocumentStatus document_status,
                          int rating) { return document_status == status; });
}

std::vector<Document> RequestQueue::AddFindRequest(
    const std::string& raw_query) {
  return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}
