// search_server_s1_t1_v2.cpp

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
        document_feature_.emplace(document_id,DocumentFeature{ComputeAverageRating(ratings),status});
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        const Query query = ParseQuery(raw_query);
        auto find_documents = FindAllDocuments(query);
        vector<Document> matched_documents;
        for (const auto& document : find_documents) {                       // переделать на константную ссылку, и переименовать i на более пожходящее, i обычно используется для индексов
            if (filter(document.id,document_feature_.at(document.id).stat, document_feature_.at(document.id).rating)) {
                matched_documents.push_back(document);
            }
        }

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {

                 if(abs(lhs.relevance-rhs.relevance)<EPSILON) {
                     return lhs.rating > rhs.rating;
                 }else {
                    return lhs.relevance > rhs.relevance;
                 }

             });


        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status] (int document_id, DocumentStatus stat, int rating) { return stat==status;});
    }




    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const vector<string> words = SplitIntoWordsNoStop(raw_query);
        set<string> CheckedWords;
        for (const auto& word : words) {                                // word не меняется, должно быть const
            const QueryWord query_word = ParseQueryWord(word);
            if(word_to_document_freqs_.count(query_word.data) && word_to_document_freqs_.at(query_word.data).count(document_id)) {
                if(query_word.is_minus){
                    return tuple(vector<string>{},document_feature_.at(document_id).stat);  // можно сразу выдать tuple({}, ...) это будет понятно, что пустой
                }
                else {
                    CheckedWords.insert(word);
                }
            }
        }

        vector<string> out(CheckedWords.begin(),CheckedWords.end());
        return tie(out,document_feature_.at(document_id).stat);

    }



    int GetDocumentCount() const{                    // ментод должен быть константным
        return document_feature_.size();
    }

private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map <int,DocumentFeature>  document_feature_;
        // заменить два контейнера document_ratings_ и document_status на один. для этого добавить структуру со статусом и рейтингом, и контейнер (map) сделать от этого контейнера
    	// и поля должны отличаться от переменных, если принято, что имеют в конце подчеркивание, то и дальше следуйте этому.


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



void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});



    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот", [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}
