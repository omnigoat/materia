@echo off

:: restore nuget packages
call dotnet add package Microsoft.CodeAnalysis --version 5.0.0

:: TODO(jrunting) - this doesn't work
call :normalizePath "%~p0..\..\"
set "REPO_BASE_DIR=%normalizedPath%"
set 'PATH=\"%PATH%;%REPO_BASE_DIR%env\cli\internal\"'
call dotnet atma.cs


:: normalize path
:normalizePath
SET "normalizedPath=%~f1"
goto :eof