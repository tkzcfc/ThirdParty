#pragma once

#if 1
#include "easylogging++.h"
#define LogInfo()  LOG(INFO)
#define LogWarn()  LOG(WARNING)
#define LogError() LOG(ERROR)
#define LogDebug() LOG(DEBUG)
#define LogFatal() LOG(FATAL)
#else
#include <iostream>
#define LogInfo() std::cout << "info:"
#define LogWarn() std::cout << "warn:"
#define LogError() std::cout << "error:(" << __FILE__ <<", "<< __LINE__<<", "<< __func__<< ")"
#define LogDebug() std::cout << "debug:"
#define LogFatal() std::cout << "fatal:"
#endif