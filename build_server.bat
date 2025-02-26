@echo off

set annoying_warnings=/wd4189 /wd4996 /wd4100 /wd4244 /wd4255 /wd5045 /wd4711 /wd4710 /wd4820 /wd4514

cl /nologo /Z7 /diagnostics:caret /sdl /fsanitize=address /Wall /WX /W4 %annoying_warnings% /MD ..\server.c /link /out:server.exe

if %ERRORLEVEL% equ 0 (
  server
)
