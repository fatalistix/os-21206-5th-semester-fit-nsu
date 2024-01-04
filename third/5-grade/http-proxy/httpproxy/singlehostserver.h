/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

#include "../httpparser/httprequestparser.h"
#include "../httpparser/httpresponseparser.h"
#include "../httpparser/request.h"
#include "../httpparser/response.h"
#include "./blockingqueue.h"
#include "./httpcacher.h"
#include <sys/socket.h>
#include <vector>

namespace httpproxy {

class SingleHostServer {
public:
  SingleHostServer(int fd, struct sockaddr clientAddress,
                   socklen_t clientAddressLength,
                   httpproxy::HttpCacher *cacher);
  ~SingleHostServer();
  int serve();

private:
  int clientFD;
  struct sockaddr clientAddress;
  socklen_t clientAddressLength;
  httpparser::HttpRequestParser requestParser;
  httpparser::HttpResponseParser responseParser;
  static const int BUFFER_LEN = 5 * 1024 * 1024;
  char buffer[BUFFER_LEN];
  httpproxy::HttpCacher *cacher;

  int sendResponseToClient(const httpparser::Response &response);
  int sendRequestToServer(const httpparser::Request &request,
                          const int serverFD);
  int readClientRequest(httpparser::Request *request,
                        httpparser::HttpRequestParser::ParseResult *result,
                        std::vector<char> *data);
  int readServerResponse(httpparser::Response *response,
                         httpparser::HttpResponseParser::ParseResult *result,
                         const int serverFD, std::vector<char> *data);
};

} // namespace httpproxy
