@echo off

setlocal enabledelayedexpansion

rem VS 2017
set VCVARSALL="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS130COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found
set VCVARSALL="%VS80COMNTOOLS%\..\..\VC\vcvarsall.bat"
if exist %VCVARSALL% goto found

exit /B 1

:found

if "%3"=="32" (
  set ARCH=32
)
if "%3"=="64" (
  set ARCH=64
)

if not "%ARCH%"=="" (
  if not "%4"=="" goto start
)

echo Usage: %0 openssl_archive openssl_version [32^|64] destination_dir 1>&2
exit /B 1

:start

set WORKING_DIR=%cd%
set OPENSSL_ARCHIVE=%~f1
set OPENSSL_VERSION=%2
set OPENSSL_SOURCE=%cd%\openssl-%OPENSSL_VERSION%
set DIST_DIR=%~f4
echo Using OpenSSL archive %OPENSSL_ARCHIVE% (%OPENSSL_VERSION%) into %OPENSSL_SOURCE%
echo Destination dir: %DIST_DIR%

set BASE_DIR=%~dp0
set OPENSSL_BUILD_DIR=%cd%\openssl.build%ARCH%

set PATH=%PATH%;C:\Program Files\7-Zip

if not exist %OPENSSL_SOURCE% (
  echo [*] Decompressing %OPENSSL_ARCHIVE%
  7z x -so %OPENSSL_ARCHIVE% |  7z x -si -aoa -ttar
)

echo [*] Configuring OpenSSL
cd /D %OPENSSL_SOURCE%

if "%ARCH%"=="64" (
  perl Configure VC-WIN64A
  call ms\do_win64a.bat
  perl -i.old -p -e "s#^(LFLAGS=.*$)#$1 /DYNAMICBASE#" ms\nt.mak
)
if "%ARCH%"=="32" (
  perl Configure VC-WIN32
  call ms\do_nasm.bat
  perl -i.old -p -e "s#^(ASM=.*$)#$1 -safeseh#;s#^(LFLAGS=.*$)#$1 /DYNAMICBASE /SAFESEH#" ms\nt.mak
)
  
echo [*] Building OpenSSL
mkdir %OPENSSL_BUILD_DIR% 2> NUL
mkdir %DIST_DIR%\include\openssl 2> NUL
mkdir %DIST_DIR%\lib 2> NUL
setlocal
if "%ARCH%"=="64" call %%VCVARSALL%% amd64
if "%ARCH%"=="32" call %%VCVARSALL%% x86
cd /D %OPENSSL_SOURCE%
nmake /f ms\nt.mak OUT_D=%DIST_DIR%\lib TMP_D=%OPENSSL_BUILD_DIR% INC_D=%DIST_DIR%\include INCO_D=%DIST_DIR%\include\openssl headers lib
cd /D %WORKING_DIR%
endlocal
