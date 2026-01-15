; HelloWorld Windows Installer
; NSIS Script

!include "MUI2.nsh"
!include "FileFunc.nsh"

Name "HelloWorld"
OutFile "..\dist\HelloWorld-Setup.exe"
InstallDir "$PROGRAMFILES64\HelloWorld"
InstallDirRegKey HKLM "Software\HelloWorld" "InstallDir"
RequestExecutionLevel admin

!define MUI_ICON "..\client\resources\icon.ico"
!define MUI_UNICON "..\client\resources\icon.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "HelloWorld Setup"
!define MUI_WELCOMEPAGE_TEXT "This will install HelloWorld on your computer.$\r$\n$\r$\nHelloWorld is a DPI-resistant encrypted tunnel.$\r$\n$\r$\nClick Next to continue."
!define MUI_FINISHPAGE_RUN "$INSTDIR\helloworld.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch HelloWorld"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    
    File "..\build\Release\helloworld.exe"
    File "..\client\resources\icon.ico"
    File /oname=stunnel.exe "..\deps\stunnel\stunnel.exe"
    File /oname=stunnel.conf "..\deps\stunnel\stunnel.conf"
    
    SetOutPath "$INSTDIR\docs"
    File "..\README.md"
    File "..\docs\SETUP_SERVER.md"
    
    WriteRegStr HKLM "Software\HelloWorld" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                     "DisplayName" "HelloWorld"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                     "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                     "DisplayIcon" "$INSTDIR\icon.ico"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                     "Publisher" "HelloWorld Project"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                     "DisplayVersion" "1.0.0"
    
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld" \
                       "EstimatedSize" "$0"
    
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    CreateDirectory "$SMPROGRAMS\HelloWorld"
    CreateShortcut "$SMPROGRAMS\HelloWorld\HelloWorld.lnk" "$INSTDIR\helloworld.exe" "" "$INSTDIR\icon.ico"
    CreateShortcut "$SMPROGRAMS\HelloWorld\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$DESKTOP\HelloWorld.lnk" "$INSTDIR\helloworld.exe" "" "$INSTDIR\icon.ico"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\helloworld.exe"
    Delete "$INSTDIR\stunnel.exe"
    Delete "$INSTDIR\stunnel.conf"
    Delete "$INSTDIR\icon.ico"
    Delete "$INSTDIR\uninstall.exe"
    RMDir /r "$INSTDIR\docs"
    RMDir "$INSTDIR"
    
    Delete "$SMPROGRAMS\HelloWorld\HelloWorld.lnk"
    Delete "$SMPROGRAMS\HelloWorld\Uninstall.lnk"
    RMDir "$SMPROGRAMS\HelloWorld"
    Delete "$DESKTOP\HelloWorld.lnk"
    
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HelloWorld"
    DeleteRegKey HKLM "Software\HelloWorld"
SectionEnd

