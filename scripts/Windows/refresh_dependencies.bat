@echo off
setlocal

if "%~1"=="" (
    echo Error: No destination directory provided
    echo Usage: refresh_dependencies.bat ^<destination_directory^>
    exit /b 1
)

set DEST_DIR=%~1

if not exist "%DEST_DIR%" (
    echo Error: Destination directory does not exist: %DEST_DIR%
    exit /b 1
)

echo Copying dependencies to %DEST_DIR%...

if exist ".\vendor\frasy\Frasy\lua" (
    echo Copying .\vendor\frasy\Frasy\lua...
    xcopy /E /I /Y ".\vendor\frasy\Frasy\lua" "%DEST_DIR%\lua"
) else (
    echo Warning: .\vendor\frasy\Frasy\lua not found
)

if exist ".\vendor\frasy\Frasy\assets" (
    echo Copying .\vendor\frasy\Frasy\assets...
    xcopy /E /I /Y ".\vendor\frasy\Frasy\assets" "%DEST_DIR%\assets"
) else (
    echo Warning: .\vendor\frasy\Frasy\assets not found
)

if exist ".\src\lua" (
    echo Copying .\src\lua...
    xcopy /E /I /Y ".\src\lua" "%DEST_DIR%\lua"
) else (
    echo Warning: .\src\lua not found
)

echo Done.
exit /b 0
