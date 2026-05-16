REM Description: This script runs all the BakScript tests in the main directory.
@echo off 
echo Running BakScript Tests
echo.

echo Testing Invalid Programs:
echo ---------------------
for %%f in (error\*.bak) do (
    echo Testing %%f
    ..\batch\bakscript < %%f
    echo.
) 