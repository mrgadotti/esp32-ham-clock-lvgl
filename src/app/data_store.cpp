#include "data_store.h"

DataStore* g_storePtr = nullptr;
void dataStoreInit() { g_storePtr = new DataStore(); }
