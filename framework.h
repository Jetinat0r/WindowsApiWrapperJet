#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <strsafe.h>
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib") //Windows / VS doesn't automatically link Windows libraries :) Used for LoadIconMetric()
