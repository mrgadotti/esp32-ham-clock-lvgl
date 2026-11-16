#include "events.h"

#include <Arduino.h>
#include <freertos/queue.h>

#include "config_store.h"

static QueueHandle_t q;

void eventsInit() { q = xQueueCreate(4, sizeof(UiEvent)); }
bool postUiEvent(const UiEvent& e) { return q && xQueueSend(q, &e, 0) == pdTRUE; }
bool popUiEvent(UiEvent& e) { return q && xQueueReceive(q, &e, 0) == pdTRUE; }

void onConfigChanged() {
  UiEvent e;
  e.kind = EventKind::ConfigChanged;
  postUiEvent(e);
}
