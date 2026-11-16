#include "events.h"

#include <Arduino.h>
#include <atomic>
#include <freertos/queue.h>

#include "config_store.h"

static QueueHandle_t q;
static std::atomic<bool> dirty{false};

bool configDirtyTake() { return dirty.exchange(false); }

void eventsInit() { q = xQueueCreate(4, sizeof(UiEvent)); }
bool postUiEvent(const UiEvent& e) { return q && xQueueSend(q, &e, 0) == pdTRUE; }
bool popUiEvent(UiEvent& e) { return q && xQueueReceive(q, &e, 0) == pdTRUE; }

void onConfigChanged() {
  dirty = true;  // sticky: survives a full queue
  UiEvent e;
  e.kind = EventKind::ConfigChanged;
  postUiEvent(e);
}
