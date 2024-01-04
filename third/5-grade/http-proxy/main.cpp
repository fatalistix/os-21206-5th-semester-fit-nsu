/*
 * Copyright (c) 2023, Balashov Vyacheslav
 */

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>

#include "httpproxy/httpproxy.h"

int main() {
  httpproxy::HttpProxy().listenAndServe(8069);
  std::cerr << "Proxy: " << strerror(errno) << std::endl;
  return 0;
}
