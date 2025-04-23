@echo off
setlocal
set EXE=MergeSortParallel1.exe

:: Șterge fișierul results.txt dacă există deja
echo > results.txt

for /L %%i in (1,1,10) do (
    echo Running data%%i.txt >> results.txt
    echo Running data%%i.txt
    mpiexec -n 8 %EXE% data%%i.txt >> results.txt
    echo. >> results.txt
)

echo Toate rularile au fost salvate in results.txt
endlocal
pause