#pragma once

#include <algorithm>
#include <execution>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const size_t MAX_BUCKET_COUNT = 3;

class SearchServer {
 public:
  explicit SearchServer(const std::string& stop_words_text);

  explicit SearchServer(const std::string_view stop_words_text);

  template <typename StringContainer>
  explicit SearchServer(const StringContainer& stop_words);

  void AddDocument(int document_id, const std::string_view document,
                   DocumentStatus status, const std::vector<int>& ratings);

  template <typename ExecutionPolicy, typename DocumentPredicate>
  std::vector<Document> FindTopDocuments(
      ExecutionPolicy&& policy, const std::string_view raw_query,
      DocumentPredicate document_predicate) const;

  template <typename DocumentPredicate>
  std::vector<Document> FindTopDocuments(
      const std::string_view raw_query,
      DocumentPredicate document_predicate) const;

  template <typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,
                                         const std::string_view raw_query,
                                         DocumentStatus status) const;

  std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                         DocumentStatus status) const;

  template <typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(
      ExecutionPolicy&& policy, const std::string_view raw_query) const;

  std::vector<Document> FindTopDocuments(
      const std::string_view raw_query) const;

  int GetDocumentCount() const;

  std::set<int>::const_iterator begin();

  std::set<int>::const_iterator end();

  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
      const std::string_view, int document_id) const;

  template <typename ExecutionPolicy>
  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
      ExecutionPolicy&& policy, const std::string_view raw_query,
      int document_id) const;

  const std::map<std::string_view, double> GetWordFrequencies(
      int document_id) const;

  void RemoveDocument(int document_id);

  template <typename ExecutionPolicy>
  void RemoveDocument(ExecutionPolicy&& policy, int document_id);

 private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };
  const std::set<std::string> stop_words_;
  std::map<std::string, std::map<int, double>> word_to_document_freqs_;
  std::map<int, DocumentData> documents_;
  std::set<int> document_ids_;
  std::map<int, std::map<std::string, double>> words_frequencies_;

  bool IsStopWord(const std::string& word) const;

  static bool IsValidWord(const std::string& word);

  std::vector<std::string> SplitIntoWordsNoStop(
      const std::string_view text) const;

  static int ComputeAverageRating(const std::vector<int>& ratings);

  struct QueryWord {
    std::string data;
    bool is_minus;
    bool is_stop;
  };

  QueryWord ParseQueryWord(const std::string& text) const;

  struct Query {
    std::set<std::string> plus_words;
    std::set<std::string> minus_words;
  };

  Query ParseQuery(const std::string_view text) const;

  template <typename ExecutionPolicy>
  Query ParseQuery(ExecutionPolicy&& policy, const std::string& text) const;

  double ComputeWordInverseDocumentFreq(const std::string& word) const;

  template <typename ExecutionPolicy, typename DocumentPredicate>
  std::vector<Document> FindAllDocuments(
      ExecutionPolicy&& policy, const Query& query,
      DocumentPredicate document_predicate) const;

  template <typename DocumentPredicate>
  std::vector<Document> FindAllDocuments(
      const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
  if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
    throw std::invalid_argument(std::string("Some of stop words are invalid"));
  }
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy&& policy, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
  const auto query = ParseQuery(raw_query);

  auto matched_documents = FindAllDocuments(policy, query, document_predicate);

  sort(matched_documents.begin(), matched_documents.end(),
       [](const Document& lhs, const Document& rhs) {
         if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
           return lhs.rating > rhs.rating;
         } else {
           return lhs.relevance > rhs.relevance;
         }
       });
  if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
  }

  return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
  return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy&& policy, const std::string_view raw_query,
    DocumentStatus status) const {
  return FindTopDocuments(
      policy, raw_query,
      [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
      });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy&& policy, const std::string_view raw_query) const {
  return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    ExecutionPolicy&& policy, const Query& query,
    DocumentPredicate document_predicate) const {
  ConcurrentMap<int, double> document_to_relevance_concurrent(MAX_BUCKET_COUNT);

  for (const std::string& word : query.plus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
    auto documents = &documents_;
    for_each(
        policy, word_to_document_freqs_.at(word).begin(),
        word_to_document_freqs_.at(word).end(),
        [&documents, &document_to_relevance_concurrent, &document_predicate,
         &inverse_document_freq](const auto doc_id_freq) {
          const auto& document_data = documents->at(doc_id_freq.first);
          if (document_predicate(doc_id_freq.first, document_data.status,
                                 document_data.rating)) {
            document_to_relevance_concurrent[doc_id_freq.first].ref_to_value +=
                (doc_id_freq.second * inverse_document_freq);
          }
        });
  }

  std::map<int, double> document_to_relevance =
      document_to_relevance_concurrent.BuildOrdinaryMap();
  for (const std::string& word : query.minus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
      document_to_relevance.erase(document_id);
    }
  }

  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] : document_to_relevance) {
    matched_documents.push_back(
        {document_id, relevance, documents_.at(document_id).rating});
  }
  return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const Query& query, DocumentPredicate document_predicate) const {
  return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
  std::map<std::string, double> delete_doc;
  if (words_frequencies_.count(document_id)) {
    delete_doc = words_frequencies_[document_id];
    words_frequencies_.erase(document_id);
    for (const auto& it : delete_doc) {
      if (word_to_document_freqs_.count(it.first) &&
          word_to_document_freqs_.at(it.first).count(document_id)) {
        word_to_document_freqs_.at(it.first).erase(document_id);
      }
    }

    auto itr = std::find(policy, document_ids_.begin(), document_ids_.end(),
                         document_id);
    document_ids_.erase(itr);
    documents_.erase(document_id);
  }
}

template <typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(ExecutionPolicy&& policy,
                            const std::string_view raw_query,
                            int document_id) const {
  const auto query = ParseQuery(raw_query);
  std::vector<std::string_view> matched_words;

  std::for_each(policy, words_frequencies_.at(document_id).begin(),
                words_frequencies_.at(document_id).end(),
                [&query, &matched_words](std::string& word, double& _) {
                  if (query.plus_words.count(word) == 0) {
                    matched_words.push_back(word);
                  }
                });

  std::for_each(policy, words_frequencies_.at(document_id).begin(),
                words_frequencies_.at(document_id).end(),
                [&query, &matched_words](std::string& word, double& _) {
                  if (query.minus_words.count(word)) {
                    matched_words.clear();
                  }
                });
  return {matched_words, documents_.at(document_id).status};
}
