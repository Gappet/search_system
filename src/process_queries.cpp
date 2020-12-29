#include "process_queries.h"

#include <algorithm>
#include <execution>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
  std::vector<std::vector<Document>> out(queries.size());
  std::transform(std::execution::par, queries.begin(), queries.end(),
                 out.begin(), [&search_server](const std::string& querie) {
                   return search_server.FindTopDocuments(querie);
                 });

  return out;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
  std::vector<std::vector<Document>> out_1(queries.size());
  std::transform(std::execution::par, queries.begin(), queries.end(),
                 out_1.begin(), [&search_server](const std::string& querie) {
                   return search_server.FindTopDocuments(querie);
                 });
  std::vector<Document> out;
  for (std::vector<Document> doc_vec : out_1) {
    out.insert(std::end(out), doc_vec.begin(), doc_vec.end());
  }
  return out;
}
