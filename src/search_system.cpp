#include <iostream>
#include <string>

#include "document.h"
#include "paginator.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_functions.h"

using namespace std;

int main() {
  SearchServer search_server("и в на"s);
  RequestQueue request_queue(search_server);

  AddDocument(search_server, 1, "пушистый кот пушистый хвост"s,
              DocumentStatus::ACTUAL, {7, 2, 7});
  AddDocument(search_server, 2, "пушистый пёс и модный ошейник"s,
              DocumentStatus::ACTUAL, {1, 2, 3});
  AddDocument(search_server, 3, "большой кот модный ошейник "s,
              DocumentStatus::ACTUAL, {1, 2, 8});
  AddDocument(search_server, 4, "большой пёс скворец евгений"s,
              DocumentStatus::ACTUAL, {1, 3, 2});
  AddDocument(search_server, 5, "большой пёс скворец василий"s,
              DocumentStatus::ACTUAL, {1, 1, 1});

  FindTopDocuments(search_server, "хвос кот пёс"s);

  MatchDocuments(search_server, "кот хвост"s);
}
