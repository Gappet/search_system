//============================================================================
// Name        : test_search_system_macros.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

struct DocumentFeature {
    int rating;
    DocumentStatus stat;
};

class SearchServer {
 public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_feature_.emplace(document_id, DocumentFeature{ComputeAverageRating(ratings), status});
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        const Query query = ParseQuery(raw_query);
        auto find_documents = FindAllDocuments(query);
        vector<Document> matched_documents;
        for (const auto& document : find_documents) {
            if (filter(document.id, document_feature_.at(document.id).stat, document_feature_.at(document.id).rating)) {
                matched_documents.push_back(document);
            }
        }

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if ( abs(lhs.relevance-rhs.relevance) < EPSILON ) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status] (int document_id, DocumentStatus stat, int rating) { return stat == status;});
    }




    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const vector<string> words = SplitIntoWordsNoStop(raw_query);
        set<string> CheckedWords;
        for (const auto& word : words) {                                // word не меняется, должно быть const
            const QueryWord query_word = ParseQueryWord(word);
            if ( word_to_document_freqs_.count(query_word.data) && word_to_document_freqs_.at(query_word.data).count(document_id) ) {
                if ( query_word.is_minus ) {
                    return tuple(vector<string>{}, document_feature_.at(document_id).stat);  // можно сразу выдать tuple({}, ...) это будет понятно, что пустой
                } else {
                    CheckedWords.insert(word);
                }
            }
        }

        vector<string> out(CheckedWords.begin(), CheckedWords.end());
        return tie(out, document_feature_.at(document_id).stat);
    }



    int GetDocumentCount() const {
        return document_feature_.size();
    }

 private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map <int, DocumentFeature>  document_feature_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(document_feature_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                document_feature_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))





void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size()== 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Сервер не исключает стоп слова");
    }
}

void TestAddDocument() {
  const int doc_id = 42;
  const vector<int> ratings = {1, 2, 3};
  const string content = "cat in the city"s;

  {
  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments(content, DocumentStatus::ACTUAL);
  ASSERT_HINT(found_docs.at(0).id == doc_id, "Сервер не находит добавленные документы");
  }

  {
  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Сервер неправильно считает количество документов");
  }
}


void TestExcludeDocsWithMinusWords() {
  const int doc_id0 = 42;
  const vector<int> ratings = {1, 2, 3};
  const string content0 = "cat in the city"s;
  const int doc_id1 = 43;
  const string content1 = "cat in the"s;
  {
  SearchServer server;
  server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("cat in city -the", DocumentStatus::ACTUAL);
  ASSERT_HINT(found_docs.empty(), "Сервер не учитывает минус-слова");
  }

  {
  SearchServer server;
  server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("cat in the -city", DocumentStatus::ACTUAL);
  ASSERT_HINT(found_docs.at(0).id == 43, "Сервер игнорирует минус-слово в документе");
  }

  {
  SearchServer server;
  server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("cat in the -dog", DocumentStatus::ACTUAL);
  for ( const auto& doc : found_docs ) {
      ASSERT_HINT(doc.id == 42 || doc.id == 43, "Сервер находит несуществующие минус-слова в докумете");
      }
  }
  }



void TestMatchDocs() {
  const int doc_id = 42;
  const vector<int> ratings = {1, 2, 3};
  const string content = "cat in the city"s;
  {
    SearchServer server;
    const string query = "in the"s;
    const vector<string> VectorQuery = SplitIntoWords(query);
    const vector<string> VectorContent = SplitIntoWords(content);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    auto words = server.MatchDocument(query, doc_id);
    for ( const string& word : get<0>(words) ) {
         ASSERT_HINT(find(VectorQuery.begin(), VectorQuery.end(), word) != VectorQuery.end(), "Полученного слова нет в запросе");
    }
    for ( const string& word : get<0>(words) ) {
     ASSERT_HINT(find(VectorContent.begin(), VectorContent.end(), word) != VectorContent.end(), "Полученного слова нет в добавленном документе");
     }
    }

    {
      SearchServer server;
      const string query = "in -the"s;
      const vector<string> VectorContent = SplitIntoWords(content);
      server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
      const auto words = server.MatchDocument(query, doc_id);
      ASSERT_HINT(get<0>(words).empty(), "Метод MatchDocument не учитывает минус-слова");
    }
}



