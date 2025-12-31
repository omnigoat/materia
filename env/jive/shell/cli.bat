@echo off
setlocal enabledelayedexpansion

:: add the base-dir of the repo to our path.
:: it can be referenced as REPO_BASE_DIR
call :normalizePath REPO_BASE_DIR "%~p0..\..\"
set 'PATH=\"%PATH%;%REPO_BASE_DIR%env\cli\internal\"'

:: restore nuget packages
call dotnet restore >NUL 2>&1

:: build mantle
call dotnet build --configuration Release %REPO_BASE_DIR%env\mantle\mantle.sln

:: run our cli
call %REPO_BASE_DIR%env\mantle\build\Release\net9.0\mantle.exe

goto :eof

:: normalize path
:normalizePath
if "%~2"=="" (
    SET "normalizedPath=%~f1"
)
else (
    SET "%~1=%~f2"
)
goto :eof
