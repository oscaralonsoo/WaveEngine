# GNU Make project for 'NoesisApp'

# Parallel builds can be enabled using the environment variable MAKEFLAGS (eg: MAKEFLAGS="-j 4")
# For parallel execution in Windows, Make 4.0 or better is required

CC = emcc
CXX = em++
AR = emar

CFLAGS_COMMON = -pipe -Wall -Werror -Wfatal-errors -Wtype-limits -Wtype-limits -Wnested-anon-types -msimd128
CXXFLAGS_COMMON = $(CFLAGS_COMMON) -std=c++11 -fno-exceptions -frtti -fno-threadsafe-statics
LDFLAGS_COMMON = -s USE_WEBGL2=1 -s NO_FILESYSTEM=1 -s WASM=1 -s STACK_SIZE=1MB -s INITIAL_MEMORY=64MB
SHADERFLAGS_COMMON = -I..\..\..\Packages\Render -I..\..\..\Tools\ShaderCompiler\Include -lang=essl

SHELLTYPE = msdos
ifeq (, $(ComSpec)$(COMSPEC))
  SHELLTYPE = posix
endif
ifeq (/bin, $(findstring /bin, $(SHELL)))
  SHELLTYPE = posix
endif
ifeq (/bin, $(findstring /bin, $(MAKESHELL)))
  SHELLTYPE = posix
endif

ifeq (posix, $(SHELLTYPE))
  DEVNUL = /dev/null
  WHICH = which
  MKDIR = mkdir -p "$(1)"
  BIN2H = ../../../Tools/Bin2h/bin2h.py
  SHADERC = ../../../Tools/ShaderCompiler/Bin/Linux/ShaderCompiler
  CP = cp "$(1)" "$(2)"
  MV = mv "$(1)" "$(2)"
  RM = rm -f "$(1)"
  RMTREE = rm -rf "$(1)"
  FIXSLASHES = 
else
  DEVNUL = NUL
  WHICH = where
  MKDIR = mkdir "$(subst /,\,$(1))" 2> nul || exit 0
  BIN2H = ..\..\..\Tools\Bin2h\bin2h.exe
  SHADERC = ..\..\..\Tools\ShaderCompiler\Bin\Windows\ShaderCompiler.exe
  CP = copy /Y "$(subst /,\,$(1))" "$(subst /,\,$(2))" > nul
  MV = move /Y "$(subst /,\,$(1))" "$(subst /,\,$(2))" > nul
  RM = del /Q "$(subst /,\,$(1))" 2> nul || exit 0
  RMTREE = rmdir /S /Q "$(subst /,\,$(1))" 2> nul || exit 0
  SED = ..\..\..\Tools\sed\sed.exe
  FIXSLASHES = $(SED) -i "s/\(\S\)\\\(\S\)/\1\/\2/g" "$(1)"
endif

ifeq (, $(shell ${WHICH} $(CC) 2> $(DEVNUL) ))
$(error $(CC) not found in PATH)
endif

ifeq (, $(shell ${WHICH} $(CXX) 2> $(DEVNUL) ))
$(error $(CXX) not found in PATH)
endif

ifeq ($(V), 1)
  Q =
else
  Q = @
endif

ifndef CONFIG
  CONFIG = Release
endif

