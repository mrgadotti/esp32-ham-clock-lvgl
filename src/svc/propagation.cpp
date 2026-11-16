#include "svc/propagation.h"

#include <Arduino.h>
#include <time.h>

#include "app/data_store.h"
#include "net/http_fetch.h"
#include "net/time_service.h"
#include "propagation_xml.h"

constexpr uint32_t PROP_INTERVAL_MS = 60UL * 60UL * 1000UL;
constexpr uint32_t PROP_RETRY_MS = 5UL * 60UL * 1000UL;

static char s_body[8192];  // static: keeps 8 KB off the net task stack

void propTick(uint32_t now) {
  static bool first = true;
  static uint32_t nextAt;
  NetStatus n;
  g_store.net.read([&](const NetStatus& s) { n = s; });
  if (!n.sta || !timeValid()) return;
  if (!first && (int32_t)(now - nextAt) < 0) return;
  first = false;
  PropagationData p{};
  g_store.prop.read([&](const PropagationData& s) { p = s; });
  if (httpGet("https://www.hamqsl.com/solarxml.php", s_body, sizeof s_body, 8000) &&
      parsePropagationXml(s_body, p)) {
    p.fetchedAt = (uint32_t)time(nullptr);
    g_store.prop.write([&](PropagationData& d) { d = p; });
    nextAt = now + PROP_INTERVAL_MS;
  } else {
    nextAt = now + PROP_RETRY_MS;
  }
}
