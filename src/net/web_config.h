#pragma once

void webInit();  // net task: start the HTTP server on port 80 (works in STA and AP)
void webTick();  // net task: serve one pending request, run the delayed restart
