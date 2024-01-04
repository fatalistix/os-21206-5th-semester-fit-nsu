/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include "./singlehostserver.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <libwebsockets.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

httpproxy::SingleHostServer::SingleHostServer(int fd,
                                              struct sockaddr clientAddress,
                                              socklen_t clientAddressLength,
                                              httpproxy::HttpCacher *cacher)
    : clientFD(fd), clientAddress(clientAddress),
      clientAddressLength(clientAddressLength), cacher(cacher) {}

httpproxy::SingleHostServer::~SingleHostServer() { close(clientFD); }

bool equalIgnoreCase(const std::string &first, const std::string &second) {
  if (first.size() != second.size()) {
    return false;
  }
  for (int i = 0; i < first.size(); ++i) {
    if (std::tolower(first[i]) != std::tolower(second[i])) {
      return false;
    }
  }
  return true;
}

void printRequest(const httpparser::Request &r) {
  std::cout << "<<================REQUEST:================>>" << std::endl;
  std::cout << "HTTP " << r.versionMajor << "." << r.versionMinor << std::endl;
  std::cout << "METHOD: " << r.method << std::endl;
  std::cout << "KEEP ALIVE" << r.keepAlive << std::endl;
  std::cout << "URI: " << r.uri << std::endl;
  std::cout << "HEADERS:" << std::endl;
  for (auto &h : r.headers) {
    std::cout << h.name << ": " << h.value << std::endl;
  }
  if (r.content.data() == nullptr) {
    std::cout << "CONTENT is empty" << std::endl;
  } else {
    std::cout << "CONTENT: " << std::string(r.content.data()) << std::endl;
  }
}

void printResponse(const httpparser::Response &r) {
  std::cout << "<<----------------RESPONSE:--------------->>" << std::endl;
  std::cout << "HTTP " << r.versionMajor << "." << r.versionMinor << std::endl;
  std::cout << "STATUS: " << r.statusCode << " - " << r.status << std::endl;
  std::cout << "KEEP ALIVE" << r.keepAlive << std::endl;
  std::cout << "HEADERS:" << std::endl;
  for (auto &h : r.headers) {
    std::cout << h.name << ": " << h.value << std::endl;
  }
  if (r.content.data() == nullptr) {
    std::cout << "CONTENT is empty" << std::endl;
  } else {
    std::cout << "CONTENT: " << std::string(r.content.data()) << std::endl;
  }
}

bool getRequestHeaderValue(const std::string &headerName, std::string *value,
                           const httpparser::Request &request) {
  bool hostHeaderFound = false;
  for (auto &h : request.headers) {
    if (equalIgnoreCase(h.name, headerName)) {
      hostHeaderFound = true;
      *value = h.value;
      return true;
    }
  }
  return false;
}

int openConnectionWithServer(const std::string &targetHost, int port) {
  int err;

  int serverFD;

  struct addrinfo hints;
  struct addrinfo *result;
  struct addrinfo *rp;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  err = getaddrinfo(targetHost.data(), NULL, &hints, &result);
  if (err) {
    std::cerr << "single host server: getaddrinfo: " << strerror(errno)
              << std::endl;
    return -1;
  }

  std::cout << "got address info" << std::endl;

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    serverFD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    std::cout << "AFTER SOCKET" << std::endl;
    if (serverFD == -1) {
      continue;
    }
    std::cout << "SOCKETED SUCCESSFULLY" << std::endl;

    if (rp->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
      ipv4->sin_port = htons(port);
    } else {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
      ipv6->sin6_port = htons(port);
    }

    err = connect(serverFD, rp->ai_addr, rp->ai_addrlen);
    std::cout << "ERR FROM CONNECT = " << err << std::endl;
    if (!err) {
      std::cout << "CONNECTED SUCESSFULLY" << std::endl;
      freeaddrinfo(result);
      return serverFD;
    }

    std::cout << "CONNECT FAILED" << std::endl;

    err = close(serverFD);
    if (err) {
      freeaddrinfo(result);
      return -1;
    }
  }
  freeaddrinfo(result);
  return -1;
}

