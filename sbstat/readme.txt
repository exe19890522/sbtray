//******************************************************************************
// sbstat - provides periodic status (graphically) of the Shadowbane servers
//        - server status information is obtained from Sancus' status page
//          not directly from any of the Ubisoft servers.
//              http://sancus.off.net/serverup.php
//        - developed for Sun's Java 2 Platform Standard Edition, v1.3.1_06
//        - Java runtime is available for Windows, Solaris, and Linux at:
//              http://java.sun.com
//        - Java runtime is available for MacOS X at:
//              http://www.apple.com/java
//==============================================================================
// build instructions
//        - you must have the Java SDK installed
//        - you must have the source, which includes the following files:
//              ----------------------------------------------------------------
//              support files
//              ----------------------------------------------------------------
//              readme.txt                  - this file
//              run_sbstat_windows.bat      - helper file for running on windows
//              windows_jar_association.reg - allows windows users to run jar
//                                            files by just double-clicking them
//              mainclass                   - used to create the runnable
//                                            jar file
//              upsound.wav                 - sound to play when game server
//                                            comes up
//              downsound.wav               - sound to play when game server
//                                            goes down
//              ----------------------------------------------------------------
//              source files
//              ----------------------------------------------------------------
//              sbstat.java
//              mousehandler.java
//              serverstatus.java
//              serverstatusupdater.java
//        - compile all java sources
//              javac *.java
//        - generate a runnable .jar file
//              jar cmf mainclass sbstat.jar *.class *.wav
//        - you should now be able to run the jar file.  for example:
//              java -jar sbstat.jar
//==============================================================================
// running instructions (General)
//        - by default sounds are enabled.  when the state of the game server
//          changes, a sound is played.  these sounds are the upsound.wav
//          and downsound.wav files contained in the sbstat.jar file.  You
//          can use the context menu to disable the sounds, but you need to
//          do this each time you run sbstat as the choice currently doesn't
//          persist.  you can replace these files with your own sounds if you
//          want.
// running instructions (Windows)
//        - you must have the Java runtime installed (at least v1.3.1_06)
//        - double-click on the run_sbstat_windows.bat file included with
//          sbstat
//        - alternatively, you can double-click on the
//          windows_jar_association.reg file.  this will add the necessary
//          entries to your system registry so that you will be able to run
//          jar files simply by double-clicking them.
// running instructions (MacOS X)
//        - you should be able to just double-click the jar file
//==============================================================================
// Date        Author        Version
//      Changes
//==============================================================================
// 2003-01-18  coder_1024    v1.0
//
//      - initial creation
//------------------------------------------------------------------------------
// 2003-02-10  coder_1024    v1.1
//
//      - took measures to ensure the code which updates the server status
//        panes is thread safe
//      - added support for playing an up and down sound when the state of the
//        game server changes.  note that the build instructions have changed
//        slightly.  the .wav files are included in the runnable jar file.
//------------------------------------------------------------------------------
// 2003-03-08  coder_1024    v1.2
//
//      - updated to use Sancus' server
//******************************************************************************