void TestSortingRelevance() {
  {
  SearchServer server;
  server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
  server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {8, -3});
  server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3});
  server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::ACTUAL, {6, 4});
  const auto documents = server.FindTopDocuments("пушистый ухоженный кот", DocumentStatus::ACTUAL);
  for ( size_t i = 1; i < documents.size(); ++i ) {   /// рекомендация, объявить size_t i, тогда не нужно будет приведение типов
       ASSERT_HINT(documents[i - 1].relevance - documents[i].relevance > EPSILON  /// вместо 1e-6 должна быть константа, подсказка: она уже у вас объявлена в самом начале
       || (abs(documents[i - 1].relevance - documents[i].relevance) < EPSILON && (documents[i - 1].rating >= documents[i].rating)), "Сервер неправильно сортирует документы");    /// тут то почему оставили 1e-6?
      }
  }
}


void TestComputeAverageRating() {
     const int doc_id = 42;
     const vector<int> ratings = {1, 2, 3};
     const string content = "cat in the city"s;
     {
       SearchServer server;
       server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
       const auto found_docs = server.FindTopDocuments(content, DocumentStatus::ACTUAL);
       ASSERT_EQUAL_HINT(found_docs.at(0).rating, 2, "Сервер неправильно считает среднии рейтинг");
     }
}

void TestUsePredicat() {
     {
      SearchServer server;
      server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
      server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
      server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
      server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::ACTUAL, {9});
      const auto documents =server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
      for ( const auto& doc : documents ) {
         ASSERT_EQUAL_HINT(doc.id % 2, 0, "Ошибка фильтрации документов по id");
      }
      }




     {
      SearchServer server;
      server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -4});
      server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, -5});
      server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
      server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::ACTUAL, {9});
      const auto documents =server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return rating == 2; });
      for ( const auto& doc : documents ) {
          ASSERT_EQUAL_HINT(doc.rating, 2, "Ошибка фильтрации документов по рейтингу");
      }
      }

     {
       SearchServer server;
       server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::BANNED, {8, -4});
       server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, -5});
       server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, {5, -12, 2, 1});
       server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::ACTUAL, {9});
       const auto documents =server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
       ASSERT_HINT(documents.empty(), "Ошибка фильтрации документов по статусу");
       }
}

void TestSearchDocWithStatus() {
      {
        SearchServer server;
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::BANNED, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::REMOVED, {9});
        ASSERT_HINT(DocumentStatus::BANNED == get<1>(server.MatchDocument("белый кот и модный ошейник", 0)), "Документу присвоен некорректный статус");
        ASSERT_HINT(DocumentStatus::ACTUAL == get<1>(server.MatchDocument("пушистый кот пушистый хвост", 1)), "Документу присвоен некорректный статус");
        ASSERT_HINT(DocumentStatus::IRRELEVANT == get<1>(server.MatchDocument("ухоженный пёс выразительные глаза", 2)), "Документу присвоен некорректный статус");
        ASSERT_HINT(DocumentStatus::REMOVED == get<1>(server.MatchDocument("ухоженный скворец евгений", 3)), "Документу присвоен некорректный статус");
      }



      {
        SearchServer server;
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот", DocumentStatus::ACTUAL);
        for ( const auto& doc : found_docs ) {
            ASSERT_HINT(doc.id != 3, "В поисковой запрос включаются документы с некорректным статусом");
            }
      }
}

void TestComputeTFIDF() {
     {
       SearchServer server;
       server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
       server.AddDocument(1, "белый кот и модный"s,       DocumentStatus::ACTUAL, {7, 2, 7});
       const auto found_docs = server.FindTopDocuments("ошейник", DocumentStatus::ACTUAL);
       ASSERT_HINT(found_docs.at(0).relevance == log(2.0 / 1.0) * (1.0 / 5.0), "Неправильно считается релевантность найденных документов");
      }
}




/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestAddDocument();
    TestExcludeDocsWithMinusWords();
    TestMatchDocs();
    TestSortingRelevance();
    TestComputeAverageRating();
    TestUsePredicat();
    TestSearchDocWithStatus();
    TestComputeTFIDF();

    // Не забудьте вызывать остальные тесты здесь
}
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    return 0;
}