void fillResponse(
    httpparser::Response *response, int versionMajor, int versionMinor,
    bool keepAlive, int statusCode, const std::string &status,
    const std::vector<std::pair<std::string, std::string>> &headers,
    const std::vector<char> &content) {
  response->versionMajor = versionMajor;
  response->versionMinor = versionMinor;
  response->keepAlive = keepAlive;
  response->status = status;
  response->statusCode = statusCode;
  response->headers.clear();
  for (auto &h : headers) {
    response->headers.emplace_back(h.first, h.second);
  }
  response->content = content;
}

void fillRequest(
    httpparser::Request *request, int versionMajor, int versionMinor,
    bool keepAlive, const std::string &uri,
    const std::vector<std::pair<std::string, std::string>> &headers,
    const std::vector<char> &content, const std::string &method) {
  request->versionMajor = versionMajor;
  request->versionMinor = versionMinor;
  request->keepAlive = keepAlive;
  request->uri = uri;
  request->content = content;
  request->method = method;
  for (auto &h : headers) {
    request->headers.emplace_back(h.first, h.second);
  }
}

void splitHostAndPort(const std::string &address, std::string *host,
                      int *port) {
  for (int i = 0; i < address.size(); ++i) {
    if (!isdigit(address[address.size() - 1 - i])) {
      if (address[address.size() - i - 1] == ':' && i) {
        *host = address.substr(0, address.size() - 1 - i);
        *port = 0;
        for (int j = address.size() - i; j < address.size(); ++j) {
          *port = *port * 10 + address[j] - '0';
        }
        return;
      } else {
        *host = address;
        *port = 0;
      }
      return;
    }
  }
}

int writeAllBytes(const std::vector<char> &data, int fd);

int httpproxy::SingleHostServer::serve() {
  httpparser::Request clientRequest;
  httpparser::HttpRequestParser::ParseResult clientParseResult;

  httpparser::Response response;
  httpparser::HttpResponseParser::ParseResult responseParseResult;

  int err;

  std::vector<char> responseData(0);
  std::vector<char> requestData(0);

  err =
      this->readClientRequest(&clientRequest, &clientParseResult, &requestData);
  if (err) {
    std::cerr << "single host server: readCLientRequest: " << strerror(errno)
              << std::endl;
    return -1;
  }

  if (clientParseResult == httpparser::HttpRequestParser::ParsingError) {
    errno = EPROTO;
    std::cerr << "single host server: parse: " << strerror(errno) << std::endl;
    return -1;
  }

  bool isGetRequest = equalIgnoreCase(clientRequest.method, "get");
  bool cacheFound =
      isGetRequest && this->cacher->getCache(requestData, &responseData);
  if (cacheFound) {
    err = writeAllBytes(responseData, clientFD);
    if (err) {
      std::cerr << "single host server: send request to server: "
                << strerror(errno) << std::endl;
      return -1;
    }
    return 0;
  }

  std::string serverAddress;
  bool hostHeaderFound =
      getRequestHeaderValue("host", &serverAddress, clientRequest);

  if (!hostHeaderFound) {
    std::vector<std::pair<std::string, std::string>> headers;
    for (auto &h : clientRequest.headers) {
      if (equalIgnoreCase(h.name, "host")) {
        continue;
      }
      headers.emplace_back(h.name, h.value);
    }

    fillResponse(&response, clientRequest.versionMajor,
                 clientRequest.versionMinor, false, 400, "BAD REQUEST", headers,
                 std::vector<char>());
    this->sendResponseToClient(response);
    errno = EPROTO;
    std::cerr << "single host server: host header not found" << std::endl;
    return -1;
  }

  int serverPort = 0;
  std::string serverHost;
  std::cout << "SPLITTING: " << serverAddress << std::endl;
  splitHostAndPort(serverAddress, &serverHost, &serverPort);
  if (serverPort == 0) {
    serverPort = 80;
  }

  std::cout << "TRYING TO CONNECT TO: " << serverHost << ' ' << serverPort
            << std::endl;
  int serverFD = openConnectionWithServer(serverHost, serverPort);

  err = writeAllBytes(requestData, serverFD);
  if (err) {
    std::cerr << "single host server: send request to server: "
              << strerror(errno) << std::endl;
    return -1;
  }

  err = this->readServerResponse(&response, &responseParseResult, serverFD,
                                 &responseData);
  if (err) {
    std::cerr << "single host server: read response from server: "
              << strerror(errno) << std::endl;
    return -1;
  }

  // if (serverParseResult == httpparser::HttpResponseParser::ParsingError) {
  //   errno = EPROTO;
  //   std::cerr << "single host server: parse: " << strerror(errno) <<
  //   std::endl; return -1;
  // }

  err = writeAllBytes(responseData, this->clientFD);
  if (err) {
    std::cerr << "single host server: send response to client: "
              << strerror(errno) << std::endl;
    return -1;
  }

  if (isGetRequest) {
    cacher->cache(requestData, responseData);
  }

  return 0;
}

