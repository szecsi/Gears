SetLocal EnableDelayedExpansion

set SolutionDir=%1
set ProjectDir=%2
set Configuration=%3
set OutDir=%4
set IntDir=%5

echo Packaging with PyInstaller....
echo Writing to:"%OutDir%

rem python D:\Development\PyInstaller-3.2\pyinstaller.py  %SolutionDir%GearsPy\Gears.pyw --distpath %OutDir% --workpath %IntDir% -y --exclude-module Project --hidden-import six --hidden-import packaging --hidden-import packaging.version --hidden-import packaging.specifiers
pyinstaller  %SolutionDir%GearsPy\Gears.pyw --distpath %OutDir% --workpath %IntDir% -y --exclude-module Project --hidden-import six --hidden-import packaging --hidden-import packaging.version --hidden-import packaging.specifiers

if not exist "%OutDir%\Gears\Project\" (mkdir "%OutDir%Gears\Project\")

xcopy %SolutionDir%GearsPy\Project %OutDir%Gears\Project /y /f /exclude:%ProjectDir%excludeFiles.txt  /e
xcopy %ProjectDir%dlls %OutDir%Gears /y /f /e