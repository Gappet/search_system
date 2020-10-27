#include "document.h"

#include <iostream>
#include <string>

using namespace std::string_literals;

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}

std::ostream& operator<<(std::ostream& out, const Document& document) {
  out << "{ "s
           << "document_id = "s << document.id << ", "s
           << "relevance = "s << document.relevance << ", "s
           << "rating = "s << document.rating << " }"s;
  return out;
}

/// дублирущая функция оператор выше, постарайтесь избавиться от нее
void PrintDocument(const Document& document) {
  std::cout << "{ "s
            << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << std::endl;
}

