; This script builds the Windows installer for Renamifier.
;
; Extract the GhostXPS distribution file to the build directory:
; ghostxps-10.01.0-win64.zip                (for the 64-bit version)
; ghostxps-10.01.0-win32.zip                (for the 32-bit version)
;
; Usage:
; makensis winsetup.nsi                     (for the 64-bit version)
; makensis /DPLATFORM=win32 winsetup.nsi    (for the 32-bit version)

!define MULTIUSER_EXECUTIONLEVEL Highest
!include MultiUser.nsh

; Sync this with renamifier.h
!ifndef VERSION
  !define VERSION "0.1.2"
!endif

; This can be "win32" or "win64"
!ifndef PLATFORM
  !define PLATFORM "win64"
!endif

!if ${PLATFORM} == "win64"
  !define MINGW_DIR "C:\msys64\mingw64"
  !define GXPS_DIR "ghostxps-10.01.0-win64"
  InstallDir "$PROGRAMFILES64\Renamifier"

!else if ${PLATFORM} == "win32"
  !define MINGW_DIR "C:\msys64\mingw32"
  !define GXPS_DIR "ghostxps-10.01.0-win32"
  InstallDir "$PROGRAMFILES32\Renamifier"

!else
  !error "Platform must be one of 'win32' or 'win64'."

!endif

; ------------------------------------------------------------------------

Name "Renamifier"
OutFile "renamifier-${VERSION}-${PLATFORM}-setup.exe"
InstallDirRegKey HKLM "Software\Benjamin Johnson\Renamifier" ""
RequestExecutionLevel admin
XPStyle on
LicenseData "COPYING"

; ------------------------------------------------------------------------

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; ------------------------------------------------------------------------

