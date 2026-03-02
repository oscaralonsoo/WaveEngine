////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/Launcher.h>
#include <NsCore/Init.h>
#include <NsCore/Error.h>
#include <NsCore/Log.h>
#include <NsCore/UTF8.h>
#include <NsCore/Version.h>
#include <NsCore/StringUtils.h>
#include <NsCore/HighResTimer.h>
#include <NsApp/CommandLine.h>

#include <stdio.h>


#ifdef NS_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>
  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    NS_WARNING_PUSH
    NS_MSVC_WARNING_DISABLE(4091)
      #include <Dbghelp.h>
    NS_WARNING_POP
    #include <Shlobj.h>
    #include <signal.h>
    #include <eh.h>
    #include <wchar.h>

    static HMODULE Dbghelp;
    typedef BOOL (WINAPI *PFN_MINIDUMPWRITEDUMP)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION);
    static PFN_MINIDUMPWRITEDUMP MiniDumpWriteDump_;

    static PEXCEPTION_POINTERS gExceptionInfo;
    static DWORD gThreadID;
    static WCHAR gLogFilename[MAX_PATH];
    static FILE* gLogFile;

    static void Terminator()
    {
        *((int*)0) = 13;
    }

    static void SignalHandler(int)
    {
        Terminator();
    }

    static void __cdecl InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*,
        unsigned int, uintptr_t)
    {
       Terminator();
    }
  #endif
#endif

#ifdef NS_PLATFORM_ANDROID
    #include <android/log.h>
#endif

#ifdef NS_PLATFORM_EMSCRIPTEN
    #include <emscripten.h>
#endif

#ifdef NS_APP_FRAMEWORK
    extern "C" void NsRegisterReflection_NoesisApp();
    extern "C" void NsInitPackages_NoesisApp();
    extern "C" void NsShutdownPackages_NoesisApp();
#endif


using namespace Noesis;
using namespace NoesisApp;


static Launcher* gInstance;
static CommandLine gCommandLine;
static char gCrashReporterAppName[128];
static bool gCrashReporterQuiet;


