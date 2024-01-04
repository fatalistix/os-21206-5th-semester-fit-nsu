/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include "./httpcacher.h"
#include <algorithm>
#include <bits/chrono.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <queue>

httpproxy::HttpCacher::HttpCacher() {}

void cleanerFunction(
    std::future<void> *exitFuture, int revalidateTimeMillis, int cacheTime,
    std::mutex *mutex,
    std::map<std::vector<char>, httpproxy::CacheData> *cachingMap) {
  while (exitFuture->wait_for(std::chrono::milliseconds(
             revalidateTimeMillis)) == std::future_status::timeout) {
    std::cout << "IT's TIME TO CLEAN!!!" << std::endl;
    auto now = std::chrono::system_clock::now();
    {
      std::cout << "CLEANER before lock!!" << std::endl;
      std::lock_guard lk(*mutex);
      // int err = pthread_mutex_lock(mutex);
      // if (err) {
      //   std::cerr << "ERROR LOCKING MUTEX!!!!!!!" << std::endl;
      //   std::cerr << strerror(errno) << std::endl;
      //   return;
      // }
      std::cout << "CLEANER LOCKED!!" << std::endl;
      auto iter = cachingMap->begin();
      std::vector<std::vector<char>> forRemoval;
      while (iter != cachingMap->end()) {
        std::cout << "I AM STILL ITERATING" << std::endl;
        auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now - iter->second.creationTime)
                              .count();
        if (difference >= cacheTime) {
          forRemoval.emplace_back(iter->first);
          // std::cout << "DELETING: " << forDelete->first.data() << std::endl;
          std::cout << "DELETING" << std::endl;
        }
        ++iter;
      }
      for (auto &i : forRemoval) {
        cachingMap->erase(i);
      }
      // err = pthread_mutex_unlock(mutex);
      // if (err) {
      //   std::cerr << "ERROR UNLOCKING MUTEX!!!!!!!" << std::endl;
      //   std::cerr << strerror(errno) << std::endl;
      //   return;
      // }
    }
  }
  std::cout << "CLEANER THREAD: I AM EXITING..." << std::endl;
}

void eventFunction(
    blocking_queue<httpproxy::CacheEvent> *eventBlockingQueue,
    std::mutex *mutex,
    std::map<std::vector<char>, httpproxy::CacheData> *cachingMap) {
  std::map<std::vector<char>,
           std::queue<std::promise<httpproxy::CacheResult> *>>
      waiters;
  while (true) {
    auto cacheEvent = eventBlockingQueue->pop();
    switch (cacheEvent.cacheEventType) {
    case httpproxy::CacheEventType::REQUEST: {
      std::lock_guard lk(*mutex);
      auto iter = cachingMap->find(cacheEvent.request);
      if (iter == cachingMap->end()) {
        auto waitingIter = waiters.find(cacheEvent.request);
        if (waitingIter == waiters.end()) {
          waiters.insert(std::make_pair(
              cacheEvent.request,
              std::queue<std::promise<httpproxy::CacheResult> *>()));
          cacheEvent.promise->set_value(httpproxy::CacheResult{{}, false});
        } else {
          waitingIter->second.push(cacheEvent.promise);
        }
      } else {
        cacheEvent.promise->set_value(
            httpproxy::CacheResult{iter->second.data, true});
      }
    } break;
    case httpproxy::CacheEventType::ANSWER: {
      auto waitingIter = waiters.find(cacheEvent.request);
      if (waitingIter != waiters.end()) {
        auto waitersQueue = waitingIter->second;
        for (; !waitersQueue.empty(); waitersQueue.pop()) {
          auto w = waitersQueue.front();
          w->set_value(httpproxy::CacheResult{cacheEvent.response, true});
        }
        waiters.erase(waitingIter);
      }
      std::lock_guard lk(*mutex);
      cachingMap->insert(std::make_pair(
          cacheEvent.request,
          httpproxy::CacheData{cacheEvent.response,
                               std::chrono::system_clock::now()}));
    } break;
    case httpproxy::CacheEventType::EXIT: {
      return;
    } break;
    }
  }
}

struct CleanerThreadArgs {
  std::future<void> *exitFuture;
  int revalidateTimeMillis;
  int cacheTime;
  std::mutex *mutex;
  std::map<std::vector<char>, httpproxy::CacheData> *cachingMap;
};

struct EventThreadArgs {
  blocking_queue<httpproxy::CacheEvent> *eventBlockingQueue;
  std::mutex *mutex;
  std::map<std::vector<char>, httpproxy::CacheData> *cachingMap;
};

void *pthreadStartUpCacheCleaner(void *args) {
  CleanerThreadArgs *ca = static_cast<CleanerThreadArgs *>(args);
  cleanerFunction(ca->exitFuture, ca->revalidateTimeMillis, ca->cacheTime,
                  ca->mutex, ca->cachingMap);
  delete ca;
  return nullptr;
}

void *pthreadStartUpEvent(void *args) {
  EventThreadArgs *ea = static_cast<EventThreadArgs *>(args);
  eventFunction(ea->eventBlockingQueue, ea->mutex, ea->cachingMap);
  delete ea;
  return nullptr;
}

void httpproxy::HttpCacher::runThreads() {
  pthread_t cleanerThread;
  CleanerThreadArgs *cleanerArgs =
      new CleanerThreadArgs{&exitFuture, CACHE_TIMEOUT_MILLIS / 4,
                            CACHE_TIMEOUT_MILLIS, &mutex, &cachingMap};
  pthread_create(&cleanerThread, NULL, pthreadStartUpCacheCleaner, cleanerArgs);
  pthread_detach(cleanerThread);

  pthread_t eventThread;
  EventThreadArgs *eventThreadArgs =
      new EventThreadArgs{&eventQueue, &mutex, &cachingMap};
  pthread_create(&eventThread, NULL, pthreadStartUpEvent, eventThreadArgs);
  pthread_detach(eventThread);
}

void httpproxy::HttpCacher::cache(const std::vector<char> &request,
                                  const std::vector<char> &response) {
  eventQueue.push(
      CacheEvent{request, response, nullptr, CacheEventType::ANSWER});
}

bool httpproxy::HttpCacher::getCache(const std::vector<char> &request,
                                     std::vector<char> *response) {
  std::promise<CacheResult> cachePromise;
  auto cacheFuture = cachePromise.get_future();
  eventQueue.push(
      CacheEvent{request, {}, &cachePromise, CacheEventType::REQUEST});
  cacheFuture.wait();
  auto cacheResult = cacheFuture.get();
  if (cacheResult.hasCache) {
    response->resize(cacheResult.data.size());
    std::copy(cacheResult.data.begin(), cacheResult.data.end(),
              response->begin());
    return true;
  } else {
    return false;
  }
}

httpproxy::HttpCacher::~HttpCacher() {
  exitPromise.set_value();
  eventQueue.push({{}, {}, nullptr, CacheEventType::EXIT});
}
