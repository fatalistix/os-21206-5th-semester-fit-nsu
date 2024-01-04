#include <arpa/inet.h>
#include <cstring>
#include <future>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

std::future<int> test() {
  std::promise<int> x;
  std::thread([&x]() {
    sleep(7);
    x.set_value(100);
  }).detach();
  return x.get_future();
}

int main() {
  auto c = test();
  c.wait();
  std::cout << "I HAVE WAITED FOR: " << c.get() << std::endl;

  struct addrinfo hints;
  struct addrinfo *res;
  struct addrinfo *p;

  std::memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo("fit.ippolitov.me", NULL, &hints, &res);
  if (status) {
    perror("getaddrinfo");
    return -1;
  }

  char ip[INET6_ADDRSTRLEN];

  for (p = res; p != NULL; p = p->ai_next) {
    void *addr;
    std::string ipver;

    if (p->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      ipver = "IPV4";

      int sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sfd == -1) {
        std::cout << "SDF IS -1" << strerror(errno) << std::endl;
        continue;
      }

      ipv4->sin_port = htons(80);

      int err = connect(sfd, (struct sockaddr *)ipv4, p->ai_addrlen);
      if (err) {
        std::cout << "err IS -1" << strerror(errno) << std::endl;
        continue;
      }

    } else {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPV6";
    }
    inet_ntop(p->ai_family, addr, ip, sizeof(ip));
    std::cout << std::string(ip) << std::endl;
  }

  return 0;
}
