@echo off
SETLOCAL EnableExtensions EnableDelayedExpansion
::
:: ========================================================================================================
:: ==== ICU Library compilation Cygwin/MSVC
:: ========================================================================================================
:: 
:: This script can be used to compile the ICU Library with Cygwin/MSVC.
::
:: The script builds all (8) permutations of ICU:
::              (x86 | x64) - (static | shared) - (debug | release)
:: 
:: It is still under development and you have to configure it (see <CONFIGURATION> section below)
:: 
::    Author: stathis <stathis@npcglib.org>
::  Revision: $Id: build-icu.bat 4564 2013-08-05 10:57:31Z stathis $
::
:: Changelog:
::
:: - Cygwin is added before VS in the path. This is necessary to use Cygwin's linked instead of VS.
:: - multiple cores at make time are no longer supported (error building data on some platforms)
:: - Added the ability to compile different flavors of the library (see usage)
:: - Removed --with-data-packaging=static, auto is used instead (thx Yi Z.)
:: - Added double quotes to handle a problem with VISUAL_STUDIO_VC path containing spaces (thx Yi Z.)
:: - Improved the directory naming to include the VS version used. (works ok with VS2010 and VS2012)
:: - Fixed a bug that caused the debug built libraries to link against the wrong runtime. (thx Robert M.)
:: - ICU's "make install" doesn't deploy the PDB files, we copy them ourselves. (thx Robert M.)
::
:: ========================================================================================================
:: ==== <CONFIGURATION>
:: ========================================================================================================

:: Set the version of Visual Studio. This will just add a suffix to the string
:: of your directories to avoid mixing them up.
SET VS_VERSION=vs2013

:: Set this to the directory that contains vcvarsall.bat file of the 
:: VC Visual Studio version you want to use for building ICU.
SET VISUAL_STUDIO_VC=C:\Program Files (x86)\VC

:: Set this to the name of the project
SET BUILD_PROJECT=icu

:: Set this to the version of ICU you are building
SET PROJECT_VERSION=51.2

:: Set this to the Windows directory that contains the toplevel ICU 
:: distribution (one level up from the icu/source directory)
SET PROJECT_SOURCE=E:\_Work\icu4c-51_2-src\icu

