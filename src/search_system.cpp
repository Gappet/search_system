#include <execution>
#include <iostream>
#include <string>
#include <vector>

#include "process_queries.h"
#include "search_server.h"

using namespace std;

void PrintDocument(const Document& document) {
  cout << "{ "s
       << "document_id = "s << document.id << ", "s
       << "relevance = "s << document.relevance << ", "s
       << "rating = "s << document.rating << " }"s << endl;
}

int main() {
  cout << "Enter stop words"s << endl;
  string stop_words = ReadLine();

  SearchServer search_server(stop_words);
  int id = 0;

  cout << "Enter quantity of docs"s << endl;
  int quantity = ReadLineWithNumber();

  for (int i = 0; i < quantity; ++i) {
    search_server.AddDocument(++id, ReadLine(), DocumentStatus::ACTUAL, {1, 2});
  }

  cout << "Quantity of query"s << endl;
  quantity = ReadLineWithNumber();
  string query;

  for (int i = 0; i < quantity; ++i) {
    for (const Document& document :
         search_server.FindTopDocuments(ReadLine())) {
      PrintDocument(document);
    }
  }

  return 0;
}
