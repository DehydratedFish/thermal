@echo off

IF NOT EXIST "build" mkdir build

SET path_to_stbtt=""

SET sources="source/thermal.cpp" "source/utf.cpp" "source/io.cpp" "source/font.cpp" "source/renderer.cpp" "source/ui.cpp" "source/win32_platform.cpp"
SET linker="/SUBSYSTEM:CONSOLE" "/INCREMENTAL:NO" "User32.lib" "Ole32.lib" "Shell32.lib" "Shlwapi.lib" "Gdi32.lib" "Opengl32.lib" "Dbghelp.lib" "Onecore.lib"

cl /D"DEVELOPER" /D"BOUNDS_CHECKING" /Isource /I"%path_to_stbtt%" /FC /Zi /nologo /W2 /permissive- /Fo"build/debug/" /Fd"build/debug/" /Fe"build/debug/thermal.exe" %sources% /link %linker%

IF NOT EXIST "build\debug\data" mkdir build\debug\data
xcopy /eyq "data" "build\debug\data"

