#include "request_queue.h"

int RequestQueue::GetNoResultRequests() const { return empty_query_; }

void RequestQueue::IncrementTime() {
  if (time_ != sec_in_day_) {
    ++time_;
  } else {
    time_ = 0;
  }
}

void RequestQueue::SearchProcessing(const QueryResult& query_result) {
  if (query_result.result_.empty()) {
 	  ++empty_query_;
       }
       if (requests_.size() < sec_in_day_) {
 	  requests_.push_back(query_result);
       } else {
 	  if (requests_.front().result_.empty() && empty_query_ != 0) {
 	      --empty_query_;
 	  }
 	  requests_.pop_front();
 	  requests_.push_back(query_result);
       }

}