:: The installation path (this must be a Windows type path (i.e. C:\icu-dist)
:: This is the location ICU will be installed in, once it is build
SET PREFIX_WIN=E:\_Work\icu-dist-!PROJECT_VERSION!-!VS_VERSION!

:: Set CYGWIN_DIR to the location of your Cygwin root ( one level up from /bin )
:: I use Cygwin Portable (http://symbiosoft.net/projects/cygwin-portable)
SET CYGWIN_DIR=C:\cygwin

:: Set here the 7z command line to use for creating packages
:: Use an absolute path to the 7z.exe utility if it is not in your path.
:: -mx0 store compression
:: -mx9 ultimate compression
SET SEVENZIP_CMD=7z a -r -mx9

:: Set here the md5sum command, which is used to create md5 checksums for the
:: archives after packaging
:: Use an absolute path to the md5sum.exe utility if it is not in your path.
:: This script uses md5sum included in Cygwin 
SET MD5SUM_CMD=!CYGWIN_DIR!\bin\md5sum

:: ========================================================================================================
:: ==== </CONFIGURATION>
:: ========================================================================================================
::
:: Set this to the Windows directory where you want ICU to be built in.
:: You will be able to get rid of this directory completely after packaging.
:: Useful for doing out-of-source builts.
::
:: ATTENTION: this is down here because out-of-source builds are not supported 
:: DO NOT CHANGE IT!
::
SET PROJECT_BUILD=!PROJECT_SOURCE!\build

if not exist !PROJECT_SOURCE!\source (
    call :exitB "Source directory !PROJECT_SOURCE! does not exist or does not contain the !BUILD_PROJECT! sources."
    goto :eof
)

if not exist !CYGWIN_DIR!\bin\make.exe (
    call :exitB "Either !CYGWIN_DIR! is not the Cygwin root, or you have to install maketools."
    goto :eof
)

if not exist "!VISUAL_STUDIO_VC!\vcvarsall.bat" (
    call :exitB "!VISUAL_STUDIO_VC!\vcvarsall.bat does not exist."
    goto :eof
)


SET PATH=!CYGWIN_DIR!\bin;%PATH%


set argC=0
for %%x in (%*) do Set /A argC+=1

if /i "%1" == "all" (
    call :buildall
) else if /i "%1" == "package" (
    call :createPackage
) else if !argC! == 3 (
    goto :callArch %1 %2 %3
    goto :eof
) else (
    goto usage
)

ENDLOCAL
@exit /B 0




:callArch
set archGood=false
if /i "%1" == "x86" set archGood=true
if /i "%1" == "x64" set archGood=true
if /i "!archGood!" == "true" (

    set linkGood=false
    if /i "%2"=="static" set linkGood=true
    if /i "%2"=="shared" set linkGood=true

    if /i "!linkGood!" == "true" (

        set buildGood=false
        if /i "%3" == "debug" set buildGood=true
        if /i "%3" == "release" set buildGood=true

        if /i "!buildGood!" == "true" (
        
            call :build %1 %2 %3
            goto :eof
            
        )
    )
    
)
goto usage
goto :eof


:usage
echo:
echo Error in script usage. The correct usage is:
echo:
echo     %0 all - builds all permutations
echo     %0 package - creates a package file
echo     %0 [x86^|x64] [static^|shared] [debug^|release] - builds specific version
echo:    
goto :eof



:buildall

for %%a in (x86 x64) do (

    for %%l in (shared static) do (

        for %%b in (debug release) do (
        
            call :build %%a %%l %%b
            
        )
    )
)

goto :eof



:: call :build <x86|x64> <static|shared> <debug|release>
:build
SET __ARCH=%~1
SET __LINK=%~2
SET __BUILD=%~3

if /i "!__ARCH!" == "x86" (
    SET BITS=32
    SET BIT_STR=
) else (
    SET BITS=64
    SET BIT_STR=64
    SET __ARCH=x86_amd64
)

echo:
echo Building ICU Library [!__ARCH!] [!__LINK!] [!__BUILD!]
echo:

SETLOCAL EnableExtensions EnableDelayedExpansion
call "!VISUAL_STUDIO_VC!\vcvarsall.bat" !__ARCH!

call :buildtype !__ARCH! !__LINK! !__BUILD!
ENDLOCAL
goto :eof


:createPackage
echo:
echo Packaging ICU Library
echo:
    
for %%l in (shared static) do (

    SET DIST_DIR=!PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%%l-!VS_VERSION!
    
    echo !DIST_DIR!
    
    @mkdir !DIST_DIR!\bin 2>nul
    @mkdir !DIST_DIR!\bin64 2>nul
    @mkdir !DIST_DIR!\lib 2>nul
    @mkdir !DIST_DIR!\lib64 2>nul
    @mkdir !DIST_DIR!\data 2>nul
    @mkdir !DIST_DIR!\include 2>nul
    
    call :packagetype %%l
    echo:
)

ENDLOCAL
@exit /B 0




:: %1 library type (e.g. static)
:packagetype
SET HEADER_COPIED=0
SET DATA_COPIED=0
for %%a in (x86 x64) do (
    for %%b in (debug release) do (
        if /i "%%a" == "x86" (

            SET SRC_DIST_DIR=!PREFIX_WIN!\!BUILD_PROJECT!-%%b-%1-win32-!VS_VERSION!
            SET DST_DIST_DIR=!PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!
        
            if exist !SRC_DIST_DIR! (
                xcopy /Q /Y /S !SRC_DIST_DIR!\bin !DST_DIST_DIR!\bin 1>nul
                xcopy /Q /Y /S !SRC_DIST_DIR!\lib !DST_DIST_DIR!\lib 1>nul
                                
                if !HEADER_COPIED! == 0 (
                    xcopy /Q /Y /S !SRC_DIST_DIR!\include !DST_DIST_DIR!\include 1>nul
                    SET HEADER_COPIED=1
                )
                
                if !DATA_COPIED! == 0 (
                    xcopy /Q /Y /S !SRC_DIST_DIR!\data\icudt*l.dat !DST_DIST_DIR!\data\ 1>nul
                    SET DATA_COPIED=1
                )
                
            )
            
        ) else (

            SET SRC_DIST_DIR=!PREFIX_WIN!\!BUILD_PROJECT!-%%b-%1-win64-!VS_VERSION!
            SET DST_DIST_DIR=!PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!
        
            if exist !SRC_DIST_DIR! (
                xcopy /Q /Y /S !SRC_DIST_DIR!\bin !DST_DIST_DIR!\bin64 1>nul
                xcopy /Q /Y /S !SRC_DIST_DIR!\lib !DST_DIST_DIR!\lib64 1>nul
                
                if !HEADER_COPIED! == 0 (
                    xcopy /Q /Y /S !SRC_DIST_DIR!\include !DST_DIST_DIR!\include 1>nul
                    SET HEADER_COPIED=1
                )
                
                if !DATA_COPIED! == 0 (
                    xcopy /Q /Y /S !SRC_DIST_DIR!\data\icudt*l.dat !DST_DIST_DIR!\data\ 1>nul
                    SET DATA_COPIED=1
                )

            )
            
        )
    )
)

echo Copied all files for: [%1] !BUILD_PROJECT! v!PROJECT_VERSION!

set README=!PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!\readme.txt
set DATE_FORMAT="%%d %%b %%Y"

!CYGWIN_DIR!\bin\date +!DATE_FORMAT! > !README!
echo ====================================================================================================================== >> !README!
echo These are the pre-built %1 ICU Libraries v!PROJECT_VERSION!. They are compiled with Cygwin/MSVC  >> !README!
echo for 32/64-bit Windows, using Visual Studio !VS_VERSION!. >> !README!
echo: >> !README!
echo When using them you may need to specify an environment variable ICU_DATA pointing to the data/ folder. >> !README!
echo This is where the icudtXXl.dat file lives. >> !README!
echo     e.g. set ICU_DATA=F:\icu\data >> !README!
echo ====================================================================================================================== >> !README!
echo If you have any comments or problems send me an email at: >> !README!
echo stathis ^<stathis@npcglib.org^> >> !README!
    

set COMPRESSED_FILE=!PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!.7z

!SEVENZIP_CMD! !COMPRESSED_FILE! !PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION! 1> nul

echo Compressing in: !COMPRESSED_FILE!

IF EXIST !COMPRESSED_FILE! (
    pushd !PREFIX_WIN!
    
    for %%I in (!COMPRESSED_FILE!) do (
        SET /A _fsize=%%~zI / 1024 / 1024
    )
    
    !MD5SUM_CMD! !BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!.7z 1> !BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!.md5
    
    echo Generated md5sum for !PREFIX_WIN!\!BUILD_PROJECT!-!PROJECT_VERSION!-%1-!VS_VERSION!.md5 [!_fsize!MB]
    popd
)
goto :eof


:: %1 architecture (e.g. x86)
:: %2 library type (e.g. static)
:: %3 build type (e.g. release)
:buildtype

if /i "%1" == "x86" (
    SET BITS=32
) else (
    SET BITS=64
)

SET BTDIR=!BUILD_PROJECT!-%3-%2-win!BITS!-!VS_VERSION!

SET INSTALL_DIR_WIN=!PREFIX_WIN!\!BTDIR!

SET BUILD_DIR_DEST=!PROJECT_BUILD!\!BTDIR!

if not exist !BUILD_DIR_DEST! (
    mkdir !BUILD_DIR_DEST!
)


set INSTALL_DIR_CYGWIN=!INSTALL_DIR_WIN:~0,1!
call :toLower INSTALL_DIR_CYGWIN
set INSTALL_DIR_CYGWIN=/cygdrive/!INSTALL_DIR_CYGWIN!!INSTALL_DIR_WIN:~2!
set INSTALL_DIR_CYGWIN=!INSTALL_DIR_CYGWIN:\=/!

set UNIX_PROJECT_SOURCE_DRIVE=!PROJECT_SOURCE:~0,1!
call :toLower UNIX_PROJECT_SOURCE_DRIVE
set UNIX_PROJECT_SOURCE_PATH=/cygdrive/!UNIX_PROJECT_SOURCE_DRIVE!/!PROJECT_SOURCE:~3!

SET LOG_FILE=!INSTALL_DIR_WIN!\!BTDIR!.log

pushd !BUILD_DIR_DEST!
    
    SET B_CMD=../../source/runConfigureICU
    
    IF /i "%3" == "debug" (
        SET B_CMD=!B_CMD! --enable-debug --disable-release
    ) 

    SET B_CMD=!B_CMD! Cygwin/MSVC --prefix=!INSTALL_DIR_CYGWIN! 
    
    IF /i "%2" == "static" (
        SET B_CMD=!B_CMD! --enable-static --disable-shared
    ) ELSE (
        SET B_CMD=!B_CMD! --enable-shared --disable-static
    )
    
    IF /i "%3" == "debug" (
        SET B_CMD=!B_CMD! --enable-debug --disable-release
    ) ELSE (
        SET B_CMD=!B_CMD! --enable-release --disable-debug
    )
    
    
    echo ------------------------------
    echo Configuring: !BTDIR!
    echo Building in: !BUILD_DIR_DEST!
    echo Install Dir: !INSTALL_DIR_CYGWIN!
    echo Install Win: !INSTALL_DIR_WIN!
    echo -Logging in: !LOG_FILE!
    echo ----Command: !B_CMD!
    echo ------------------------------

    IF NOT EXIST !INSTALL_DIR_WIN! (
        mkdir !INSTALL_DIR_WIN!
    )
    
        
    bash -c "!B_CMD!"  2>&1 > !BTDIR!.log
    
    make 2>&1 >> !BTDIR!.log 
    
    IF NOT EXIST !INSTALL_DIR_WIN!\data (
        mkdir !INSTALL_DIR_WIN!\data
    )

    copy /Y data\out\tmp\icudt*l.dat !INSTALL_DIR_WIN!\data\ 1>nul

    SETLOCAL
    SET ICU_DATA=!INSTALL_DIR_WIN!\data\
    make check >> !BTDIR!.log 2>&1
    ENDLOCAL
    
    make install 2>&1 >> !BTDIR!.log 
    
    IF /i "%2" == "shared" (
    
        xcopy /Q /Y /S !BUILD_DIR_DEST!\lib\*.exp !INSTALL_DIR_WIN!\lib\ 1>nul
        
        IF /i "%3" == "debug" (
            xcopy /Q /Y /S !BUILD_DIR_DEST!\lib\*.pdb !INSTALL_DIR_WIN!\lib\ 1>nul
        )

    )
    
    copy !BTDIR!.log !LOG_FILE!

popd

goto :eof


:toLower str -- converts uppercase character to lowercase
::           -- str [in,out] - valref of string variable to be converted
:$created 20060101 :$changed 20080219 :$categories StringManipulation
:$source http://www.dostips.com
if not defined %~1 EXIT /b
for %%a in ("A=a" "B=b" "C=c" "D=d" "E=e" "F=f" "G=g" "H=h" "I=i"
            "J=j" "K=k" "L=l" "M=m" "N=n" "O=o" "P=p" "Q=q" "R=r"
            "S=s" "T=t" "U=u" "V=v" "W=w" "X=x" "Y=y" "Z=z" "Ä=ä"
            "Ö=ö" "Ü=ü") do (
    call set %~1=%%%~1:%%~a%%
)
EXIT /b

:: %1 an error message
:exitB
echo:
echo Error: %1
echo:
echo Contact stathis@npcglib.org
