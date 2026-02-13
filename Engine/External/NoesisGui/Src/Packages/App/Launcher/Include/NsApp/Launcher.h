////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LAUNCHER_H__
#define __APP_LAUNCHER_H__


#include <NsCore/Noesis.h>
#include <NsApp/LauncherApi.h>


namespace NoesisApp
{

class CommandLine;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for application launchers
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_LAUNCHER_API Launcher
{
public:
    Launcher();
    Launcher(const Launcher&) = delete;
    Launcher& operator=(const Launcher&) = delete;
    ~Launcher();

    /// Gets the current launcher object
    static Launcher* Current();

    /// Registers app framework components
    static void RegisterAppComponents();

    /// Sets command line arguments
    static void SetArguments(int argc, char** argv);

    /// Retrieves command line arguments
    static const CommandLine& GetArguments();

    /// Enables the crash reporter for the current application. The crash reporter runs
    /// in a separate process and shows a dialog before sending the dump. If 'quiet' is
    /// set to true, the dump is sent quietly without showing any dialog.
    static void EnableCrashReporter(const char* appName, bool quiet);

protected:
    /// Launcher initialization
    void Init() const;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    uint32_t CrashHandler(void* info);
  #endif

private:
    /// Registers application components
    virtual void RegisterComponents() const;
};

}

#endif
