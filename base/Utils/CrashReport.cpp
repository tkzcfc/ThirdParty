#include "CrashReport.h"

#ifdef WIN32
#pragma warning(disable: 4996)
#include <windows.h>

#include <Dbghelp.h>

#include "time.h"

#pragma auto_inline (off)
#pragma comment( lib, "DbgHelp" )


LPTOP_LEVEL_EXCEPTION_FILTER g_preFilter;

std::string g_AppName;


long   __stdcall  CrashCallBack(_EXCEPTION_POINTERS* pExInfo)
{
	struct tm* pTime;
	time_t ctTime;
	time(&ctTime);
	pTime = localtime( &ctTime );
	TCHAR tem[256];
	memset(tem, 0, 256);
	snprintf(tem, 256, ("%s-%d-%d-%d_%d-%d-%d.dmp"), g_AppName.c_str(),
	         1900 + pTime->tm_year, 1 + pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);

	HANDLE hFile = ::CreateFile( tem, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION einfo;
		einfo.ThreadId = ::GetCurrentThreadId();
		einfo.ExceptionPointers = pExInfo;
		einfo.ClientPointers = FALSE;
		::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, &einfo, NULL, NULL);
		::CloseHandle(hFile);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

void  SetCrashReport(std::string strAppName)
{
	g_AppName = strAppName;
	g_preFilter = SetUnhandledExceptionFilter(CrashCallBack);
}

void UnSetCrashReport()
{
	SetUnhandledExceptionFilter(g_preFilter);

	return;
}


#else
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

std::string g_AppName;

void exceptionalStack(int signal)
{
	time_t nowtime;
	time(&nowtime);
	pid_t pid = getpid();
	char cmd[256];
	snprintf(cmd, 256, "gstack %d > core_%s_log_%d.core", pid, g_AppName.c_str(), nowtime);
	system(cmd);
	exit(-1);
}

void  SetCrashReport(std::string strAppName)
{
	g_AppName = strAppName;
	/*捕获异常信息 start*/
	signal(SIGABRT, &exceptionalStack); //异常终止(abort)
	signal(SIGBUS, &exceptionalStack); //硬件故障
	signal(SIGFPE, &exceptionalStack); //算术异常
	signal(SIGILL, &exceptionalStack); //非法硬件指令
	signal(SIGIOT, &exceptionalStack); //硬件故障
	signal(SIGQUIT, &exceptionalStack); //终端退出符
	signal(SIGSEGV, &exceptionalStack); //无效存储访问
	signal(SIGSYS, &exceptionalStack); //无效系统调用
	signal(SIGTRAP, &exceptionalStack); //硬件故障
	signal(SIGXCPU, &exceptionalStack); //超过CPU限制(setrlimit)
	signal(SIGXFSZ, &exceptionalStack); //超过文件长度限制(setrlimit)
	/*捕获异常信息 end*/

	setvbuf(stdout, NULL, _IONBF, 0);
	return ;
}

void UnSetCrashReport()
{
	return;
}

/*

sudo su

1. apt install gdb

2. vim /usr/bin/gstack
   write code

3. chmod 777 /usr/bin/gstack 
*/

//#!/bin/sh
//
//if test $#  - ne 1; then
//echo "Usage: `basename $0 .sh` <process-id>" 1 > &2
//exit 1
//fi
//
//if test !- r / proc / $1; then
//echo "Process $1 not found." 1 > &2
//exit 1
//fi
//
//# GDB doesn't allow "thread apply all bt" when the process isn't
//# threaded; need to peek at the process to determine if that or the
//# simpler "bt" should be used.
//
//backtrace = "bt"
//if test - d / proc / $1 / task; then
//# Newer kernel; has a task / directory.
//if test `/bin / ls / proc / $1 / task | /usr / bin / wc - l` - gt 1 2 > / dev / null; then
//backtrace = "thread apply all bt"
//fi
//elif test - f / proc / $1 / maps; then
//# Older kernel; go by it loading libpthread.
//if / bin / grep - e libpthread / proc / $1 / maps > / dev / null 2 > &1; then
//backtrace = "thread apply all bt"
//fi
//fi
//
//GDB = ${ GDB:-/ usr / bin / gdb }
//
//if $GDB - nx --quiet --batch --readnever > / dev / null 2 > &1; then
//readnever = --readnever
//else
//readnever =
//fi
//
//# Run GDB, strip out unwanted noise.
//$GDB --quiet $readnever - nx / proc / $1 / exe $1 << EOF 2 > &1 |
//set width 0
//set height 0
//set pagination no
//$backtrace
//EOF
/// bin / sed - n \
//- e 's/^\((gdb) \)*//' \
//- e '/^#/p' \
//- e '/^Thread/p'
//#end

#endif