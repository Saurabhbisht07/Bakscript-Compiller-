REM Description: This script runs all the BakScript tests in the main directory.
@echo off 
echo Running BakScript Tests
echo.

echo Testing Valid Programs:
echo ---------------------
for %%f in (main\*.bak) do (
    echo Testing %%f
    ..\batch\bakscript < %%f
    echo.
)