Section "Renamifier"
  SectionIn RO

  SetOutPath $INSTDIR

  ; Remove older versions of GhostXPS
  RMDir /r "$INSTDIR\ghostxps-9.53.3-win32"
  RMDir /r "$INSTDIR\ghostxps-9.53.3-win64"
  RMDir /r "$INSTDIR\ghostxps-10.0.0-win32"
  RMDir /r "$INSTDIR\ghostxps-10.0.0-win64"

  ; Remove older versions of dependencies
  Delete "$INSTDIR\libicudt68.dll"
  Delete "$INSTDIR\libicuin68.dll"
  Delete "$INSTDIR\libicuuc68.dll"
  Delete "$INSTDIR\libpcre-1.dll"
  Delete "$INSTDIR\libpoppler-105.dll"

  ; Include the application files
  File renamifier.exe

  ; Include Qt dependencies
  File *.dll
  File /r iconengines
  File /r imageformats
  File /r platforms
  File /r styles
  File /r translations

  ; Include other library dependencies
  ;
  ; FIXME: This list was manually generated by running `ldd renamifier.exe`.
  ; There has to be a better way to identify and package dependencies.
  File "${MINGW_DIR}\bin\libLerc.dll"
  File "${MINGW_DIR}\bin\libbrotlicommon.dll"
  File "${MINGW_DIR}\bin\libbrotlidec.dll"
  File "${MINGW_DIR}\bin\libbz2-1.dll"
  !if ${PLATFORM} == "win64"
    File "${MINGW_DIR}\bin\libcrypto-1_1-x64.dll"
  !else
    File "${MINGW_DIR}\bin\libcrypto-1_1.dll"
  !endif
  File "${MINGW_DIR}\bin\libcurl-4.dll"
  File "${MINGW_DIR}\bin\libdeflate.dll"
  File "${MINGW_DIR}\bin\libdouble-conversion.dll"
  File "${MINGW_DIR}\bin\libfreetype-6.dll"
  File "${MINGW_DIR}\bin\libglib-2.0-0.dll"
  File "${MINGW_DIR}\bin\libgraphite2.dll"
  File "${MINGW_DIR}\bin\libharfbuzz-0.dll"
  File "${MINGW_DIR}\bin\libiconv-2.dll"
  File "${MINGW_DIR}\bin\libicudt71.dll"
  File "${MINGW_DIR}\bin\libicuin71.dll"
  File "${MINGW_DIR}\bin\libicuuc71.dll"
  File "${MINGW_DIR}\bin\libidn2-0.dll"
  File "${MINGW_DIR}\bin\libintl-8.dll"
  File "${MINGW_DIR}\bin\libjbig-0.dll"
  File "${MINGW_DIR}\bin\libjpeg-8.dll"
  File "${MINGW_DIR}\bin\liblcms2-2.dll"
  File "${MINGW_DIR}\bin\liblzma-5.dll"
  File "${MINGW_DIR}\bin\libmd4c.dll"
  File "${MINGW_DIR}\bin\libnghttp2-14.dll"
  File "${MINGW_DIR}\bin\libnspr4.dll"
  File "${MINGW_DIR}\bin\libopenjp2-7.dll"
  File "${MINGW_DIR}\bin\libpcre2-8-0.dll"
  File "${MINGW_DIR}\bin\libpcre2-16-0.dll"
  File "${MINGW_DIR}\bin\libplc4.dll"
  File "${MINGW_DIR}\bin\libplds4.dll"
  File "${MINGW_DIR}\bin\libpng16-16.dll"
  File "${MINGW_DIR}\bin\libpoppler-122.dll"
  File "${MINGW_DIR}\bin\libpoppler-qt5-1.dll"
  File "${MINGW_DIR}\bin\libpsl-5.dll"
  File "${MINGW_DIR}\bin\libssh2-1.dll"
  !if ${PLATFORM} == "win64"
    File "${MINGW_DIR}\bin\libssl-1_1-x64.dll"
  !else
    File "${MINGW_DIR}\bin\libssl-1_1.dll"
  !endif
  File "${MINGW_DIR}\bin\libtiff-5.dll"
  File "${MINGW_DIR}\bin\libunistring-2.dll"
  File "${MINGW_DIR}\bin\libwebp-7.dll"
  File "${MINGW_DIR}\bin\libzstd.dll"
  File "${MINGW_DIR}\bin\nss3.dll"
  File "${MINGW_DIR}\bin\nssutil3.dll"
  File "${MINGW_DIR}\bin\smime3.dll"
  File "${MINGW_DIR}\bin\zlib1.dll"

  !if ${PLATFORM} == "win64"
    SetRegView 64
  !else if ${PLATFORM} == "win32"
    SetRegView 32
  !endif

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\Benjamin Johnson\Renamifier" "" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Renamifier" "DisplayName" "Renamifier"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Renamifier" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Renamifier" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Renamifier" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

SectionGroup "File format support"
  Section "XPS support via GhostXPS"
    File /r ${GXPS_DIR}
  SectionEnd
SectionGroupEnd

SectionGroup "Create shortcuts"
  Section "Start menu shortcut"
    CreateShortcut "$SMPROGRAMS\Renamifier.lnk" "$INSTDIR\renamifier.exe" "" "$INSTDIR\renamifier.exe" 0
  SectionEnd

  Section "Desktop shortcut"
    CreateShortcut "$DESKTOP\Renamifier.lnk" "$INSTDIR\renamifier.exe" "" "$INSTDIR\renamifier.exe" 0
  SectionEnd
SectionGroupEnd

; ------------------------------------------------------------------------

Section "Uninstall"
  !if ${PLATFORM} == "win64"
    SetRegView 64
  !else if ${PLATFORM} == "win32"
    SetRegView 32
  !endif

  ; Delete registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Renamifier"
  DeleteRegKey HKLM "Software\Benjamin Johnson\Renamifier"

  ; Delete shortcuts
  Delete "$DESKTOP\Renamifier.lnk"
  Delete "$SMPROGRAMS\Renamifier.lnk"

  ; Delete application files
  RMDir /r "$INSTDIR\${GXPS_DIR}"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\translations"
  Delete "$INSTDIR\renamifier.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\uninstall.exe"

  ; Remove the application directory if it's empty
  RMDir "$INSTDIR"
SectionEnd

; ------------------------------------------------------------------------

Function .onInit
  !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd
