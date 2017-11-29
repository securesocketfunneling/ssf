@echo off

setlocal enabledelayedexpansion

set BOOST_LIBRARIES=system,date_time,filesystem,thread,chrono

if "%3"=="32" (
  set ARCH=32
)
if "%3"=="64" (
  set ARCH=64
)

if not "%ARCH%"=="" (
  if not "%4"=="" goto start
)

echo Usage: %0 boost_archive_path boost_version [32^|64] destination_dir 1>&2
exit /B 1

:start
set WORKING_DIR=%cd%
set BOOST_ARCHIVE=%~f1
set BOOST_VERSION=%2
set BOOST_SOURCE=%cd%\boost_%BOOST_VERSION%
echo Using Boost archive %BOOST_ARCHIVE% (%BOOST_VERSION%) into %BOOST_SOURCE%
set DIST_DIR=%~f4
echo Destination dir: %DIST_DIR%
set BASE_DIR=%~dp0
set BOOST_BUILD_DIR=%cd%\boost.build%ARCH%
set BOOST_STAGE_DIR=%cd%\boost.stage%ARCH%

set PATH=%PATH%;C:\Program Files\7-Zip

if not exist %BOOST_SOURCE% (
  echo [*] Decompressing %BOOST_ARCHIVE%
  7z x -so %BOOST_ARCHIVE% |  7z x -si -aoa -ttar
)

if not exist %BOOST_SOURCE%/b2.exe (
  echo [*] Bootstrapping Boost
  cd /D %BOOST_SOURCE%
  call bootstrap.bat
)

echo [*] Building Boost

set B2_ARGS=--build-dir=%BOOST_BUILD_DIR% --stagedir=%BOOST_STAGE_DIR% -j%NUMBER_OF_PROCESSORS%
for %%l in (%BOOST_LIBRARIES%) do (
  set B2_ARGS=!B2_ARGS! --with-%%l
)
set B2_ARGS=!B2_ARGS! link=static runtime-link=static variant=debug,release address-model=%ARCH% cxxflags="-GR-"
if "%ARCH%"=="32" (
  set B2_ARGS=!B2_ARGS! asmflags=\safeseh
)

cd /D %BOOST_SOURCE%
mkdir %BOOST_BUILD_DIR% 2> NUL
b2.exe %B2_ARGS% --prefix=%DIST_DIR% stage install

cd /D %WORKING_DIR%