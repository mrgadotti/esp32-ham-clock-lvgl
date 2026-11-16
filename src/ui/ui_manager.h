#pragma once
#include "screen.h"
#include "../app/events.h"

void uiInit();             // UI task, after cfgRefreshUiSnapshot(): gesture sink, boot screen
void uiTick(uint32_t nowMs);  // UI task: call every loop pass; screens tick at 100 ms
void uiHandleEvent(const UiEvent&);  // UI task: ConfigChanged, StartTouchCal, alert overlays
// Request a change; applied by uiTick, never inside an event callback.
void uiRequest(ScreenId);
void uiNext();
void uiPrev();
void uiOpenModal(Screen*);  // takes ownership; shown full-screen on lv_layer_top
void uiCloseModal();
bool uiModalActive();  // a modal is open or queued
