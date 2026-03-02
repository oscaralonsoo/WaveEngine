////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/DisplayLauncher.h>
#include <NsApp/Display.h>
#include <NsCore/Delegate.h>
#include <NsCore/CpuProfiler.h>
#include <NsCore/HighResTimer.h>
#include <NsApp/CommandLine.h>

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#include <excpt.h>
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
DisplayLauncher::DisplayLauncher(): mExitCode(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DisplayLauncher::~DisplayLauncher()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int DisplayLauncher::Run()
{
  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    __try {
  #endif

    {
        NS_PROFILE_CPU("Display/Init");
        Init();
        {
            NS_PROFILE_CPU("Display/Create");
            mDisplay = CreateDisplay();
        }
        {
            NS_PROFILE_CPU("Display/StartUp");
            OnStartUp();
        }
        {
            NS_PROFILE_CPU("Display/Show");
            mDisplay->Show();
        }
    }

    mDisplay->Render() += [this](Display*)
    {
        static uint64_t startTime = HighResTimer::Ticks();

        OnTick(HighResTimer::Seconds(HighResTimer::Ticks() - startTime));
        NS_PROFILE_CPU_FRAME;
    };

    bool runInBackground = GetRunInBackgroundOverride();
    mDisplay->EnterMessageLoop(runInBackground);

    {
        NS_PROFILE_CPU("Display/Close");
        OnExit();
        mDisplay.Reset();
    }

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    } __except (CrashHandler(GetExceptionInformation())) {}
  #endif

    return mExitCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayLauncher::Quit(int exitCode)
{
    mExitCode = exitCode;
    mDisplay->Close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Display* DisplayLauncher::GetDisplay() const
{
    return mDisplay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayLauncher::OnStartUp(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayLauncher::OnTick(double)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayLauncher::OnExit()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool DisplayLauncher::GetRunInBackgroundOverride() const
{
    return GetArguments().HasOption("runInBackground");
}
