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

if exist ".\vendor\frasy\Frasy\lua\core" (
    echo Copying .\vendor\frasy\Frasy\lua\core...
    robocopy /MIR /NFL /NDL /NJH /NJS /NC /NS ".\vendor\frasy\Frasy\lua\core" "%DEST_DIR%\lua\core"
) else (
    echo Warning: .\vendor\frasy\Frasy\lua\core not found
)

if exist ".\vendor\frasy\Frasy\assets" (
    echo Copying .\vendor\frasy\Frasy\assets...
    robocopy /MIR /NFL /NDL /NJH /NJS /NC /NS ".\vendor\frasy\Frasy\assets" "%DEST_DIR%\assets"
) else (
    echo Warning: .\vendor\frasy\Frasy\assets not found
)

if exist ".\src\lua\user" (
    echo Copying .\src\lua...
    robocopy /MIR /NFL /NDL /NJH /NJS /NC /NS ".\src\lua\user" "%DEST_DIR%\lua\user"
) else (
    echo Warning: .\src\lua\user not found
)

echo Done.
exit /b 0