ifeq ($(CONFIG), Debug)
  TARGET_DIR = ../../../../Bin/wasm
  TARGET = ../../../../Bin/wasm/NoesisApp.a
  INTERMEDIATE = build/Debug
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/ApplicationLauncher/Include -I../../../Packages/App/DisplayLauncher/Include -I../../../Packages/App/Display/Include -I../../../Packages/App/Interactivity/Include -I../../../Packages/App/LangServerHelpers/Include -I../../../Packages/App/LangServer/Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/MediaElement/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -I../../../Packages/Render/RenderContext/Include -Ibuild/Debug
  DEFINES = -DNS_BUILD -DNS_CONFIG=Debug -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY -DNS_DEBUG -DNS_PROFILE -DNS_MINIMUM_LOG_LEVEL=1 -DNS_APP_PROVIDERS_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERDEVICE_API=NS_DLL_EXPORT -DNS_RENDER_RENDERCONTEXT_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERCONTEXT_API=NS_DLL_EXPORT -DNS_APP_DISPLAY_API=NS_DLL_EXPORT -DNS_APP_LAUNCHER_API=NS_DLL_EXPORT -DNS_APP_THEME_API=NS_DLL_EXPORT -DNS_APP_DISPLAYLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVERHELPERS_API=NS_DLL_EXPORT -DNS_APP_APPLICATIONLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_MEDIAELEMENT_API=NS_DLL_EXPORT -DNS_APP_INTERACTIVITY_API=NS_DLL_EXPORT
  CFLAGS = $(CFLAGS_COMMON) -g -D_DEBUG
  CXXFLAGS = $(CXXFLAGS_COMMON) -g -D_DEBUG
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON) -Od -g
else ifeq ($(CONFIG), Profile)
  TARGET_DIR = ../../../../Bin/wasm
  TARGET = ../../../../Bin/wasm/NoesisApp.a
  INTERMEDIATE = build/Profile
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/ApplicationLauncher/Include -I../../../Packages/App/DisplayLauncher/Include -I../../../Packages/App/Display/Include -I../../../Packages/App/Interactivity/Include -I../../../Packages/App/LangServerHelpers/Include -I../../../Packages/App/LangServer/Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/MediaElement/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -I../../../Packages/Render/RenderContext/Include -Ibuild/Profile
  DEFINES = -DNS_BUILD -DNS_CONFIG=Profile -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY -DNS_PROFILE -DNS_MINIMUM_LOG_LEVEL=2 -DNS_APP_PROVIDERS_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERDEVICE_API=NS_DLL_EXPORT -DNS_RENDER_RENDERCONTEXT_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERCONTEXT_API=NS_DLL_EXPORT -DNS_APP_DISPLAY_API=NS_DLL_EXPORT -DNS_APP_LAUNCHER_API=NS_DLL_EXPORT -DNS_APP_THEME_API=NS_DLL_EXPORT -DNS_APP_DISPLAYLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVERHELPERS_API=NS_DLL_EXPORT -DNS_APP_APPLICATIONLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_MEDIAELEMENT_API=NS_DLL_EXPORT -DNS_APP_INTERACTIVITY_API=NS_DLL_EXPORT
  CFLAGS = $(CFLAGS_COMMON) -Oz -DNDEBUG
  CXXFLAGS = $(CXXFLAGS_COMMON) -Oz -DNDEBUG
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON)
else ifeq ($(CONFIG), Release)
  TARGET_DIR = ../../../../Bin/wasm
  TARGET = ../../../../Bin/wasm/NoesisApp.a
  INTERMEDIATE = build/Release
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/ApplicationLauncher/Include -I../../../Packages/App/DisplayLauncher/Include -I../../../Packages/App/Display/Include -I../../../Packages/App/Interactivity/Include -I../../../Packages/App/LangServerHelpers/Include -I../../../Packages/App/LangServer/Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/MediaElement/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -I../../../Packages/Render/RenderContext/Include -Ibuild/Release
  DEFINES = -DNS_BUILD -DNS_CONFIG=Release -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY -DNS_APP_PROVIDERS_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERDEVICE_API=NS_DLL_EXPORT -DNS_RENDER_RENDERCONTEXT_API=NS_DLL_EXPORT -DNS_RENDER_GLRENDERCONTEXT_API=NS_DLL_EXPORT -DNS_APP_DISPLAY_API=NS_DLL_EXPORT -DNS_APP_LAUNCHER_API=NS_DLL_EXPORT -DNS_APP_THEME_API=NS_DLL_EXPORT -DNS_APP_DISPLAYLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVER_API=NS_DLL_EXPORT -DNS_APP_LANGSERVERHELPERS_API=NS_DLL_EXPORT -DNS_APP_APPLICATIONLAUNCHER_API=NS_DLL_EXPORT -DNS_APP_MEDIAELEMENT_API=NS_DLL_EXPORT -DNS_APP_INTERACTIVITY_API=NS_DLL_EXPORT
  CFLAGS = $(CFLAGS_COMMON) -Oz -DNDEBUG -flto
  CXXFLAGS = $(CXXFLAGS_COMMON) -Oz -DNDEBUG -flto
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON)
else
  $(error Invalid configuration '$(CONFIG)')
