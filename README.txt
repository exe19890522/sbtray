//******************************************************************************
// sbtray 
// written by coder1024@gmail.com
//
// - provides periodic status of the Shadowbane servers, graphically, via a
//   Windows tray icon
// - red = down, green = up, grey = unknown
// - unknown could mean that the page could not be downloaded or that it was
//   not in the expected form.
// - server status information is obtained from the Ubisoft Chronicle of Strife
//   web page
//                 http://chronicle.ubi.com
// - visit the sbtray home page at
//                 http://sbtray.sourceforge.net
// - the tray icon shows the state of the currently selected game world, along
//   with the state of the patch and login servers, in that order (i.e. G|P|L).
// - right-click on the tray icon to access the sbtray menu
// - use the worlds menu to see the state of all of the game worlds and to 
//   select a different world.
// - the sbtray menu also shows how long its been since the last check
//   and how long the currently selected game world, along with the patch and
//   login servers have been in the state they're currently in.  in other words,
//   if the patch server is down, you can use this to see how long its been
//   down for, or if its up, you can see whether its just come up or been up
//   for awhile.
// - the links menu allows you to quickly jump to various SB web sites,
// - you can customize the links menu by choosing the "customize this menu"
//   item on the links menu
// - you can customize the list of worlds by choosing the "Edit World List"
//   item in the worlds menu
// - you can use the Options menu item to configure sbtray
// - if you've enabled sounds, then the upsound.wav and downsound.wav will be
//   played as appropriate when the currently selected world goes up and down.
//   you can replace these files with your own sounds if you'd like.
// - see history.txt for detailed revision history
//******************************************************************************
