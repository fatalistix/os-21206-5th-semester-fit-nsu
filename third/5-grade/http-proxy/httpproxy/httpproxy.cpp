/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include "./httpproxy.h"
#include "./singlehostserver.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

static std::mutex mutex;
// static std::map<std::thread::id, std::thread *> createdThreads;
static std::set<pthread_t> createdThreads;

void signalHandler(int code) {
  std::cout << "HERE!!!" << std::endl;
  mutex.lock();

  for (auto &p : createdThreads) {
    pthread_cancel(p);
  }

  mutex.unlock();

  std::terminate();
}

httpproxy::HttpProxy::HttpProxy() : serverSocketFD(0) {
  signal(SIGINT, signalHandler);
}

httpproxy::HttpProxy::~HttpProxy() { close(serverSocketFD); }

void threadStartupFunction(int fd, struct sockaddr clientAddress,
                           socklen_t clientAddressLength,
                           httpproxy::HttpCacher *cacher) {
  int err;
  err = httpproxy::SingleHostServer(fd, clientAddress, clientAddressLength,
                                    cacher)
            .serve();
  std::cout << "STOPPED SERVING " << pthread_self() << std::endl;
  if (err) {
    std::cerr << "serve: " << strerror(errno) << std::endl;
  }
  mutex.lock();

  std::cout << "I AM UNDER LOCK <================>" << std::endl;

  createdThreads.erase(pthread_self());

  mutex.unlock();
}

struct pthreadArgs {
  std::future<void> starterFuture;
  int fd;
  struct sockaddr clientAddress;
  socklen_t clientAddressLength;
  httpproxy::HttpCacher *cacher;
};

void *pthreadStartup(void *arg) {
  pthreadArgs *a = reinterpret_cast<pthreadArgs *>(arg);
  a->starterFuture.wait();
  threadStartupFunction(a->fd, a->clientAddress, a->clientAddressLength,
                        a->cacher);
  delete a;
  std::cout << "exiting pthread" << std::endl;
  return NULL;
}

int httpproxy::HttpProxy::listenAndServe(int port) {
  this->serverSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocketFD == -1) {
    std::cerr << "socket: " << strerror(errno) << std::endl;
    return -1;
  }

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  int err;
  err =
      bind(serverSocketFD, reinterpret_cast<struct sockaddr *>(&serverAddress),
           sizeof(serverAddress));
  if (err) {
    std::cerr << "bind: " << strerror(errno) << std::endl;
    return -1;
  }

  err = listen(serverSocketFD, 1000);
  if (err) {
    std::cerr << "listen: " << strerror(errno) << std::endl;
    return -1;
  }

  struct sockaddr clientAddress;
  socklen_t clientAddressLength;

  std::cout << "HERE IN HTTPPROXY" << std::endl;

  auto cacher = new HttpCacher();
  cacher->runThreads();

  std::cout << "HERE IN HTTPPROXY" << std::endl;

  while (true) {
    int connFD = accept(serverSocketFD, &clientAddress, &clientAddressLength);
    if (connFD == -1) {
      std::cerr << "accept: " << strerror(errno) << std::endl;
      return -1;
    }

    std::promise<void> threadStarter;
    pthread_t t;

    auto arg = new pthreadArgs{threadStarter.get_future(), connFD,
                               clientAddress, clientAddressLength, cacher};

    err = pthread_create(&t, NULL, pthreadStartup, arg);
    if (err) {
      return -1;
    }

    mutex.lock();
    createdThreads.insert(t);
    mutex.unlock();
    pthread_detach(t);

    threadStarter.set_value();
  }
  std::cout << "a;sdlkfjadsl;fkjasdlkfjl;das jf I AM FINISHING "
               "<============================================================>"
            << std::endl;
}
