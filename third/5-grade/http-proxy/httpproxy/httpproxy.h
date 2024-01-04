/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#pragma once

#include <string>

namespace httpproxy {

class HttpProxy {
public:
  HttpProxy();
  ~HttpProxy();
  int listenAndServe(int port);

private:
  int serverSocketFD;
};

} // namespace httpproxy