endif

PCH = $(INTERMEDIATE)/pch.h.gch
PCHFLAGS = -include $(INTERMEDIATE)/pch.h
DATA = $(INTERMEDIATE)/NoesisTheme.Styles.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Fonts.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Colors.Dark.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Colors.Light.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkRed.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightRed.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkGreen.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightGreen.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkBlue.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightBlue.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkOrange.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightOrange.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkEmerald.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightEmerald.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkPurple.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightPurple.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkCrimson.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightCrimson.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkLime.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightLime.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.DarkAqua.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.Brushes.LightAqua.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkRed.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightRed.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkGreen.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightGreen.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkBlue.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightBlue.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkOrange.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightOrange.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkEmerald.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightEmerald.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkPurple.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightPurple.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkCrimson.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightCrimson.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkLime.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightLime.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.DarkAqua.xaml.bin.h $(INTERMEDIATE)/NoesisTheme.LightAqua.xaml.bin.h $(INTERMEDIATE)/PT\ Root\ UI_Regular.otf.bin.h $(INTERMEDIATE)/PT\ Root\ UI_Bold.otf.bin.h $(INTERMEDIATE)/StatsOverlay.xaml.bin.h $(INTERMEDIATE)/MessageBox.xaml.bin.h $(INTERMEDIATE)/dripicons-v2.ttf.bin.h $(INTERMEDIATE)/ModeNine.ttf.bin.h $(INTERMEDIATE)/generic.xaml.bin.h

.SUFFIXES:
.DELETE_ON_ERROR:
.PHONY: all prebuild data clean

all: prebuild $(TARGET)

prebuild:
	@echo ==== [NoesisApp] Building '$(CONFIG)' configuration ====

data: $(DATA)

$(INTERMEDIATE):
	$(Q)$(call MKDIR,$(INTERMEDIATE))

$(TARGET_DIR):
	$(Q)$(call MKDIR,$(TARGET_DIR))

$(PCH): pch/pch.h | $(INTERMEDIATE)
	@echo [PCH] [NoesisApp] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -x c++-header -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)


