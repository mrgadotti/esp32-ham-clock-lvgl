#include "svc/async_connect.h"

#include <Arduino.h>
#include <fcntl.h>
#include <lwip/dns.h>
#include <lwip/sockets.h>
#include <string.h>

namespace {
constexpr uint32_t kDnsMs = 5000, kConnectMs = 4000;

void onDns(const char* name, const ip_addr_t* addr, void* arg) {
  auto* self = static_cast<AsyncConnect*>(arg);
  if (strcmp(name, self->host_) != 0) return;  // late answer for an abandoned lookup of another host
  if (addr) self->ip_ = ip_addr_get_ip4_u32(addr);
  self->dnsState_ = addr ? 1 : 2;
}
}  // namespace

void AsyncConnect::cancel() {
  if (fd_ >= 0) close(fd_);
  fd_ = -1;
  st_ = Idle;
}

void AsyncConnect::start(const char* host, uint16_t port, uint32_t now) {
  cancel();
  strlcpy(host_, host, sizeof host_);
  port_ = port;
  t0_ = now;
  dnsState_ = 0;
  IPAddress lit;
  if (lit.fromString(host_)) {
    ip_ = (uint32_t)lit;
    connectNow(now);
    return;
  }
  ip_addr_t a;
  err_t e = dns_gethostbyname_addrtype(host_, &a, onDns, this, LWIP_DNS_ADDRTYPE_IPV4);  // same call as hostByName()
  if (e == ERR_OK) {
    ip_ = ip_addr_get_ip4_u32(&a);
    connectNow(now);
  } else {
    st_ = e == ERR_INPROGRESS ? Dns : Failed;
  }
}

void AsyncConnect::connectNow(uint32_t now) {
  t0_ = now;
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) { st_ = Failed; return; }
  fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) | O_NONBLOCK);
  sockaddr_in sa = {};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_);
  sa.sin_addr.s_addr = ip_;
  int r = connect(fd_, (sockaddr*)&sa, sizeof sa);
  if (r < 0 && errno != EINPROGRESS) { cancel(); st_ = Failed; return; }
  st_ = Connecting;
}

AsyncConnect::State AsyncConnect::poll(uint32_t now, WiFiClient& out) {
  if (st_ == Dns) {
    if (dnsState_ == 1) connectNow(now);
    else if (dnsState_ == 2 || now - t0_ >= kDnsMs) st_ = Failed;
  }
  if (st_ == Connecting) {
    fd_set w;
    FD_ZERO(&w);
    FD_SET(fd_, &w);
    timeval tv = {0, 0};
    int r = select(fd_ + 1, nullptr, &w, nullptr, &tv);
    int err = 0;
    socklen_t len = sizeof err;
    if (r > 0) st_ = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0 ? Done : Failed;
    else if (r < 0 || now - t0_ >= kConnectMs) st_ = Failed;
  }
  State s = st_;
  if (s == Done) {
    fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) & ~O_NONBLOCK);  // WiFiClient expects a blocking fd, as after connect()
    out = WiFiClient(fd_);                                        // takes ownership of the fd
    fd_ = -1;
    st_ = Idle;
  } else if (s == Failed) {
    cancel();
  }
  return s;
}
