
@echo off

cls

where /q cl.exe
if ERRORLEVEL 1 (
	@REM Replace with your path
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

if not exist build\NUL mkdir build

set files=..\src\main.cpp
set comp=-nologo -std:c++20 -Zc:strictStrings- -W4 -wd4505 -wd4127 -wd4324 -FC -Dm_app -D_CRT_SECURE_NO_WARNINGS
set linker=user32.lib Shell32.lib Winhttp.lib ole32.lib Ws2_32.lib Gdi32.lib Opengl32.lib Shlwapi.lib -INCREMENTAL:NO

set debug=2
if %debug%==0 (
	set comp=%comp% -O2 -MT
)
if %debug%==1 (
	set comp=%comp% -O2 -Dm_debug -MTd
)
if %debug%==2 (
	set comp=%comp% -Od -Dm_debug -Zi -MTd
)

pushd build
	cl %files% %comp% -link %linker% > temp_compiler_output.txt
popd
if %errorlevel%==0 goto success

goto end

:success
copy build\main.exe main.exe > NUL

:end
copy build\temp_compiler_output.txt compiler_output.txt > NUL
type build\temp_compiler_output.txt