$(INTERMEDIATE)/%.o: ../../../Packages/App/Providers/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/Providers] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Providers/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/Providers] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/GLRenderDevice/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [Render/GLRenderDevice] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/GLRenderDevice/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [Render/GLRenderDevice] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/RenderContext/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [Render/RenderContext] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/RenderContext/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [Render/RenderContext] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/GLRenderContext/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [Render/GLRenderContext] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Render/GLRenderContext/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [Render/GLRenderContext] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Display/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/Display] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Display/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/Display] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/EmscriptenDisplay/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/EmscriptenDisplay] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/EmscriptenDisplay/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/EmscriptenDisplay] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Launcher/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/Launcher] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Launcher/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/Launcher] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Theme/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/Theme] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Theme/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/Theme] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/NoesisTheme.Styles.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Styles.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Styles.xaml" > "$(INTERMEDIATE)/NoesisTheme.Styles.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Fonts.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Fonts.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Fonts.xaml" > "$(INTERMEDIATE)/NoesisTheme.Fonts.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Colors.Dark.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Colors.Dark.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Colors.Dark.xaml" > "$(INTERMEDIATE)/NoesisTheme.Colors.Dark.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Colors.Light.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Colors.Light.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Colors.Light.xaml" > "$(INTERMEDIATE)/NoesisTheme.Colors.Light.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkRed.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkRed.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkRed.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkRed.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightRed.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightRed.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightRed.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightRed.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkGreen.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkGreen.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkGreen.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkGreen.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightGreen.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightGreen.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightGreen.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightGreen.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkBlue.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkBlue.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkBlue.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkBlue.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightBlue.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightBlue.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightBlue.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightBlue.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkOrange.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkOrange.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkOrange.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkOrange.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightOrange.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightOrange.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightOrange.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightOrange.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkEmerald.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkEmerald.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkEmerald.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkEmerald.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightEmerald.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightEmerald.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightEmerald.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightEmerald.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkPurple.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkPurple.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkPurple.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkPurple.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightPurple.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightPurple.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightPurple.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightPurple.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkCrimson.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkCrimson.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkCrimson.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkCrimson.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightCrimson.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightCrimson.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightCrimson.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightCrimson.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkLime.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkLime.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkLime.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkLime.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightLime.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightLime.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightLime.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightLime.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.DarkAqua.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkAqua.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.DarkAqua.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.DarkAqua.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.Brushes.LightAqua.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightAqua.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.Brushes.LightAqua.xaml" > "$(INTERMEDIATE)/NoesisTheme.Brushes.LightAqua.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkRed.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkRed.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkRed.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkRed.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightRed.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightRed.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightRed.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightRed.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkGreen.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkGreen.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkGreen.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkGreen.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightGreen.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightGreen.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightGreen.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightGreen.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkBlue.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkBlue.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkBlue.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkBlue.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightBlue.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightBlue.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightBlue.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightBlue.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkOrange.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkOrange.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkOrange.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkOrange.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightOrange.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightOrange.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightOrange.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightOrange.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkEmerald.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkEmerald.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkEmerald.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkEmerald.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightEmerald.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightEmerald.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightEmerald.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightEmerald.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkPurple.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkPurple.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkPurple.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkPurple.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightPurple.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightPurple.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightPurple.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightPurple.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkCrimson.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkCrimson.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkCrimson.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkCrimson.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightCrimson.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightCrimson.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightCrimson.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightCrimson.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkLime.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkLime.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkLime.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkLime.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightLime.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightLime.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightLime.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightLime.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.DarkAqua.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkAqua.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.DarkAqua.xaml" > "$(INTERMEDIATE)/NoesisTheme.DarkAqua.xaml.bin.h"

$(INTERMEDIATE)/NoesisTheme.LightAqua.xaml.bin.h: ../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightAqua.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/NoesisTheme.LightAqua.xaml" > "$(INTERMEDIATE)/NoesisTheme.LightAqua.xaml.bin.h"

$(INTERMEDIATE)/PT\ Root\ UI_Regular.otf.bin.h: ../../../Packages/App/Theme/Data/Theme/Fonts/PT\ Root\ UI_Regular.otf | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/Fonts/PT Root UI_Regular.otf" > "$(INTERMEDIATE)/PT Root UI_Regular.otf.bin.h"

$(INTERMEDIATE)/PT\ Root\ UI_Bold.otf.bin.h: ../../../Packages/App/Theme/Data/Theme/Fonts/PT\ Root\ UI_Bold.otf | $(INTERMEDIATE)
	@echo [BIN] [App/Theme] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/Theme/Data/Theme/Fonts/PT Root UI_Bold.otf" > "$(INTERMEDIATE)/PT Root UI_Bold.otf.bin.h"

