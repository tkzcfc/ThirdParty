#pragma once


// https://blog.csdn.net/albertsh/article/details/123978539
template<typename T, typename ...Args>
void GLog_Template(T& os, const Args &...args)
{
	(os << ... << args);
}

#include "easylogging++.h"
#define LogInfo()  LOG(INFO)
#define LogWarn()  LOG(WARNING)
#define LogError() LOG(ERROR)
#define LogDebug() LOG(DEBUG)
#define LogFatal() LOG(FATAL)


#define LOG_INFO(...)	GLog_Template(LOG(INFO), __VA_ARGS__)
#define LOG_WARN(...)	GLog_Template(LOG(WARNING), __VA_ARGS__)
#define LOG_ERROR(...)  GLog_Template(LOG(ERROR), __VA_ARGS__)
#define LOG_ERR(...)	GLog_Template(LOG(ERROR), __VA_ARGS__)
#define LOG_DEBUG(...)	GLog_Template(LOG(DEBUG), __VA_ARGS__)
#define LOG_FATAL(...)	GLog_Template(LOG(FATAL), __VA_ARGS__)