////////////////////////////////////////////////////////////////////////////////////////////////////
Launcher::Launcher()
{
    NS_ASSERT(gInstance == 0);
    gInstance = this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Launcher::~Launcher()
{
  #ifdef NS_APP_FRAMEWORK
    NsShutdownPackages_NoesisApp();
  #endif

    Noesis::Shutdown();

    NS_ASSERT(gInstance != 0);
    gInstance = 0;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    if (Dbghelp != 0)
    {
        FreeLibrary(Dbghelp);
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Launcher* Launcher::Current()
{
    NS_ASSERT(gInstance != 0);
    return gInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Launcher::RegisterAppComponents()
{
  #ifdef NS_APP_FRAMEWORK
    NsRegisterReflection_NoesisApp();
    NsInitPackages_NoesisApp();
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Launcher::SetArguments(int argc, char** argv)
{
    gCommandLine = CommandLine(argc, argv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const CommandLine& Launcher::GetArguments()
{
    return gCommandLine;
}

#ifdef NS_PLATFORM_WINDOWS_DESKTOP

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool GetTmpPath(const WCHAR* base, const WCHAR* ext, WCHAR* out, uint32_t size)
{
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, out)))
    {
        wcscat_s(out, size, L"\\Temp");
        CreateDirectoryW(out, nullptr);

        wcscat_s(out, size, L"\\Noesis");
        CreateDirectoryW(out, nullptr);

        wcscat_s(out, size, L"\\Dumps");
        CreateDirectoryW(out, nullptr);

        wcscat_s(out, size, L"\\");
        wcscat_s(out, size, base);
        wcscat_s(out, size, ext);

        return true;
    }

    return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
void Launcher::EnableCrashReporter(const char* appName, bool quiet)
{
    StrCopy(gCrashReporterAppName, sizeof(gCrashReporterAppName), appName);
    gCrashReporterQuiet = quiet;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    if (!StrIsNullOrEmpty(gCrashReporterAppName))
    {
        // Keep writing the log into a temp file in case we need to send it to the crash reporter
        // Multiple instances of the application might be running simultaneously
        // We iterate through possible filenames (log.0.txt to log.31.txt) to find an available slot

        for (uint32_t i = 0; i < 32; i++)
        {
            WCHAR base[32];
            wsprintf(base, L"log.%d", i);

            if (GetTmpPath(base, L".txt", gLogFilename, NS_COUNTOF(gLogFilename)))
            {
                _wfopen_s(&gLogFile, (wchar_t*)gLogFilename, L"wb");
                if (gLogFile != 0) break;
            }
        }

        if (MiniDumpWriteDump_ == 0)
        {
            Dbghelp = LoadLibraryA("Dbghelp.dll");
            MiniDumpWriteDump_ = (PFN_MINIDUMPWRITEDUMP)GetProcAddress(Dbghelp, "MiniDumpWriteDump");
        }

        signal(SIGABRT, SignalHandler);
        set_terminate(&Terminator);
        set_unexpected(&Terminator);
        _set_abort_behavior(0, _WRITE_ABORT_MSG|_CALL_REPORTFAULT);
        _set_purecall_handler(&Terminator);
        _set_invalid_parameter_handler(&InvalidParameterHandler);
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Launcher::Init() const
{
  #if NS_MINIMUM_LOG_LEVEL != 0xFFFF
    SetLogHandler([](const char*, uint32_t, uint32_t level, const char* channel, const char* msg)
    {
        // If channel is not main ("") filter out debug and info messages
        bool filter = !StrIsEmpty(channel) && level < NS_LOG_LEVEL_WARNING;

        // Use --log_binding to also enable debug and info for bindings channel
        if (gCommandLine.HasOption("log_binding") && StrEquals(channel, "Binding"))
        {
            filter = false;
        }

        if (!filter)
        {
            level = level > NS_LOG_LEVEL_ERROR ? NS_LOG_LEVEL_ERROR : level;

          #if defined(NS_PLATFORM_ANDROID)
            const int priority[] = { ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG, ANDROID_LOG_INFO,
                ANDROID_LOG_WARN, ANDROID_LOG_ERROR};
            (void)__android_log_print(priority[level], "Noesis", "%s", msg);

          #elif defined(NS_PLATFORM_EMSCRIPTEN)
            const int flags[] = { 0, 0, 0, EM_LOG_WARN, EM_LOG_ERROR };
            constexpr const char* prefixes[] = { "T", "D", "I", "W", "E" };
            emscripten_log(EM_LOG_CONSOLE | flags[level], "[NOESIS/%s] %s", prefixes[level], msg);

          #elif defined(NS_PLATFORM_WINDOWS)
            OutputDebugStringA("[NOESIS/");
            constexpr const char* prefixes[] = { "T", "D", "I", "W", "E" };
            OutputDebugStringA(prefixes[level]);
            OutputDebugStringA("] ");
            OutputDebugStringA(msg);
            OutputDebugStringA("\n");

          #else
            constexpr const char* prefixes[] = { "T", "D", "I", "W", "E" };
            printf("[NOESIS/%s] %s\n", prefixes[level], msg);
          #endif

          #ifdef NS_PLATFORM_WINDOWS_DESKTOP
            if (gLogFile)
            {
                static uint64_t Start = HighResTimer::Ticks();
                double ms = HighResTimer::Milliseconds(HighResTimer::Ticks() - Start);

                int h = (int)(ms / 3600000);
                ms -= h * 3600000;

                int m = (int)(ms / 60000);
                ms -= m * 60000;

                int s = (int)(ms / 1000);
                ms -= (s * 1000);

                constexpr const char* L[] = { "T", "D", "I", "W", "E" };
                fprintf(gLogFile, "%02d:%02d:%02d.%03.0f |%s| %s\n", h, m, s, ms, L[level], msg);
            }
          #endif
        }
    });
  #endif

    Noesis::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);

    Noesis::Init();
    RegisterAppComponents();
    RegisterComponents();
}

#ifdef NS_PLATFORM_WINDOWS_DESKTOP

////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD WINAPI ReportFunc(LPVOID)
{
    SYSTEMTIME t;
    GetLocalTime(&t);

    WCHAR base[32];
    wsprintfW(base, L"%4d.%02d.%02d_%02d.%02d.%02d.%03d", t.wYear, t.wMonth, t.wDay, t.wHour,
        t.wMinute, t.wSecond, t.wMilliseconds);

    WCHAR dump[MAX_PATH];
    if (!GetTmpPath(base, L".dmp", dump, NS_COUNTOF(dump)))
    {
        return 0;
    }

    HANDLE file = CreateFileW(dump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);

    if (file)
    {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ClientPointers = true;
        mdei.ThreadId = gThreadID;
        mdei.ExceptionPointers = gExceptionInfo;

        HANDLE process = GetCurrentProcess();
        DWORD processId = GetCurrentProcessId();

        MINIDUMP_TYPE mdt = MiniDumpWithIndirectlyReferencedMemory;

        BOOL r = MiniDumpWriteDump_(process, processId, file, mdt, &mdei, NULL, NULL);
        CloseHandle(file);

        if (r)
        {
            // Executable
            WCHAR exe[8192];
            GetModuleFileNameW(NULL, exe, NS_COUNTOF(exe));

            wchar_t* backslash = wcsrchr(exe, '\\');
            if (backslash) *backslash = 0;

            wcscat_s(exe, L"\\App.CrashReporter.exe");

            // Parameters
            WCHAR params[1024]{};

            WCHAR application[128];
            UTF8::UTF8To16(gCrashReporterAppName, (uint16_t*)application, NS_COUNTOF(application));

            WCHAR version[128];
            UTF8::UTF8To16(GetBuildVersion(), (uint16_t*)version, NS_COUNTOF(version));

            wcscat_s(params, L"--application ");
            wcscat_s(params, application);
            wcscat_s(params, L" --version ");
            wcscat_s(params, version);
            wcscat_s(params, L" --dump \"");
            wcscat_s(params, dump);
            wcscat_s(params, L"\"");

            if (gLogFile)
            {
                fflush(gLogFile);
                fclose(gLogFile);

                WCHAR log[MAX_PATH];
                if (GetTmpPath(base, L".txt", log, NS_COUNTOF(log)))
                {
                    _wrename(gLogFilename, log);
                    wcscat_s(params, L" --log \"");
                    wcscat_s(params, log);
                    wcscat_s(params, L"\"");
                }
            }

            if (gCrashReporterQuiet)
            {
                wcscat_s(params, L" --quiet");
            }

            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi = {};

            // Launch the CrashReporter and send the minidump to the server
            if (!CreateProcessW(exe, params, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
              #if 0
                // Something went wrong, log it..
                WCHAR log[MAX_PATH];
                GetTmpPath(L"CreateProcess", L".log", log, NS_COUNTOF(log));

                FILE* fp = 0;
                _wfopen_s(&fp, (wchar_t*)log, L"wb");

                if (fp)
                {
                    fwprintf(fp, L"%s %s -> 0x%x", exe, params, GetLastError());
                    fflush(fp);
                    fclose(fp);
                }
              #endif
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Launcher::CrashHandler(void* info_)
{
    if (MiniDumpWriteDump_)
    {
        PEXCEPTION_POINTERS info = (PEXCEPTION_POINTERS)info_;

        // Launch the handler in a separate thread to properly process stack overflow exceptions.
        // Ensure the handler is not re-entered, which could happen if a crash occurs inside

        if (gExceptionInfo == 0)
        {
            gExceptionInfo = info;
            gThreadID = GetCurrentThreadId();

            HANDLE h = CreateThread(NULL, 0, ReportFunc, NULL, 0, NULL);
            WaitForSingleObject(h, INFINITE);
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
void Launcher::RegisterComponents() const
{
}
