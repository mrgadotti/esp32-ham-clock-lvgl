#pragma once
#include "types.h"

enum class EventKind : uint8_t { AprsAlert, Beaconing, WeatherAlert, ConfigChanged, StartTouchCal };
struct UiEvent {
  EventKind kind;
  union {
    AprsStation station;
    BeaconInfo beacon;
    WeatherAlertInfo wx;
  };
};

bool postUiEvent(const UiEvent&);  // any task; drops the event if the queue is full
bool popUiEvent(UiEvent&);         // UI task
void eventsInit();                 // create the queue (before starting tasks)
