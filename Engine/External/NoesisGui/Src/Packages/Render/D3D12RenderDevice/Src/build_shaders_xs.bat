:: This script builds shaders for Xbox with stripped DXIL (precompiled) for much faster loading times
:: Note that using this script is optional and you can keep using 'build_shaders.bat'
:: Just make sure to #undef USE_DXC_SHADERS in D3D12RenderDevice.cpp if not using this script

@echo off

if exist Shaders.tar del Shaders.tar
if exist Shaders_xs.h del Shaders_xs.h
if not exist pdb mkdir pdb

:: Root signature components

set root=RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
set vs_cb0=CBV(b0,flags=DATA_STATIC,visibility=SHADER_VISIBILITY_VERTEX),
set vs_cb1=CBV(b1,flags=DATA_STATIC,visibility=SHADER_VISIBILITY_VERTEX),
set ps_cb0=CBV(b0,flags=DATA_STATIC,visibility=SHADER_VISIBILITY_PIXEL),
set ps_cb1=CBV(b1,flags=DATA_STATIC,visibility=SHADER_VISIBILITY_PIXEL),
set t0=DescriptorTable(SRV(t0,flags=DATA_STATIC_WHILE_SET_AT_EXECUTE),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s0),visibility=SHADER_VISIBILITY_PIXEL),
set t1=DescriptorTable(SRV(t1,flags=DATA_STATIC_WHILE_SET_AT_EXECUTE),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s1),visibility=SHADER_VISIBILITY_PIXEL),
set t2=DescriptorTable(SRV(t2,flags=DATA_STATIC_WHILE_SET_AT_EXECUTE),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s2),visibility=SHADER_VISIBILITY_PIXEL),
set t3=DescriptorTable(SRV(t3,flags=DATA_STATIC_WHILE_SET_AT_EXECUTE),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s3),visibility=SHADER_VISIBILITY_PIXEL),
set t4=DescriptorTable(SRV(t4,flags=DATA_STATIC_WHILE_SET_AT_EXECUTE),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s4),visibility=SHADER_VISIBILITY_PIXEL),

:: Vertex Shaders

CALL :fxc_vs "%root%%vs_cb0%" Pos_VS ShaderVS.hlsl
CALL :fxc_vs "%root%%vs_cb0%" PosColor_VS ShaderVS.hlsl /DHAS_COLOR
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t1%" PosTex0_VS ShaderVS.hlsl /DHAS_UV0
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0Rect_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_RECT
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0RectTile_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_RECT /DHAS_TILE
CALL :fxc_vs "%root%%vs_cb0%" PosColorCoverage_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_COVERAGE
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t1%" PosTex0Coverage_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0CoverageRect_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE /DHAS_RECT
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0CoverageRectTile_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE /DHAS_RECT /DHAS_TILE
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%t3%" PosColorTex1_SDF_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DSDF
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t1%%t3%" PosTex0Tex1_SDF_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DSDF
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" PosTex0Tex1Rect_SDF_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DSDF
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" PosTex0Tex1RectTile_SDF_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DHAS_TILE /DSDF
CALL :fxc_vs "%root%%vs_cb0%%t2%" PosColorTex1_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t1%%t2%" PosTex0Tex1_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%%t2%" PosTex0Tex1Rect_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%%t2%" PosTex0Tex1RectTile_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DHAS_RECT /DHAS_TILE
CALL :fxc_vs "%root%%vs_cb0%%t0%%t2%" PosColorTex0Tex1_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_UV1
CALL :fxc_vs "%root%%vs_cb0%%t0%" PosTex0Tex1_Downsample_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DDOWNSAMPLE
CALL :fxc_vs "%root%%vs_cb0%%ps_cb1%%t2%%t4%" PosColorTex1Rect_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DHAS_RECT
CALL :fxc_vs "%root%%vs_cb0%%ps_cb1%%t0%" PosColorTex0RectImagePos_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_RECT /DHAS_IMAGE_POSITION

:: Vertex Shaders (sRGB variants)
CALL :fxc_vs "%root%%vs_cb0%" PosColor_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%" PosColorCoverage_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_COVERAGE /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%t3%" PosColorTex1_SDF_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DSDF /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%%t2%" PosColorTex1_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%%t0%%t2%" PosColorTex0Tex1_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_UV1 /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%%ps_cb1%%t2%%t4%" PosColorTex1Rect_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1 /DHAS_RECT /DLINEAR_COLOR_SPACE
CALL :fxc_vs "%root%%vs_cb0%%ps_cb1%%t0%" PosColorTex0RectImagePos_SRGB_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV0 /DHAS_RECT /DHAS_IMAGE_POSITION /DLINEAR_COLOR_SPACE

