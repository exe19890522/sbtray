Name                 "sbtray"
OutFile              "sbtray-installer.exe"
InstallDir           $PROGRAMFILES\sbtray
DirText              "This will install sbtray on your computer.  Choose a directory."
CRCCheck             force
SetCompress          force
SetCompressor        lzma
LicenseData          installer_text.rtf
LicenseText          Overview Continue

Section ""
   SetOutPath        $INSTDIR
   File              "Release\sbtray.exe"
   File              "upsound.wav"
   File              "downsound.wav"
   File              "history.txt"
   File              "readme.txt"
   File              "sbtray_links.txt"
   File              "sbtray_worldlist.txt"

   CreateShortcut    "$STARTMENU\Programs\sbtray.lnk" "$INSTDIR\sbtray.exe"
   CreateShortcut    "$DESKTOP\sbtray.lnk" "$INSTDIR\sbtray.exe"

   WriteUninstaller  Uninst.exe
   WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sbtray" "DisplayName" "sbtray"
   WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sbtray" "UninstallString" "$INSTDIR\Uninst.exe"
SectionEnd

Section "Uninstall"
   RMDir             /r $INSTDIR
   Delete            "$STARTMENU\Programs\sbtray.lnk"
   Delete            "$DESKTOP\sbtray.lnk"
   DeleteRegKey      HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sbtray"
SectionEnd

Function un.onInit
   MessageBox MB_YESNO "This will uninstall sbtray. Continue?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
   NoAbort:
FunctionEnd