int writeAllBytes(const std::vector<char> &data, int fd) {
  int err;
  int wroteBytes;
  int totallyWroteBytes = 0;

  while (totallyWroteBytes != data.size()) {
    wroteBytes = send(fd, data.data() + totallyWroteBytes,
                      data.size() - totallyWroteBytes, MSG_NOSIGNAL);
    if (wroteBytes == -1) {
      std::cerr << "single host server: send request to client: write: "
                << strerror(errno) << std::endl;
      return -1;
    }
    totallyWroteBytes += wroteBytes;
    std::cout << "TOTALLY WROTE BYTES: " << totallyWroteBytes << " OF "
              << data.size() << " PID: " << pthread_self() << std::endl;
  }
  std::cout << " MAYBER HERE INFO POINT" << std::endl;
  return 0;
}

int httpproxy::SingleHostServer::sendRequestToServer(
    const httpparser::Request &request, const int serverFD) {
  // return writeAllBytes(request.inspect(), serverFD);
  return 0;
}

int httpproxy::SingleHostServer::sendResponseToClient(
    const httpparser::Response &response) {
  // return writeAllBytes(response.inspect(), this->clientFD);
  return 0;
}

int httpproxy::SingleHostServer::readClientRequest(
    httpparser::Request *request,
    httpparser::HttpRequestParser::ParseResult *result,
    std::vector<char> *data) {
  int readBytes;
  int totallyReadBytes = 0;
  data->clear();
  do {
    readBytes = recv(clientFD, buffer, BUFFER_LEN, 0);
    if (readBytes == -1) {
      std::cerr << "single host server: recv: " << strerror(errno) << ' '
                << errno << std::endl;
      return -1;
    }
    data->resize(totallyReadBytes + readBytes);
    std::copy(buffer, buffer + readBytes, data->data() + totallyReadBytes);
    totallyReadBytes += readBytes;
    *result = requestParser.parse(*request, data->data(),
                                  data->data() + totallyReadBytes);
  } while (*result == httpparser::HttpRequestParser::ParsingIncompleted);
  // std::cout << "CLIENT REQUEST READ: " << data->size()
  // << " BYTES which end with " << data->back() << std::endl;
  return 0;
}

int httpproxy::SingleHostServer::readServerResponse(
    httpparser::Response *response,
    httpparser::HttpResponseParser::ParseResult *result, const int serverFD,
    std::vector<char> *data) {
  int readBytes;
  int totallyReadBytes = 0;
  data->clear();
  do {
    readBytes = recv(serverFD, buffer, BUFFER_LEN, 0);
    if (readBytes == -1) {
      std::cerr << "single host server: recv: " << strerror(errno) << std::endl;
      return -1;
    }
    data->resize(totallyReadBytes + readBytes);
    std::copy(buffer, buffer + readBytes, data->data() + totallyReadBytes);
    totallyReadBytes += readBytes;
    std::cout << "READ BYTES FROM SERVER: " << totallyReadBytes << std::endl;
  } while (readBytes);
  return 0;
}
