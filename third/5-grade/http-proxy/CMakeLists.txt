cmake_minimum_required(VERSION 3.27)
project(httpproxy)

set(CMAKE_CXX_STANDARD 20)

add_executable(
  httpproxy
  main.cpp
  httpproxy/httpproxy.cpp
  httpproxy/httpproxy.h
  httpproxy/singlehostserver.cpp
  httpproxy/singlehostserver.h
  httpparser/httprequestparser.h
  httpparser/httpresponseparser.h
  httpparser/request.h
  httpparser/response.h
  httpparser/urlparser.h
  httpproxy/httpcacher.h
  httpproxy/httpcacher.cpp
  httpproxy/blockingqueue.h)
