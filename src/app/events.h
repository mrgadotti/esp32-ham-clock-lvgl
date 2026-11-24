#pragma once
#include "types.h"

enum class EventKind : uint8_t { AprsAlert, Beaconing, WeatherAlert, ConfigChanged, StartTouchCal, AprsMessage };
struct UiEvent {
  EventKind kind;
  union {
    AprsStation station;
    BeaconInfo beacon;
    WeatherAlertInfo wx;
    AprsMessage msg;
  };
};

bool postUiEvent(const UiEvent&);  // any task; drops the event if the queue is full
bool popUiEvent(UiEvent&);         // UI task
void eventsInit();                 // create the queue (before starting tasks)
bool configDirtyTake();               // UI task: true once per ConfigChanged burst, even if the event was dropped