:: Vertex Shaders (duplicated with a different root signature)
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%" Pos_x_VS ShaderVS.hlsl
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0_x_VS ShaderVS.hlsl /DHAS_UV0
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%" PosTex0Coverage_x_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_COVERAGE
CALL :fxc_vs "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" PosTex0Tex1_SDF_x_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1 /DSDF
CALL :fxc_vs "%root%%vs_cb0%%ps_cb0%%t0%%t2%" PosTex0Tex1_x_VS ShaderVS.hlsl /DHAS_UV0 /DHAS_UV1
CALL :fxc_vs "%root%%vs_cb0%%ps_cb1%%t2%%t4%" PosColorTex1_x_VS ShaderVS.hlsl /DHAS_COLOR /DHAS_UV1

:: Pixel Shaders

CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%" RGBA_PS ShaderPS.hlsl /DEFFECT_RGBA
CALL :fxc_ps "%root%%vs_cb0%" Mask_PS ShaderPS.hlsl /DEFFECT_MASK
CALL :fxc_ps "%root%%vs_cb0%" Clear_PS ShaderPS.hlsl /DEFFECT_CLEAR

CALL :fxc_ps "%root%%vs_cb0%" Path_Solid_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_SOLID
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%" Path_Linear_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_LINEAR
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%" Path_Radial_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_RADIAL
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_Clamp_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_Repeat_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_MirrorU_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_MirrorV_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_Pattern_Mirror_PS ShaderPS.hlsl /DEFFECT_PATH /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps "%root%%vs_cb0%" Path_AA_Solid_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_SOLID
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%" Path_AA_Linear_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_LINEAR
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%" Path_AA_Radial_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_RADIAL
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_Clamp_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_Repeat_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_MirrorU_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_MirrorV_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%" Path_AA_Pattern_Mirror_PS ShaderPS.hlsl /DEFFECT_PATH_AA /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%t3%" SDF_Solid_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_SOLID
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t1%%t3%" SDF_Linear_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_LINEAR
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t1%%t3%" SDF_Radial_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_RADIAL
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_Clamp_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_Repeat_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_MirrorU_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_MirrorV_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%vs_cb1%%ps_cb0%%t0%%t3%" SDF_Pattern_Mirror_PS ShaderPS.hlsl /DEFFECT_SDF /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps "%root%%vs_cb0%%t2%" Opacity_Solid_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_SOLID
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%%t2%" Opacity_Linear_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_LINEAR
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t1%%t2%" Opacity_Radial_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_RADIAL
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_Clamp_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DCLAMP_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_Repeat_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DREPEAT_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_MirrorU_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRRORU_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_MirrorV_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRRORV_PATTERN
CALL :fxc_ps "%root%%vs_cb0%%ps_cb0%%t0%%t2%" Opacity_Pattern_Mirror_PS ShaderPS.hlsl /DEFFECT_OPACITY /DPAINT_PATTERN /DMIRROR_PATTERN

CALL :fxc_ps "%root%%vs_cb0%%t0%%t2%" Upsample_PS ShaderPS.hlsl /DEFFECT_UPSAMPLE
CALL :fxc_ps "%root%%vs_cb0%%t0%" Downsample_PS ShaderPS.hlsl /DEFFECT_DOWNSAMPLE

CALL :fxc_ps "%root%%vs_cb0%%ps_cb1%%t2%%t4%" Shadow_PS ShaderPS.hlsl /DEFFECT_SHADOW /DPAINT_SOLID
CALL :fxc_ps "%root%%vs_cb0%%ps_cb1%%t2%%t4%" Blur_PS ShaderPS.hlsl /DEFFECT_BLUR /DPAINT_SOLID

:: Compress
Pack Shaders.tar Shaders.z
bin2h.py Shaders.z Shaders >> Shaders_xs.h

del Shaders.z
del Shaders.tar

EXIT /B 0

:: -------------------------------------------------------------------------------------------------------
:fxc_vs
    :: DXIL not stripped from vertex shaders because it is needed for custom pixel shaders
    "%GXDKLatest%bin\Scarlett\dxc" -T vs_6_0 -nologo -O3 -Zi -Fd pdb\ -Qstrip_reflect -Qstrip_rootsignature -D__XBOX_DX12_ROOT_SIGNATURE=%1 -Fo %2.o %3 %4 %5 %6 %7 %8 %9 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %2.o >> Shaders_xs.h
    del %2.o
    EXIT /B 0

:fxc_ps
    "%GXDKLatest%bin\Scarlett\dxc" -T ps_6_0 -nologo -O3 -Zi -Fd pdb\ -Qstrip_reflect -Qstrip_rootsignature -D__XBOX_STRIP_DXIL -D__XBOX_DX12_ROOT_SIGNATURE=%1 -Fo %2.o %3 %4 %5 %6 %7 %8 %9 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %2.o >> Shaders_xs.h
    del %2.o
    EXIT /B 0