$(INTERMEDIATE)/%.o: ../../../Packages/App/DisplayLauncher/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/DisplayLauncher] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/DisplayLauncher/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/DisplayLauncher] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/LangServer/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/LangServer] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/LangServer/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/LangServer] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/LangServerHelpers/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/LangServerHelpers] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/LangServerHelpers/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/LangServerHelpers] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/ApplicationLauncher/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/ApplicationLauncher/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/StatsOverlay.xaml.bin.h: ../../../Packages/App/ApplicationLauncher/Src/StatsOverlay.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/ApplicationLauncher/Src/StatsOverlay.xaml" > "$(INTERMEDIATE)/StatsOverlay.xaml.bin.h"

$(INTERMEDIATE)/MessageBox.xaml.bin.h: ../../../Packages/App/ApplicationLauncher/Src/MessageBox.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/ApplicationLauncher/Src/MessageBox.xaml" > "$(INTERMEDIATE)/MessageBox.xaml.bin.h"

$(INTERMEDIATE)/dripicons-v2.ttf.bin.h: ../../../Packages/App/ApplicationLauncher/Src/dripicons-v2.ttf | $(INTERMEDIATE)
	@echo [BIN] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/ApplicationLauncher/Src/dripicons-v2.ttf" > "$(INTERMEDIATE)/dripicons-v2.ttf.bin.h"

$(INTERMEDIATE)/ModeNine.ttf.bin.h: ../../../Packages/App/ApplicationLauncher/Src/ModeNine.ttf | $(INTERMEDIATE)
	@echo [BIN] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/ApplicationLauncher/Src/ModeNine.ttf" > "$(INTERMEDIATE)/ModeNine.ttf.bin.h"

$(INTERMEDIATE)/generic.xaml.bin.h: ../../../Packages/App/ApplicationLauncher/Src/generic.xaml | $(INTERMEDIATE)
	@echo [BIN] [App/ApplicationLauncher] $(notdir $<)
	$(Q)$(BIN2H) "../../../Packages/App/ApplicationLauncher/Src/generic.xaml" > "$(INTERMEDIATE)/generic.xaml.bin.h"

$(INTERMEDIATE)/%.o: ../../../Packages/App/MediaElement/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/MediaElement] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/MediaElement/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/MediaElement] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Interactivity/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [App/Interactivity] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/App/Interactivity/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [App/Interactivity] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/register.o: register.cpp $(PCH) | $(INTERMEDIATE)
	@echo [CXX] [NoesisApp] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)


