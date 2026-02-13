@echo off

if exist Shaders.tar del Shaders.tar
if exist Shaders.h del Shaders.h

:: Vertex Shaders

CALL :fxc_vs Pos ShaderVS.hlsl
CALL :fxc_vs PosColor ShaderVS.hlsl /DHAS_COLOR
CALL :fxc_vs PosTex0 ShaderVS.hlsl /DHAS_UV0
CALL :fxc_vs PosTex0Rect ShaderVS.hlsl /DHAS_UV0 /DHAS_RECT
CALL :fxc_vs PosTex0RectTile ShaderVS.hlsl /DHAS_UV0 /DHAS_RECT /DHAS_TILE
CALL :fxc_vs PosColorCoverage ShaderVS.hlsl /DHAS_COLOR /DHAS_COVERAGE
CALL :fxc_vs PosTex0Coverage ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE
CALL :fxc_vs PosTex0CoverageRect ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE /DHAS_RECT
CALL :fxc_vs PosTex0CoverageRectTile ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE /DHAS_RECT /DHAS_TILE
CALL :fxc_vs PosColorTex1_SDF ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DSDF
CALL :fxc_vs PosTex0Tex1_SDF ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DSDF
CALL :fxc_vs PosTex0Tex1Rect_SDF ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DSDF
CALL :fxc_vs PosTex0Tex1RectTile_SDF ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DHAS_TILE /DSDF
CALL :fxc_vs PosColorTex1 ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1
CALL :fxc_vs PosTex0Tex1 ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1
CALL :fxc_vs PosTex0Tex1Rect ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT
CALL :fxc_vs PosTex0Tex1RectTile ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DHAS_TILE
CALL :fxc_vs PosColorTex0Tex1 ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_UV1
CALL :fxc_vs PosTex0Tex1_Downsample ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DDOWNSAMPLE
CALL :fxc_vs PosColorTex1Rect ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DHAS_RECT
CALL :fxc_vs PosColorTex0RectImagePos ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_RECT /DHAS_IMAGE_POSITION

CALL :fxc_vs PosColor_SRGB ShaderVS.hlsl /DHAS_COLOR /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorCoverage_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_COVERAGE /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorTex1_SDF_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DSDF /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorTex1_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorTex0Tex1_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_UV1 /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorTex1Rect_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DHAS_RECT /DLINEAR_COLOR_SPACE
CALL :fxc_vs PosColorTex0RectImagePos_SRGB ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_RECT /DHAS_IMAGE_POSITION /DLINEAR_COLOR_SPACE

:: Pixel Shaders

CALL :fxc_ps RGBA ShaderPS.hlsl /DEFFECT_RGBA
CALL :fxc_ps Mask ShaderPS.hlsl /DEFFECT_MASK
CALL :fxc_ps Clear ShaderPS.hlsl /DEFFECT_CLEAR

CALL :fxc_ps Path_Solid ShaderPS.hlsl /DEFFECT_PATH /DPAINT_SOLID
CALL :fxc_ps Path_Linear ShaderPS.hlsl /DEFFECT_PATH /DPAINT_LINEAR
CALL :fxc_ps Path_Radial ShaderPS.hlsl /DEFFECT_PATH /DPAINT_RADIAL
CALL :fxc_ps Path_Pattern ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN
CALL :fxc_ps Path_Pattern_Clamp ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps Path_Pattern_Repeat ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps Path_Pattern_MirrorU ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps Path_Pattern_MirrorV ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps Path_Pattern_Mirror ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps Path_AA_Solid ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_SOLID
CALL :fxc_ps Path_AA_Linear ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_LINEAR
CALL :fxc_ps Path_AA_Radial ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_RADIAL
CALL :fxc_ps Path_AA_Pattern ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN
CALL :fxc_ps Path_AA_Pattern_Clamp ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps Path_AA_Pattern_Repeat ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps Path_AA_Pattern_MirrorU ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps Path_AA_Pattern_MirrorV ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps Path_AA_Pattern_Mirror ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps SDF_Solid ShaderPS.hlsl /DEFFECT_SDF /DPAINT_SOLID
CALL :fxc_ps SDF_Linear ShaderPS.hlsl /DEFFECT_SDF /DPAINT_LINEAR
CALL :fxc_ps SDF_Radial ShaderPS.hlsl /DEFFECT_SDF /DPAINT_RADIAL
CALL :fxc_ps SDF_Pattern ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN
CALL :fxc_ps SDF_Pattern_Clamp ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps SDF_Pattern_Repeat ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps SDF_Pattern_MirrorU ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps SDF_Pattern_MirrorV ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps SDF_Pattern_Mirror ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps SDF_LCD_Solid ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_SOLID
CALL :fxc_ps SDF_LCD_Linear ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_LINEAR
CALL :fxc_ps SDF_LCD_Radial ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_RADIAL
CALL :fxc_ps SDF_LCD_Pattern ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN
CALL :fxc_ps SDF_LCD_Pattern_Clamp ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps SDF_LCD_Pattern_Repeat ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps SDF_LCD_Pattern_MirrorU ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps SDF_LCD_Pattern_MirrorV ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps SDF_LCD_Pattern_Mirror ShaderPS.hlsl /DEFFECT_SDF_LCD /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps Opacity_Solid ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_SOLID
CALL :fxc_ps Opacity_Linear ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_LINEAR
CALL :fxc_ps Opacity_Radial ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_RADIAL
CALL :fxc_ps Opacity_Pattern ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN
CALL :fxc_ps Opacity_Pattern_Clamp ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps Opacity_Pattern_Repeat ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps Opacity_Pattern_MirrorU ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps Opacity_Pattern_MirrorV ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps Opacity_Pattern_Mirror ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps Upsample ShaderPS.hlsl /DEFFECT_UPSAMPLE
CALL :fxc_ps Downsample ShaderPS.hlsl /DEFFECT_DOWNSAMPLE

CALL :fxc_ps Shadow ShaderPS.hlsl /DEFFECT_SHADOW /DPAINT_SOLID
CALL :fxc_ps Blur ShaderPS.hlsl /DEFFECT_BLUR /DPAINT_SOLID

:: Compress
Pack Shaders.tar Shaders.z
bin2h.py Shaders.z Shaders >> Shaders.h

del Shaders.z
del Shaders.tar

EXIT /B 0

:: -------------------------------------------------------------------------------------------------------
:fxc_vs
    fxc /T vs_4_0 /nologo /O3 /Qstrip_reflect /Fo %1_VS.o %2 %3 %4 %5 %6 %7 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %1_VS.o >> Shaders.h
    del %1_VS.o

    fxc /T vs_4_0 /nologo /O3 /Qstrip_reflect /Fo %1_Stereo_VS.o /DSTEREO_RENDERING %2 %3 %4 %5 %6 %7 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %1_Stereo_VS.o >> Shaders.h
    del %1_Stereo_VS.o

    EXIT /B 0

:fxc_ps
    fxc /T ps_4_0 /nologo /O3 /Qstrip_reflect /Fo %1_PS.o %2 %3 %4 %5 %6 %7 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %1_PS.o >> Shaders.h
    del %1_PS.o
    EXIT /B 0