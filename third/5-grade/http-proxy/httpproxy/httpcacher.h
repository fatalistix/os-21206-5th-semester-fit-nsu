/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

#include <chrono>
#include <ctime>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "./blockingqueue.h"

namespace httpproxy {

struct CacheData {
  std::vector<char> data;
  std::chrono::system_clock::time_point creationTime;
};

enum class CacheEventType : char {
  REQUEST = 0,
  ANSWER,
  EXIT,
};

struct CacheResult {
  std::vector<char> data;
  bool hasCache;
};

struct CacheEvent {
  std::vector<char> request;
  std::vector<char> response;
  std::promise<CacheResult> *promise;
  CacheEventType cacheEventType;
};

class HttpCacher {
public:
  HttpCacher();

  ~HttpCacher();

  void runThreads();
  void cache(const std::vector<char> &request, const std::vector<char> &data);
  bool getCache(const std::vector<char> &request, std::vector<char> *data);

private:
  // pthread_mutex_t mutex;
  std::mutex mutex;
  std::map<std::vector<char>, CacheData> cachingMap;

  blocking_queue<CacheEvent> eventQueue;

  std::promise<void> exitPromise;
  std::future<void> exitFuture = exitPromise.get_future();
  int CACHE_TIMEOUT_MILLIS = 2000;
};
} // namespace httpproxy
