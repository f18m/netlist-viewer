; *************************************************************************
;   This is the NSIS (http://nsis.sf.net) installer of NetlistViewer      *
;   Copyright (C) 2010 by Francesco Montorsi                              *
;   netlistviewer.sourceforge.net                                         *
;                                                                         *
;   This program is free software; you can redistribute it and/or modify  *
;   it under the terms of the GNU General Public License as published by  *
;   the Free Software Foundation; either version 2 of the License, or     *
;   (at your option) any later version.                                   *
;                                                                         *
;   This program is distributed in the hope that it will be useful,       *
;   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
;   GNU General Public License for more details.                          *
;                                                                         *
;   You should have received a copy of the GNU General Public License     *
;   along with this program; if not, write to the                         *
;   Free Software Foundation, Inc.,                                       *
;   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
; *************************************************************************

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

; -------------------------------------------------------------------------------------------------
; General

  ; NOTE: the version should be the same as the one in svn_revision.sh (see the UPP_VERSION #define)
  !define PRODUCT_NAME            "NetlistViewer"
  !define PRODUCT_VERSION         "0.3"
  !define PRODUCT_PUBLISHER       "Francesco Montorsi"
  !define INSTALLER_MODE          "release"     ; choose between "debug" and "release"

  ; Name and file
  Name "${PRODUCT_NAME} ${PRODUCT_VERSION} Installer"
  OutFile "${PRODUCT_NAME}-${PRODUCT_VERSION}-win-installer.exe"
  Icon "..\src\icon.ico"
  Unicode True

  InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
  LicenseData "gnugpl.txt"
  SetCompressor /SOLID lzma    ; this was found to be the best compressor
  
  ; see http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista for more info:
  RequestExecutionLevel admin
  
;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

; -------------------------------------------------------------------------------------------------
; Pages
 ; !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchApplication"

  !insertmacro MUI_PAGE_LICENSE "gnugpl.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
 ; !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
; -------------------------------------------------------------------------------------------------
; Additional info (will appear in the "details" tab of the properties window for the installer)

  VIAddVersionKey "ProductName"      "${PRODUCT_NAME}"
  VIAddVersionKey "Comments"         ""
  VIAddVersionKey "CompanyName"      "${PRODUCT_NAME} Team"
  VIAddVersionKey "LegalTrademarks"  "Application released under the GNU GPL"
  VIAddVersionKey "LegalCopyright"   "© ${PRODUCT_NAME} Team"
  VIAddVersionKey "FileDescription"  "Text to schematic conversion of SPICE netlists"
  VIAddVersionKey "FileVersion"      "${PRODUCT_VERSION}"
  VIProductVersion                   "${PRODUCT_VERSION}.0.0" 

; -------------------------------------------------------------------------------------------------
; Installer Sections

Section "install" ; No components page, name is not important

  ; Set files to be extracted in the user-chosen installation path:

  SetOutPath "$INSTDIR"
  File gnugpl.txt
  File ..\src\icon.ico
  File ..\build\win\x64\Release\netlist_viewer.exe
  File ..\build\win\x64\Release\*.dll  

  SetOutPath "$INSTDIR\examples"
  File /nonfatal ..\examples\*.ckt
  File /nonfatal ..\examples\*.cir
  
  SetOutPath "$INSTDIR\tests"
  File /nonfatal ..\tests\*.ckt
  File /nonfatal ..\tests\*.cir
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Add the uninstaller to the list of programs accessible from "Control Panel -> Programs and Features"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "DisplayIcon" "$\"$INSTDIR\icon.ico$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "URLInfoAbout" "http://netlistviewer.sourceforge.net"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                   "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
                     "EstimatedSize" 5000
                     ; the estimated must be expressed in Kb; for us it's about 5 MB!

  ; Create shortcuts
  SetShellVarContext all        ; see http://nsis.sourceforge.net/Shortcuts_removal_fails_on_Windows_Vista
  SetOutPath "$INSTDIR"         ; this will be the working directory for the shortcuts created below
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\NetlistViewer.lnk" "$INSTDIR\netlist_viewer.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  
SectionEnd

; -------------------------------------------------------------------------------------------------
; Uninstaller Section

Section "un.install"

  ; clean the list accessible from "Control Panel -> Programs and Features"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

  ; clean start menu
  SetShellVarContext all
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\NetlistViewer.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  ; clean installation folder
  Delete "$INSTDIR\uninstall.exe"
  Delete "$INSTDIR\gnugpl.txt"
  Delete "$INSTDIR\icon.ico"
  Delete "$INSTDIR\netlist_viewer.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.manifest"
  Delete "$INSTDIR\examples\*.ckt"
  Delete "$INSTDIR\tests\*.cir"
  RMDir "$INSTDIR\examples"
  RMDir "$INSTDIR\tests"
  RMDir "$INSTDIR"

SectionEnd

Function .oninstsuccess   
  ; launch the application at the end of the install
  Exec "$INSTDIR\netlist_viewer.exe"   
FunctionEnd