$(TARGET): $(LIBS_DEPS) $(INTERMEDIATE)/register.o $(INTERMEDIATE)/App.Providers.o $(INTERMEDIATE)/EmbeddedFontProvider.o $(INTERMEDIATE)/EmbeddedTextureProvider.o $(INTERMEDIATE)/EmbeddedXamlProvider.o $(INTERMEDIATE)/FastLZ.o $(INTERMEDIATE)/FileSystemWatcher.o $(INTERMEDIATE)/FileTextureProvider.o $(INTERMEDIATE)/Find.o $(INTERMEDIATE)/LocalFontProvider.o $(INTERMEDIATE)/LocalTextureProvider.o $(INTERMEDIATE)/LocalXamlProvider.o $(INTERMEDIATE)/GLRenderDevice.o $(INTERMEDIATE)/Render.GLRenderDevice.o $(INTERMEDIATE)/Image.o $(INTERMEDIATE)/Render.RenderContext.o $(INTERMEDIATE)/RenderContext.o $(INTERMEDIATE)/GLRenderContext.mm.o $(INTERMEDIATE)/Render.GLRenderContext.o $(INTERMEDIATE)/App.Display.o $(INTERMEDIATE)/Audio.mm.o $(INTERMEDIATE)/Display.o $(INTERMEDIATE)/EmscriptenDisplay.o $(INTERMEDIATE)/Gui.EmscriptenDisplay.o $(INTERMEDIATE)/App.Launcher.o $(INTERMEDIATE)/CommandLine.o $(INTERMEDIATE)/Launcher.o $(INTERMEDIATE)/App.Theme.o $(INTERMEDIATE)/ThemeProviders.o $(INTERMEDIATE)/App.DisplayLauncher.o $(INTERMEDIATE)/DisplayLauncher.o $(INTERMEDIATE)/App.LangServer.o $(INTERMEDIATE)/CapabilityColor.o $(INTERMEDIATE)/CapabilityCompletion.o $(INTERMEDIATE)/CapabilityDiagnostics.o $(INTERMEDIATE)/DocumentHelper.o $(INTERMEDIATE)/JsonBuilder.o $(INTERMEDIATE)/JsonObject.o $(INTERMEDIATE)/LangServer.o $(INTERMEDIATE)/LangServerReflectionHelper.o $(INTERMEDIATE)/LangServerSocket.o $(INTERMEDIATE)/LenientXamlParser.o $(INTERMEDIATE)/MessageBuffer.o $(INTERMEDIATE)/MessageReader.o $(INTERMEDIATE)/MessageWriter.o $(INTERMEDIATE)/Workspace.o $(INTERMEDIATE)/App.LangServerHelpers.o $(INTERMEDIATE)/PreviewRenderer.o $(INTERMEDIATE)/App.ApplicationLauncher.o $(INTERMEDIATE)/Application.o $(INTERMEDIATE)/ApplicationLauncher.o $(INTERMEDIATE)/DelegateCommand.o $(INTERMEDIATE)/LocExtension.o $(INTERMEDIATE)/MessageBox.o $(INTERMEDIATE)/NotifyPropertyChangedBase.o $(INTERMEDIATE)/RichText.o $(INTERMEDIATE)/Window.o $(INTERMEDIATE)/App.MediaElement.o $(INTERMEDIATE)/MediaElement.o $(INTERMEDIATE)/MediaPlayer.o $(INTERMEDIATE)/App.Interactivity.o $(INTERMEDIATE)/AttachableObject.o $(INTERMEDIATE)/BackgroundEffectBehavior.o $(INTERMEDIATE)/Behavior.o $(INTERMEDIATE)/BehaviorCollection.o $(INTERMEDIATE)/ChangePropertyAction.o $(INTERMEDIATE)/CollectionFilterBehavior.o $(INTERMEDIATE)/CollectionSortBehavior.o $(INTERMEDIATE)/ComparisonCondition.o $(INTERMEDIATE)/ComparisonLogic.o $(INTERMEDIATE)/ConditionalExpression.o $(INTERMEDIATE)/ConditionBehavior.o $(INTERMEDIATE)/ControlStoryboardAction.o $(INTERMEDIATE)/DataBindingHelper.o $(INTERMEDIATE)/DataEventTrigger.o $(INTERMEDIATE)/DataTrigger.Interactivity.o $(INTERMEDIATE)/EventTrigger.Interactivity.o $(INTERMEDIATE)/EventTriggerBase.o $(INTERMEDIATE)/FilterPredicate.o $(INTERMEDIATE)/GamepadTrigger.o $(INTERMEDIATE)/GoToStateAction.o $(INTERMEDIATE)/Interaction.o $(INTERMEDIATE)/InvokeCommandAction.o $(INTERMEDIATE)/KeyTrigger.o $(INTERMEDIATE)/LaunchUriOrFileAction.o $(INTERMEDIATE)/LineDecorationBehavior.o $(INTERMEDIATE)/LoadContentAction.o $(INTERMEDIATE)/MediaActions.o $(INTERMEDIATE)/MouseDragElementBehavior.o $(INTERMEDIATE)/MoveFocusAction.o $(INTERMEDIATE)/PlaySoundAction.o $(INTERMEDIATE)/PropertyChangedTrigger.o $(INTERMEDIATE)/RemoveElementAction.o $(INTERMEDIATE)/SelectAction.o $(INTERMEDIATE)/SelectAllAction.o $(INTERMEDIATE)/SetFocusAction.o $(INTERMEDIATE)/SortComparer.o $(INTERMEDIATE)/StoryboardAction.o $(INTERMEDIATE)/StoryboardCompletedTrigger.o $(INTERMEDIATE)/StoryboardTrigger.o $(INTERMEDIATE)/StringFilterPredicate.o $(INTERMEDIATE)/StringSortComparer.o $(INTERMEDIATE)/StyleInteraction.o $(INTERMEDIATE)/TargetedTriggerAction.o $(INTERMEDIATE)/TimerTrigger.o $(INTERMEDIATE)/TranslateZoomRotateBehavior.o $(INTERMEDIATE)/TriggerAction.Interactivity.o $(INTERMEDIATE)/TriggerActionCollection.o $(INTERMEDIATE)/TriggerBase.Interactivity.o $(INTERMEDIATE)/TriggerCollection.o | $(TARGET_DIR)
	@echo [AR] $(notdir $@)
	$(Q)$(AR) rcs $(TARGET) @$(CONFIG).rsp

