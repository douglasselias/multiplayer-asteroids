@echo off

cl

if %ERRORLEVEL% neq 0 (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

cls

rmdir /S /Q .\build
mkdir build
pushd .\build

start cmd /k Call ../build_server.bat

set annoying_warnings=/wd4189 /wd4996 /wd4100 /wd4244 /wd4255 /wd5045 /wd4711 /wd4710 /wd4820 /wd4514 /wd4101

cl /nologo /Z7 /diagnostics:caret /sdl /fsanitize=address /Wall /WX /W4 %annoying_warnings% ../main.c /MD /I../vendor /link ../vendor/SDL3.lib ../vendor/SDL3_image.lib /subsystem:windows

if %ERRORLEVEL% equ 0 (
  xcopy /Y ..\vendor\SDL3.dll .
  xcopy /Y ..\vendor\SDL3_image.dll .

  start cmd /k Call main 0
  start cmd /k Call main 1
  @REM start cmd /k Call main 2
  @REM start cmd /k Call main 3
)

popd