# GNU Make project for 'Samples.IntegrationGLUT'

# Parallel builds can be enabled using the environment variable MAKEFLAGS (eg: MAKEFLAGS="-j 4")
# For parallel execution in Windows, Make 4.0 or better is required

CC = emcc
CXX = em++
AR = emar

CFLAGS_COMMON = -pipe -Wall -Werror -Wfatal-errors -Wtype-limits -Wtype-limits -Wnested-anon-types -msimd128
CXXFLAGS_COMMON = $(CFLAGS_COMMON) -std=c++11 -fno-exceptions -frtti -fno-threadsafe-statics
LDFLAGS_COMMON = -s USE_WEBGL2=1 -s NO_FILESYSTEM=1 -s WASM=1 -s STACK_SIZE=1MB -s INITIAL_MEMORY=64MB -s EXPORTED_FUNCTIONS='["_malloc","_free","_main"]'
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
  TARGET = ../../../../Bin/wasm/Samples.IntegrationGLUT.a
  TARGET_HTML = ../../../../Bin/wasm/Samples.IntegrationGLUT.html
  INTERMEDIATE = build/Debug
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -Ibuild/Debug
  DEFINES = -DNS_BUILD -DNS_CONFIG=Debug -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY -DNS_DEBUG -DNS_PROFILE -DNS_MINIMUM_LOG_LEVEL=1
  CFLAGS = $(CFLAGS_COMMON) -g -D_DEBUG
  CXXFLAGS = $(CXXFLAGS_COMMON) -g -D_DEBUG
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON) -Od -g
else ifeq ($(CONFIG), Profile)
  TARGET_DIR = ../../../../Bin/wasm
  TARGET = ../../../../Bin/wasm/Samples.IntegrationGLUT.a
  TARGET_HTML = ../../../../Bin/wasm/Samples.IntegrationGLUT.html
  INTERMEDIATE = build/Profile
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -Ibuild/Profile
  DEFINES = -DNS_BUILD -DNS_CONFIG=Profile -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY -DNS_PROFILE -DNS_MINIMUM_LOG_LEVEL=2
  CFLAGS = $(CFLAGS_COMMON) -Oz -DNDEBUG
  CXXFLAGS = $(CXXFLAGS_COMMON) -Oz -DNDEBUG
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON)
else ifeq ($(CONFIG), Release)
  TARGET_DIR = ../../../../Bin/wasm
  TARGET = ../../../../Bin/wasm/Samples.IntegrationGLUT.a
  TARGET_HTML = ../../../../Bin/wasm/Samples.IntegrationGLUT.html
  INTERMEDIATE = build/Release
  LIB_DIRS = 
  LIBS = 
  LIBS_DEPS = 
  INCLUDES = -I../../../../Include -I../../../Packages/App/Launcher/Include -I../../../Packages/App/Providers/Include -I../../../Packages/App/Theme/Include -I../../../Packages/Render/GLRenderDevice/Include -Ibuild/Release
  DEFINES = -DNS_BUILD -DNS_CONFIG=Release -DNS_VERSION=3.2.10 -DNS_APP_FRAMEWORK -DNS_LICENSE -DNS_STATIC_LIBRARY
  CFLAGS = $(CFLAGS_COMMON) -Oz -DNDEBUG -flto
  CXXFLAGS = $(CXXFLAGS_COMMON) -Oz -DNDEBUG -flto
  LDFLAGS = $(LDFLAGS_COMMON) $(CXXFLAGS)
  SHADERFLAGS = $(SHADERFLAGS_COMMON)
else
  $(error Invalid configuration '$(CONFIG)')
endif

PCH =
PCHFLAGS =
DATA =

.SUFFIXES:
.DELETE_ON_ERROR:
.PHONY: all prebuild data clean

all: prebuild $(TARGET_HTML)

prebuild:
	@echo ==== [Samples.IntegrationGLUT] Building '$(CONFIG)' configuration ====

data: $(DATA)

$(INTERMEDIATE):
	$(Q)$(call MKDIR,$(INTERMEDIATE))

$(TARGET_DIR):
	$(Q)$(call MKDIR,$(TARGET_DIR))

$(INTERMEDIATE)/%.o: ../../../Packages/Samples/IntegrationGLUT/Src/%.cpp $(PCH) | $(INTERMEDIATE) data
	@echo [CXX] [Samples/IntegrationGLUT] $(notdir $<)
	$(Q)$(CXX) $(CXXFLAGS) $(PCHFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(INTERMEDIATE)/%.o: ../../../Packages/Samples/IntegrationGLUT/Src/%.c | $(INTERMEDIATE) data
	@echo [CC] [Samples/IntegrationGLUT] $(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -MMD -MF $(basename $@).d.tmp -MP -o $@ $< && $(call MV,$(basename $@).d.tmp,$(basename $@).d)

	$(Q)$(call FIXSLASHES,$(@:.o=.d))

$(TARGET): $(LIBS_DEPS) $(INTERMEDIATE)/Main.o | $(TARGET_DIR)
	@echo [AR] $(notdir $@)
	$(Q)$(AR) rcs $(TARGET) @$(CONFIG).rsp

$(TARGET_HTML): $(TARGET) ../../../../Bin/wasm/Noesis.a ../../../../Bin/wasm/NoesisApp.a
	@echo [HTML] $(notdir $@)
	$(Q)$(CC) $(LDFLAGS) $(TARGET) ../../../../Bin/wasm/Noesis.a ../../../../Bin/wasm/NoesisApp.a -o $(basename $(TARGET)).html --shell-file shell.html

-include $(INTERMEDIATE)/Main.d

server:
	$(Q)python3 -m http.server --bind 127.0.0.1 --directory $(TARGET_DIR)

clean:
	@echo ==== [Samples.IntegrationGLUT] Cleaning '$(CONFIG)' configuration ====
	$(Q)$(call RMTREE,$(INTERMEDIATE))
	$(Q)$(call RM,$(TARGET))
	$(Q)$(call RM,$(basename $(TARGET)).js)
	$(Q)$(call RM,$(basename $(TARGET)).wasm)
	$(Q)$(call RM,$(basename $(TARGET)).wast)
	$(Q)$(call RM,$(basename $(TARGET)).html)