-include $(INTERMEDIATE)/register.d $(INTERMEDIATE)/pch.h.d $(INTERMEDIATE)/App.Providers.d $(INTERMEDIATE)/EmbeddedFontProvider.d $(INTERMEDIATE)/EmbeddedTextureProvider.d $(INTERMEDIATE)/EmbeddedXamlProvider.d $(INTERMEDIATE)/FastLZ.d $(INTERMEDIATE)/FileSystemWatcher.d $(INTERMEDIATE)/FileTextureProvider.d $(INTERMEDIATE)/Find.d $(INTERMEDIATE)/LocalFontProvider.d $(INTERMEDIATE)/LocalTextureProvider.d $(INTERMEDIATE)/LocalXamlProvider.d $(INTERMEDIATE)/GLRenderDevice.d $(INTERMEDIATE)/Render.GLRenderDevice.d $(INTERMEDIATE)/Image.d $(INTERMEDIATE)/Render.RenderContext.d $(INTERMEDIATE)/RenderContext.d $(INTERMEDIATE)/GLRenderContext.mm.d $(INTERMEDIATE)/Render.GLRenderContext.d $(INTERMEDIATE)/App.Display.d $(INTERMEDIATE)/Audio.mm.d $(INTERMEDIATE)/Display.d $(INTERMEDIATE)/EmscriptenDisplay.d $(INTERMEDIATE)/Gui.EmscriptenDisplay.d $(INTERMEDIATE)/App.Launcher.d $(INTERMEDIATE)/CommandLine.d $(INTERMEDIATE)/Launcher.d $(INTERMEDIATE)/App.Theme.d $(INTERMEDIATE)/ThemeProviders.d $(INTERMEDIATE)/App.DisplayLauncher.d $(INTERMEDIATE)/DisplayLauncher.d $(INTERMEDIATE)/App.LangServer.d $(INTERMEDIATE)/CapabilityColor.d $(INTERMEDIATE)/CapabilityCompletion.d $(INTERMEDIATE)/CapabilityDiagnostics.d $(INTERMEDIATE)/DocumentHelper.d $(INTERMEDIATE)/JsonBuilder.d $(INTERMEDIATE)/JsonObject.d $(INTERMEDIATE)/LangServer.d $(INTERMEDIATE)/LangServerReflectionHelper.d $(INTERMEDIATE)/LangServerSocket.d $(INTERMEDIATE)/LenientXamlParser.d $(INTERMEDIATE)/MessageBuffer.d $(INTERMEDIATE)/MessageReader.d $(INTERMEDIATE)/MessageWriter.d $(INTERMEDIATE)/Workspace.d $(INTERMEDIATE)/App.LangServerHelpers.d $(INTERMEDIATE)/PreviewRenderer.d $(INTERMEDIATE)/App.ApplicationLauncher.d $(INTERMEDIATE)/Application.d $(INTERMEDIATE)/ApplicationLauncher.d $(INTERMEDIATE)/DelegateCommand.d $(INTERMEDIATE)/LocExtension.d $(INTERMEDIATE)/MessageBox.d $(INTERMEDIATE)/NotifyPropertyChangedBase.d $(INTERMEDIATE)/RichText.d $(INTERMEDIATE)/Window.d $(INTERMEDIATE)/App.MediaElement.d $(INTERMEDIATE)/MediaElement.d $(INTERMEDIATE)/MediaPlayer.d $(INTERMEDIATE)/App.Interactivity.d $(INTERMEDIATE)/AttachableObject.d $(INTERMEDIATE)/BackgroundEffectBehavior.d $(INTERMEDIATE)/Behavior.d $(INTERMEDIATE)/BehaviorCollection.d $(INTERMEDIATE)/ChangePropertyAction.d $(INTERMEDIATE)/CollectionFilterBehavior.d $(INTERMEDIATE)/CollectionSortBehavior.d $(INTERMEDIATE)/ComparisonCondition.d $(INTERMEDIATE)/ComparisonLogic.d $(INTERMEDIATE)/ConditionalExpression.d $(INTERMEDIATE)/ConditionBehavior.d $(INTERMEDIATE)/ControlStoryboardAction.d $(INTERMEDIATE)/DataBindingHelper.d $(INTERMEDIATE)/DataEventTrigger.d $(INTERMEDIATE)/DataTrigger.Interactivity.d $(INTERMEDIATE)/EventTrigger.Interactivity.d $(INTERMEDIATE)/EventTriggerBase.d $(INTERMEDIATE)/FilterPredicate.d $(INTERMEDIATE)/GamepadTrigger.d $(INTERMEDIATE)/GoToStateAction.d $(INTERMEDIATE)/Interaction.d $(INTERMEDIATE)/InvokeCommandAction.d $(INTERMEDIATE)/KeyTrigger.d $(INTERMEDIATE)/LaunchUriOrFileAction.d $(INTERMEDIATE)/LineDecorationBehavior.d $(INTERMEDIATE)/LoadContentAction.d $(INTERMEDIATE)/MediaActions.d $(INTERMEDIATE)/MouseDragElementBehavior.d $(INTERMEDIATE)/MoveFocusAction.d $(INTERMEDIATE)/PlaySoundAction.d $(INTERMEDIATE)/PropertyChangedTrigger.d $(INTERMEDIATE)/RemoveElementAction.d $(INTERMEDIATE)/SelectAction.d $(INTERMEDIATE)/SelectAllAction.d $(INTERMEDIATE)/SetFocusAction.d $(INTERMEDIATE)/SortComparer.d $(INTERMEDIATE)/StoryboardAction.d $(INTERMEDIATE)/StoryboardCompletedTrigger.d $(INTERMEDIATE)/StoryboardTrigger.d $(INTERMEDIATE)/StringFilterPredicate.d $(INTERMEDIATE)/StringSortComparer.d $(INTERMEDIATE)/StyleInteraction.d $(INTERMEDIATE)/TargetedTriggerAction.d $(INTERMEDIATE)/TimerTrigger.d $(INTERMEDIATE)/TranslateZoomRotateBehavior.d $(INTERMEDIATE)/TriggerAction.Interactivity.d $(INTERMEDIATE)/TriggerActionCollection.d $(INTERMEDIATE)/TriggerBase.Interactivity.d $(INTERMEDIATE)/TriggerCollection.d

server:
	$(Q)python3 -m http.server --bind 127.0.0.1 --directory $(TARGET_DIR)

clean:
	@echo ==== [NoesisApp] Cleaning '$(CONFIG)' configuration ====
	$(Q)$(call RMTREE,$(INTERMEDIATE))
	$(Q)$(call RM,$(TARGET))
