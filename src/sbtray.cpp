//******************************************************************************
// main source file
//******************************************************************************

#pragma warning(disable:4786)
#define _WIN32_WINNT 0x0500

#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100

#define SD_BOTH          0x02

//******************************************************************************
// system headers
//******************************************************************************
#include <windows.h>
#include <wininet.h>
#include <mmsystem.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <ctype.h>
#include <sstream>
using namespace std;

//******************************************************************************
// application headers
//******************************************************************************
#include "resource.h"
#include "sbtray_config.h"
#include "sbtray_options_dlg.h"
#include "sbtray_about_dlg.h"
#include "sbtray_email.h"

//******************************************************************************
// constants
//******************************************************************************
// URL to check
#define URL_SBSERVERSTAT  "http://chronicle.ubi.com/"
// minimum status update period (secs).  if something less than this is
// specified in the options dialog, it'll just use this.
#define MIN_UPDATEPERIOD  5
// # bytes to read at a time from the page
#define READ_SIZE         1000
// file used to build the Links menu item
#define LINKS_FILE        "sbtray_links.txt"
// file which contains the list of worlds
#define WORLDLIST_FILE    "sbtray_worldlist.txt"
// separator keyword used in links file
#define KW_SEP            "<SEP>"
// base ID used for Links menu items
#define LINKS_BASEID      50000
// base ID used for Worlds menu items
#define WORLDS_BASEID     55000
// file to log to
#define LOGFILE           "sbtray_log.txt"
// sound files played when the game server goes up and down
#define SOUND_GAME_UP     "upsound.wav"
#define SOUND_GAME_DOWN   "downsound.wav"
// string used for the menu item which indicates how old the current
// server state information is.  %d is replaced with # seconds
#define STATEAGE_STR      "Downloaded %s ago"
// string used for the menu item which indicates how long the
// server states have been what they currently are
#define AGEBYSERVER_STR   "G:%s P:%s L:%s"
// string used for menu item which indicates which
// world is currently selected
#define SELECTEDWORLD_STR "Selected World: %s"
//
// icon ID is determined using a bitmask.  two bits
// are used for each item to provide a tri-state indicator.
// the possible states are:
//    UNK on, MASK off
//    UNK off, MASK on or off
//
#define LOGIN_MASK        1
#define PATCH_MASK        2
#define GAME_MASK         4
#define LOGIN_UNK_MASK    8
#define PATCH_UNK_MASK    16
#define GAME_UNK_MASK     32
// how often the user interface updates itself (ms)
#define WND_TIMER_LEN     1000
// information for the main window.  this is an invisible window used
// to handle messages from the tray icon
#define WND_MENU_NAME     "MainMenu"
#define WND_CLASS_NAME    "MainWndClass"
#define WND_TITLE         "sbtray"
// strings used for displaying error message boxes
#define ERR_TITLE         "sbtray: error"
#define ERR_INUSE         "Already running"
#define ERR_INIT          "Unable to initialize"
// parameters used for the tray icon
#define ICON_UID          101
#define ICON_MSG          WM_APP + ICON_UID
#define ICON_TIP          "sbtray v1.17 (G | P | L)"
// numeric value of the first icon resource used.  the individual
// icon values are offsets from this, and their offset is the
// value obtained by forming a bit pattern of the server states
#define ICON_BASEID       IDI_ICON_BASE
// similar for the bitmap versions
#define BITMAP_BASEID     IDB_BITMAP_BASE
// other misc constants
#define WND_TIMER_ID      101
#define INET_AGENT_NAME   "sbtray"
#define MUTEX_NAME        "sbtray:mutex"
#define STR_BUFFER_SIZE   1024
// initial value for world name
#define INIT_WORLD_NAME   "<unknown>"
// popup text for edit worlds comand
#define EDITWORLDSTXT     "Edit World List"
// patch/login servers
#define PATCH_SERVER       "sb-ps.ubi.com"
#define PATCH_SERVER_PORT  3360
#define LOGIN_SERVER       "sb-ls.ubi.com"
#define LOGIN_SERVER_PORT  6000

//******************************************************************************
// world/server state
//******************************************************************************
typedef enum
{
	SBSTATE_DOWN = 0,
	SBSTATE_UP   = 1,
	SBSTATE_UNK  = 2

} sbstate;

//******************************************************************************
// a world
//******************************************************************************
class world
{
public:
	// constructor
	world()
	{
		name = INIT_WORLD_NAME;
		state = SBSTATE_UNK;
		prev_state = SBSTATE_UNK;
		DWORD cur_time = time(0);
		last_change = cur_time;
	}

	// name of the world
	string name;
	// current state of the world's servers
	// (uses masks defined above)
	sbstate state;
	// prev state
	sbstate prev_state;
	// how long since last state change
	DWORD last_change;
};

//******************************************************************************
// global data used throughout
//******************************************************************************
NOTIFYICONDATA	 nid;
CRITICAL_SECTION critSect;
char             readBuf[READ_SIZE+1];
char             sndBuf[STR_BUFFER_SIZE];
sbtray_parms     parms;

HINSTANCE		 hInst		       = 0;
HWND			 hWndMain	       = 0;
BOOL             bIconAdded        = FALSE;
HANDLE           hWorkerThread     = 0;
DWORD            workerThreadID    = 0;
HANDLE           hSoundThread      = 0;
DWORD            soundThreadID     = 0;
UINT             timerID           = 0;
HMENU            hMenu             = 0;
HMENU            hPopup            = 0;
HMENU            hLinks            = 0;
HMENU            hWorlds           = 0;
HANDLE           hMutex            = 0;
bool             firstUpdate       = true;

// world collection
vector<world>    worlds;

// time of last state update
DWORD            last_update;

// patch/login servers
world            patch_server;
world            login_server;

// resource maps
map<DWORD,HBITMAP> bitmaps;
map<DWORD,HICON> icons;

//******************************************************************************
// checks for the ability to establish a TCP connection with the specified
// host on the specified port
//******************************************************************************
bool CheckTCPConnectivity(const char * host, int port)
{
	bool result = false;

	hostent * he = gethostbyname(host);
	if (he != 0)
	{
		SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
		if (sock != INVALID_SOCKET)
		{
			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons((u_short)port);
			memcpy(&addr.sin_addr,he->h_addr_list[0],sizeof(in_addr));

			if (connect(sock,(struct sockaddr *)&addr,sizeof(sockaddr)) != SOCKET_ERROR)
			{
                shutdown(sock,SD_BOTH);

				result = true;
			}

			closesocket(sock);
		}
	}

	return result;
}

//******************************************************************************
// downloads the state of the worlds and the patch/login servers
//******************************************************************************
void GetState(vector<world> & _worlds,
			  world & _patch_server,
			  world & _login_server)
{
	unsigned int x;

	// initialize all states to unknown initially
	for (x = 0; x < _worlds.size(); x++)
	{
		_worlds[x].state = SBSTATE_UNK;
	}	

	// open internet handle
	HINTERNET hInet;
	hInet = InternetOpen(INET_AGENT_NAME,INTERNET_OPEN_TYPE_PRECONFIG,0,0,0);
	if (hInet)
	{
		// open status page URL
		HINTERNET hURL;
		hURL = InternetOpenUrl(hInet,URL_SBSERVERSTAT,0,0,INTERNET_FLAG_RELOAD,0);
		if (hURL)
		{
			// read the page
			DWORD bytesRead;			
			bool keepReading = true;
			string pagestr;
			while (keepReading)
			{
				bytesRead = 0;
				ZeroMemory(readBuf,READ_SIZE+1);
				InternetReadFile(hURL,readBuf,READ_SIZE,&bytesRead);

				if (bytesRead > 0)
				{
					pagestr += readBuf;
				}
				else
				{
					keepReading = false;
				}
			}

			// did we get anything?
			if (pagestr.length() > 0)
			{
				char * linebuf = new char[pagestr.length()+1];
				if (linebuf)
				{
					istringstream ps(pagestr);

					while (!ps.eof())
					{
						linebuf[0] = '\0';
						ps.getline(linebuf,pagestr.length()+1);
						if (strlen(linebuf) > 0)
						{
							bool bFoundAllServers = true;
							for (x = 0; (x < _worlds.size()) && bFoundAllServers; x++)
							{
								if (strstr(linebuf,_worlds[x].name.c_str()) == NULL)
								{
									bFoundAllServers = false;
								}
							}

							if (bFoundAllServers)
							{
								char * serverBuf = new char[strlen(linebuf)+1];
								if (serverBuf)
								{
									for (x = 0; x < _worlds.size(); x++)
									{
										char * sstart = strstr(linebuf,_worlds[x].name.c_str());
										char * sstop = strstr(sstart,"</font>");
										memset(serverBuf,0,strlen(linebuf)+1);
										strncpy(serverBuf,sstart,(sstop-sstart));

										if (strstr(serverBuf,"Up"))
										{
											_worlds[x].state = SBSTATE_UP;
										}
										else if (strstr(serverBuf,"Down"))
										{
											_worlds[x].state = SBSTATE_DOWN;
										}
									}

									delete [] serverBuf;
								}
							}
						}
					}

					delete [] linebuf;
				}
			}

			InternetCloseHandle(hURL);
		}

		InternetCloseHandle(hInet);
	}

	_patch_server.state = SBSTATE_DOWN;
	_login_server.state = SBSTATE_DOWN;

	if (CheckTCPConnectivity(PATCH_SERVER,PATCH_SERVER_PORT))
	{
		_patch_server.state = SBSTATE_UP;
	}

	if (CheckTCPConnectivity(LOGIN_SERVER,LOGIN_SERVER_PORT))
	{
		_login_server.state = SBSTATE_UP;
	}
}

//******************************************************************************
// worker thread which periodically checks the server status
//******************************************************************************
DWORD WINAPI WorkerThreadProc(LPVOID lpParm)
{
	DWORD dwUpdatePeriod;
	BOOL log;
	unsigned int x;
	vector<world> _worlds;
	world _patch_server;
	world _login_server;

	// begin exchange of shared data
	EnterCriticalSection(&critSect);

		// grab a local snapshot of the worlds and server states
		_worlds = worlds;
		_patch_server = patch_server;
		_login_server = login_server;

	// end exchange of shared data
	LeaveCriticalSection(&critSect);

	while (TRUE)
	{
		// get the new server state
		GetState(_worlds,_patch_server,_login_server);

		// timestamp
		DWORD cur_time = time(0);
		
		// begin exchange of shared data
		EnterCriticalSection(&critSect);

			// last update time stamp
			last_update = cur_time;

			// store state and update timestamp back to
			// the shared worlds collection
			for (x = 0; x < _worlds.size(); x++)
			{
				worlds[x].state = _worlds[x].state;
			}	

			// ... and patch server
			patch_server.state = _patch_server.state;

			// ... and login server
			login_server.state = _login_server.state;

			// get the latest options settings for how often
			// to update and whether or not to log to a file
			dwUpdatePeriod = parms.dwUpdatePeriod;
			log = parms.bEnableLogging;

		// end exchange of shared data
		LeaveCriticalSection(&critSect);

		// log
		if (log)
		{
			FILE * f = 0;
			bool writeHeader = false;

			// create new logfile if it doesn't already exist
			if (!(f = fopen(LOGFILE,"r")))
			{
				// creating a new file; need to write the header
				f = fopen(LOGFILE,"w");
				if (f)
				{
					writeHeader = true;
				}
			}
			else
			{
				// already exists.  open for append
				fclose(f);
				f = fopen(LOGFILE,"a");
			}

			// was file opened?
			if (f)
			{
				time_t t = cur_time;
				struct tm * now = localtime(&t);

				if (writeHeader)
				{
					fprintf(f,"sbtray logfile (0 = down, 1 = up, 2 = unknown)\n\nDate/Time");

					for (x = 0; x < _worlds.size(); x++)
					{
						fprintf(f,",%s",_worlds[x].name.c_str());
					}

					fprintf(f,"\n");
				}

				fprintf(f,"%d/%d/%d %d:%d",
					now->tm_mon + 1,
					now->tm_mday,
					now->tm_year + 1900,
					now->tm_hour,
					now->tm_min);

				for (x = 0; x < _worlds.size(); x++)
				{
					fprintf(f,",%d",
						_worlds[x].state);
				}

				fprintf(f,"\n");

				// close the file
				fflush(f);
				fclose(f);
			}
		}

		// make sure we sleep AT LEAST as long as the minimum
		// update period
		if (dwUpdatePeriod < MIN_UPDATEPERIOD)
		{
			dwUpdatePeriod = MIN_UPDATEPERIOD;
		}

		// sleep until its time to check again
		Sleep(dwUpdatePeriod * 1000);
	}

	return 0;
}

//******************************************************************************
// kills the worker thread (if its running) and then starts it again
//******************************************************************************
BOOL BounceWorkerThread()
{
	BOOL result = FALSE;

	// kill it if its running
	if (hWorkerThread)
	{
		EnterCriticalSection(&critSect);
			TerminateThread(hWorkerThread,0);
			CloseHandle(hWorkerThread);
			hWorkerThread = 0;
		LeaveCriticalSection(&critSect);
	}

	// create the worker thread
	hWorkerThread = CreateThread(0,0,WorkerThreadProc,0,
							CREATE_SUSPENDED,&workerThreadID);
	if (hWorkerThread)
	{
		// start the worker thread running
		if (ResumeThread(hWorkerThread) != -1)
		{
			result = TRUE;
		}
	}

	return result;
}

//******************************************************************************
// worker thread which plays a specified sound
//******************************************************************************
DWORD WINAPI SoundThreadProc(LPVOID lpParm)
{
	// cancel any currently playing sound (just in case)
	sndPlaySound(0,0);

	// play the sound
	sndPlaySound(sndBuf,SND_ASYNC | SND_NODEFAULT);

	return 0;
}

//******************************************************************************
// plays a sound in a seperate thread
//******************************************************************************
BOOL BounceSoundThread(LPCTSTR soundFile)
{
	BOOL result = FALSE;

	// kill sound thread if running
	if (hSoundThread)
	{
		TerminateThread(hSoundThread,0);
		CloseHandle(hSoundThread);
		hSoundThread = 0;
	}

	// store sound to play in buffer
	strcpy(sndBuf,soundFile);

	// start sound thread
	hSoundThread = CreateThread(0,0,SoundThreadProc,0,
								CREATE_SUSPENDED,&soundThreadID);
	if (hSoundThread)
	{
		// start the sound thread running
		if (ResumeThread(hSoundThread) != -1)
		{
			result = TRUE;
		}
	}

	return result;
}

//******************************************************************************
// puts a time delta (secs) into a readable format
//******************************************************************************
void FormatTimeDelta(DWORD delta, char * buf)
{
	char tmp[STR_BUFFER_SIZE];
	buf[0] = 0;
	BOOL showSecs = TRUE;

	// weeks
	if (delta > (60*60*24*7))
	{
		wsprintf(tmp,"%dw",(DWORD)((double)delta/(double)(60*60*24*7)));
		strcat(buf,tmp);
		delta = delta % (60*60*24*7);
		showSecs = FALSE;
	}

	// days
	if (delta > (60*60*24))
	{
		wsprintf(tmp,"%dd",(DWORD)((double)delta/(double)(60*60*24)));
		strcat(buf,tmp);
		delta = delta % (60*60*24);
		showSecs = FALSE;
	}

	// hours
	if (delta > (60*60))
	{
		wsprintf(tmp,"%dh",(DWORD)((double)delta/(double)(60*60)));
		strcat(buf,tmp);
		delta = delta % (60*60);
		showSecs = FALSE;
	}

	// minutes
	if (delta > 60)
	{
		wsprintf(tmp,"%dm",(DWORD)((double)delta/(double)(60)));
		strcat(buf,tmp);
		delta = delta % 60;
		showSecs = FALSE;
	}

	// seconds
	if (showSecs)
	{
		wsprintf(tmp,"%ds",delta);
		strcat(buf,tmp);
	}

	return;
}

//******************************************************************************
// updates the user interface with the latest server state as reported from
// the worker thread.
//******************************************************************************
void UpdateUI()
{	
	static WORD prevIconID = 0;
	static bool firsttime = true;
	static char prev_m1_text[STR_BUFFER_SIZE];
	static char prev_m2_text[STR_BUFFER_SIZE];

	WORD iconID, bmpID;
	DWORD curTime, deltaTime;
	vector<world> _worlds;
	world _patch_server;
	world _login_server;
	DWORD _last_update;
	DWORD x;

	// begin exchange of shared data
	EnterCriticalSection(&critSect);

		// get local snapshot of the worlds and servers
		_worlds = worlds;
		_patch_server = patch_server;
		_login_server = login_server;
		_last_update = last_update;

	// end exchange of shared data
	LeaveCriticalSection(&critSect);

	if (firsttime)
	{
		prev_m1_text[0] = 0;
		prev_m2_text[0] = 0;
	}

	// get current time
	curTime = time(0);

	iconID = 0;

	// (icon) patch server state
	if (_patch_server.state == SBSTATE_UNK)
	{
		iconID |= PATCH_UNK_MASK;
	}
	else
	{
		if (_patch_server.state == SBSTATE_UP)
		{
			iconID |= PATCH_MASK;
		}
	}

	// patch server state change
	if (_patch_server.state != _patch_server.prev_state)
	{
		_patch_server.last_change = curTime;
		_patch_server.prev_state = _patch_server.state;
	}

	// (icon) login server state
	if (_login_server.state == SBSTATE_UNK)
	{
		iconID |= LOGIN_UNK_MASK;
	}
	else
	{
		if (_login_server.state == SBSTATE_UP)
		{
			iconID |= LOGIN_MASK;
		}
	}

	// login server state change
	if (_login_server.state != _login_server.prev_state)
	{
		_login_server.last_change = curTime;
		_login_server.prev_state = _login_server.state;
	}

	// (icon) active world state
	x = parms.dwSelectedWorld;
	if (_worlds[x].state == SBSTATE_UNK)
	{
		iconID |= GAME_UNK_MASK;
	}
	else
	{
		if (_worlds[x].state == SBSTATE_UP)
		{
			iconID |= GAME_MASK;
		}
	}

	iconID += ICON_BASEID;

	// update icon if necessary
	if (iconID != prevIconID)
	{
		prevIconID = iconID;

		ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
		nid.cbSize				= sizeof(NOTIFYICONDATA);
		nid.hWnd				= hWndMain;
		nid.uID					= ICON_UID;
		nid.hIcon				= icons[iconID];
		nid.uFlags				= NIF_ICON;
		Shell_NotifyIcon(NIM_MODIFY,&nid);
	}
		
	// update world bitmaps in the worlds menu and last update & change info
	for (x = 0; x < _worlds.size(); x++)
	{
		// world state change or first time
		if ((_worlds[x].state != _worlds[x].prev_state) ||
			firsttime)
		{
			// world state change
			if (_worlds[x].state != _worlds[x].prev_state)
			{
				_worlds[x].last_change = curTime;

				// are sounds enabled?
				if ((parms.bEnableSounds) &&
					(x == parms.dwSelectedWorld))
				{
					// play the appropriate sound
					if (_worlds[x].state == SBSTATE_UP)
					{
						BounceSoundThread(SOUND_GAME_UP);
					}
					else
					{
						BounceSoundThread(SOUND_GAME_DOWN);
					}
				}

				// send email?
				if ((parms.bEnableEmail) &&
					(x == parms.dwSelectedWorld))
				{
					char msg[STR_BUFFER_SIZE];

					wsprintf(msg,"sbtray, %s, G:%d P:%d L:%d",
							_worlds[x].name.c_str(),
							(_worlds[x].state & GAME_MASK  ? 1 : 0),
							(_worlds[x].state  & PATCH_MASK ? 1 : 0),
							(_worlds[x].state  & LOGIN_MASK ? 1 : 0));

					SendEmailAsynch(parms.szSMTPServer,parms.szMailFrom,parms.szMailTo,
						msg,msg);
				}
			}

			// update menu bitmap
			bmpID = BITMAP_BASEID + _worlds[x].state;
			MENUITEMINFO mii;
			ZeroMemory(&mii,sizeof(MENUITEMINFO));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_BITMAP;
			mii.hbmpItem = bitmaps[bmpID];
			SetMenuItemInfo(hWorlds,(x * 2),TRUE,&mii);

			_worlds[x].prev_state = _worlds[x].state;
		}
	}

	char dtBuf[STR_BUFFER_SIZE];
	MENUITEMINFO mii;

	// compute age of current state
	deltaTime = curTime - _last_update;
	char dtfBuf[STR_BUFFER_SIZE];
	FormatTimeDelta(deltaTime,dtfBuf);
	wsprintf(dtBuf,STATEAGE_STR,dtfBuf);
	
	// update menu item to indicate how old the current state is
	if (strcmp(dtBuf,prev_m1_text))
	{
		ZeroMemory(&mii,sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = dtBuf;
		SetMenuItemInfo(hPopup,ID_POPUP_SERVERSTATEAGE,FALSE,&mii);

		strcpy(prev_m1_text,dtBuf);
	}

	// update menu item to indicate age of individual server states
	char gameDelta[STR_BUFFER_SIZE],
		 patchDelta[STR_BUFFER_SIZE],
		 loginDelta[STR_BUFFER_SIZE];
	FormatTimeDelta(curTime - _worlds[parms.dwSelectedWorld].last_change,gameDelta);
	FormatTimeDelta(curTime - _patch_server.last_change,patchDelta);
	FormatTimeDelta(curTime - _login_server.last_change,loginDelta);

	wsprintf(dtBuf,AGEBYSERVER_STR,
		gameDelta,
		patchDelta,
		loginDelta);

	if (strcmp(dtBuf,prev_m2_text))
	{
		ZeroMemory(&mii,sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = dtBuf;
		SetMenuItemInfo(hPopup,ID_POPUP_AGEBYSERVER,FALSE,&mii);

		strcpy(prev_m2_text,dtBuf);
	}

	// begin exchange of shared data
	EnterCriticalSection(&critSect);

		// store updates back to the shared worlds collection
		for (x = 0; x < _worlds.size(); x++)
		{
			worlds[x].prev_state = _worlds[x].prev_state;
			worlds[x].last_change = _worlds[x].last_change;
		}

		patch_server.prev_state = _patch_server.prev_state;
		patch_server.last_change = _patch_server.last_change;

		login_server.prev_state = _login_server.prev_state;
		login_server.last_change = _login_server.last_change;


	// end exchange of shared data
	LeaveCriticalSection(&critSect);

	firsttime = false;
	return;
}

//******************************************************************************
// visits the specified link
//******************************************************************************
void VisitLink(WORD link)
{
	// get the item to open
	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_DATA;
	GetMenuItemInfo(hLinks,link,TRUE,&mii);

	// open the item
	ShellExecute(hWndMain,"open",(LPCTSTR)mii.dwItemData,0,0,SW_SHOW);

	return;
}

//******************************************************************************
// message handler for the main window
//******************************************************************************
LRESULT CALLBACK MainWndProc(HWND hWnd,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam)
{
	LRESULT result = 0;

	// user interface update timer;  call update function
	if ((uMsg == WM_TIMER) &&
		(wParam == WND_TIMER_ID))
	{
		UpdateUI();
	}

	// message from the tray icon...
	else if (uMsg == ICON_MSG)
	{
		// user right-click.  handle context menu
		if ((lParam == WM_RBUTTONDOWN) ||
			(lParam == WM_CONTEXTMENU))
		{
			POINT p;
			GetCursorPos(&p);

			SetForegroundWindow(hWndMain);
			BOOL mres = TrackPopupMenu(hPopup,TPM_LEFTALIGN | TPM_TOPALIGN,
				p.x,p.y,0,hWndMain,0);
		}
	}

	// command message received
	else if (uMsg == WM_COMMAND)
	{
		WORD cmd = LOWORD(wParam);

		// user selected Close from the popup menu.  post the quit message.
		if (LOWORD(wParam) == ID_POPUP_CLOSE)
		{
			PostQuitMessage(0);
		}

		// options
		else if (LOWORD(wParam) == ID_POPUP_OPTIONS)
		{
			// disable options menu item
			EnableMenuItem(hPopup,ID_POPUP_OPTIONS,MF_GRAYED);
			// begin shared data exchange
			EnterCriticalSection(&critSect);
				// get local snapshot of parms
				sbtray_parms tmpParms = parms;
			// end shared data exchange
			LeaveCriticalSection(&critSect);
			// present options dialog to user
			DoOptionsDlg(hInst,hWndMain,&tmpParms);
			// begin shared data exchange
			EnterCriticalSection(&critSect);
				// store parms back to shared copy
				parms = tmpParms;
			// end shared data exchange
			LeaveCriticalSection(&critSect);
			// save configuration parameters
			SaveConfig(&parms);
			// re-enable the options menu item
			EnableMenuItem(hPopup,ID_POPUP_OPTIONS,MF_ENABLED);
			// bounce worker thread in case the user changed
			// options which would affect it
			BounceWorkerThread();
		}

		// about
		else if (LOWORD(wParam) == ID_POPUP_ABOUT)
		{
			// disable About menu item
			EnableMenuItem(hPopup,ID_POPUP_ABOUT,MF_GRAYED);
			// do about
			DoAboutDlg(hInst,hWndMain);
			// re-enable the about menu item
			EnableMenuItem(hPopup,ID_POPUP_ABOUT,MF_ENABLED);
		}

		// update now
		else if (LOWORD(wParam) == ID_POPUP_UPDATENOW)
		{
			BounceWorkerThread();
		}

		// Links
		else if ((LOWORD(wParam) >= LINKS_BASEID) &&
				 (LOWORD(wParam) < WORLDS_BASEID))
		{
			VisitLink(LOWORD(wParam) - LINKS_BASEID);
		}

		// Worlds
		else if (LOWORD(wParam) >= WORLDS_BASEID)
		{
			// un check currently selected world
			CheckMenuItem(hWorlds,
						  (parms.dwSelectedWorld * 2),
						  (MF_BYPOSITION | MF_UNCHECKED));

			// get new selection
			MENUITEMINFO mii;
			ZeroMemory(&mii,sizeof(MENUITEMINFO));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_DATA;
			GetMenuItemInfo(hWorlds,(LOWORD(wParam) - WORLDS_BASEID),TRUE,&mii);

			// store new selection
			parms.dwSelectedWorld = mii.dwItemData;
			SaveConfig(&parms);

			// check new selection
			CheckMenuItem(hWorlds,
						  (parms.dwSelectedWorld * 2),
						  (MF_BYPOSITION | MF_CHECKED));

			// set selected world menu item text
			char selWorldBuf[STR_BUFFER_SIZE];
			sprintf(selWorldBuf,SELECTEDWORLD_STR,
				worlds[parms.dwSelectedWorld].name.c_str());

			ZeroMemory(&mii,sizeof(MENUITEMINFO));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = selWorldBuf;
			SetMenuItemInfo(hPopup,ID_POPUP_SELECTEDWORLD,FALSE,&mii);

			// update UI
			UpdateUI();
		}

		// edit world list
		else if (LOWORD(wParam) == ID_POPUP_EDITWORLDLIST)
		{
			// open world list file
			ShellExecute(hWndMain,"open",WORLDLIST_FILE,0,0,SW_SHOW);
		}
	}
	else
	{
		// pass the message off to the default message handler
		result = DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	return result;
}

//******************************************************************************
// trims trailing \n if present
//******************************************************************************
void TrimStr(char * str)
{
	size_t l = strlen(str);
	if (str[l-1] == '\n')
	{
		str[l-1] = '\0';
	}

	return;
}

//******************************************************************************
// populates the Links menu with content from the links file
//******************************************************************************
void LoadLinks()
{
	MENUITEMINFO mii;
	UINT mip = 0;

	// get the Links menu
	hLinks = GetSubMenu(hPopup,2);
	if (hLinks)
	{
		// remove the place-holder item (needed or else the Links gets created
		// as an item as opposed to a sub-menu)
		RemoveMenu(hLinks,0,MF_BYPOSITION);

		// open the Links file
		FILE * f = fopen(LINKS_FILE,"r");
		if (f)
		{
			char titleBuf[STR_BUFFER_SIZE],
				 urlBuf[STR_BUFFER_SIZE];

			// process entire file
			while (!feof(f))
			{
				// read the title line
				if (fgets(titleBuf,STR_BUFFER_SIZE-1,f))
				{
					// trim it
					TrimStr(titleBuf);

					// ignore comment lines
					if (titleBuf[0] != '#')
					{
						// check for SEP keyword
						if (strcmp(titleBuf,KW_SEP) == 0)
						{
							ZeroMemory(&mii,sizeof(MENUITEMINFO));
							mii.cbSize = sizeof(MENUITEMINFO);
							mii.fMask = MIIM_FTYPE;
							mii.fType = MFT_SEPARATOR;

							InsertMenuItem(hLinks,mip++,TRUE,&mii);
						}
					
						// read the URL line
						else if (fgets(urlBuf,STR_BUFFER_SIZE-1,f))
						{
							// trim it
							TrimStr(urlBuf);

							// ignore comment lines
							if (urlBuf[0] != '#')
							{
								ZeroMemory(&mii,sizeof(MENUITEMINFO));
								mii.cbSize = sizeof(MENUITEMINFO);
								mii.fMask = MIIM_STRING | MIIM_ID | MIIM_DATA;
								mii.dwTypeData = titleBuf;
								mii.wID = LINKS_BASEID + mip;
								mii.dwItemData = (DWORD)strdup(urlBuf);

								InsertMenuItem(hLinks,mip++,TRUE,&mii);
							}
						}
					}
				}
			}

			// close the file
			fclose(f);
		}
	}

	return;
}

//******************************************************************************
// loads the list of worlds
//******************************************************************************
void LoadWorldList()
{
	MENUITEMINFO mii;
	UINT mip = 0;
	UINT worldNum = 0;

	hWorlds = GetSubMenu(hPopup,5);
	if (hWorlds)
	{
		// remove the place-holder item (needed or else the Links gets created
		// as an item as opposed to a sub-menu)
		RemoveMenu(hWorlds,0,MF_BYPOSITION);
	}

	// open the Links file
	FILE * f = fopen(WORLDLIST_FILE,"r");
	if (f)
	{
		char nameBuf[STR_BUFFER_SIZE];

		// process entire file
		while (!feof(f))
		{
			// read line
			if (fgets(nameBuf,STR_BUFFER_SIZE-1,f))
			{
				// trim it
				TrimStr(nameBuf);

				// ignore comment lines
				if ((strlen(nameBuf) > 0) &&
					(nameBuf[0] != '#'))
				{
					world w;
					w.name = nameBuf;
					worlds.push_back(w);

					char nameDispBuf[STR_BUFFER_SIZE];
					sprintf(nameDispBuf,"  %s",nameBuf);

					if (worldNum > 0)
					{
						// separator
						ZeroMemory(&mii,sizeof(MENUITEMINFO));
						mii.cbSize = sizeof(MENUITEMINFO);
						mii.fMask = MIIM_FTYPE;
						mii.fType = MFT_SEPARATOR;
						InsertMenuItem(hWorlds,mip,TRUE,&mii);
						mip++;
					}

					// add menu item
					ZeroMemory(&mii,sizeof(MENUITEMINFO));
					mii.cbSize = sizeof(MENUITEMINFO);
					mii.fMask = MIIM_STRING | MIIM_ID | MIIM_DATA;
					mii.dwTypeData = nameDispBuf;
					mii.wID = WORLDS_BASEID + mip;
					mii.dwItemData = (DWORD)worldNum;
					InsertMenuItem(hWorlds,mip,TRUE,&mii);

					// set check
					if (worldNum == parms.dwSelectedWorld)
					{
						CheckMenuItem(hWorlds,mip,MF_CHECKED | MF_BYPOSITION);
					}

					mip++;

					worldNum++;
				}
			}
		}

		// close the file
		fclose(f);
	}

	// separator
	ZeroMemory(&mii,sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hWorlds,mip,TRUE,&mii);
	mip++;

	// edit world list item
	ZeroMemory(&mii,sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.dwTypeData = EDITWORLDSTXT;
	mii.wID = ID_POPUP_EDITWORLDLIST;
	InsertMenuItem(hWorlds,mip,TRUE,&mii);
	mip++;

	return;
}

//******************************************************************************
// loads resources into the resource maps to be used throughout the lifetime
// of the application
//******************************************************************************
void LoadResources()
{
	// bitmaps
	DWORD bmpid;
	for (bmpid = IDB_BITMAP_BASE;
	     bmpid < IDB_BITMAP_LASTID;
		 bmpid++)
	{
		bitmaps[bmpid] = LoadBitmap(hInst,MAKEINTRESOURCE(bmpid));
	}

	// icons
	DWORD iconid;
	for (iconid = IDI_ICON_BASE;
		 iconid < IDI_ICON_LASTID;
		 iconid++)
	{
		icons[iconid] = LoadIcon(hInst,MAKEINTRESOURCE(iconid));
	}

	return;
}

//******************************************************************************
// initializes everything we need to run
//******************************************************************************
BOOL Init()
{
	BOOL result = FALSE;
	MENUITEMINFO mii;
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2,2),&wsaData);

	// make sure sbtray is not already running
	hMutex = CreateMutex(0,FALSE,MUTEX_NAME);
	if (hMutex &&
		(GetLastError() == ERROR_ALREADY_EXISTS))
	{
		MessageBox(0,ERR_INUSE,ERR_TITLE,MB_OK);
	}
	else
	{
		// load configuration parameters
		LoadConfig(&parms);
		// save configuration parameters
		SaveConfig(&parms);

		// create a window class
		WNDCLASS wc;
		ZeroMemory(&wc,sizeof(WNDCLASS));
		wc.lpfnWndProc   = MainWndProc;
		wc.hInstance     = hInst;
		wc.hIcon         = LoadIcon(0,IDI_APPLICATION);
		wc.hCursor       = LoadCursor(0,IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName  = WND_MENU_NAME;
		wc.lpszClassName = WND_CLASS_NAME;
		if (RegisterClass(&wc))
		{
			// create the main window at 0,0 with a size of 0,0.  this window
			// is never seen by the user, however, its used to handle messages
			// from the tray icon
			hWndMain = CreateWindow(WND_CLASS_NAME,WND_TITLE,
				0,0,0,0,0,0,0,hInst,0);
			if (hWndMain)
			{
				// load the menu resource (allows us to display the right-click
				// popup menu)
				hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_SBTRAY_MENU));
				if (hMenu)
				{
					// load the popup menu
					hPopup = GetSubMenu(hMenu,0);
					if (hPopup)
					{
						// load the links menu item
						LoadLinks();

						// load world list
						LoadWorldList();

						// make sure selected world is valid
						if (parms.dwSelectedWorld >= worlds.size())
						{
							parms.dwSelectedWorld = 0;
							SaveConfig(&parms);
						}

						// load resources
						LoadResources();

						// set selected world menu item text
						char selWorldBuf[STR_BUFFER_SIZE];
						sprintf(selWorldBuf,SELECTEDWORLD_STR,
							worlds[parms.dwSelectedWorld].name.c_str());

						ZeroMemory(&mii,sizeof(MENUITEMINFO));
						mii.cbSize = sizeof(MENUITEMINFO);
						mii.fMask = MIIM_STRING;
						mii.dwTypeData = selWorldBuf;
						SetMenuItemInfo(hPopup,ID_POPUP_SELECTEDWORLD,FALSE,&mii);

						// add the tray icon
						ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
						nid.cbSize				= sizeof(NOTIFYICONDATA);
						nid.hWnd				= hWndMain;
						nid.uID					= ICON_UID;
						nid.uCallbackMessage	= ICON_MSG;
						nid.hIcon				= LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON_LOADING));
						nid.uFlags				= NIF_ICON | NIF_MESSAGE | NIF_TIP;

						strcpy(nid.szTip,ICON_TIP);

						if (Shell_NotifyIcon(NIM_ADD,&nid))
						{
							bIconAdded = TRUE;

							// initialize critical section (used for synchronization)
							InitializeCriticalSection(&critSect);

							// initialize sound library.  this can take a little bit
							// which is why we use the loading icon above.  once its done
							// future sound requests off in the sound thread are serviced
							// very quickly.
							sndPlaySound(0,0);

							// do an initial update.  this will make sure the unknown
							// state is displayed until the initial state download
							// has completed
							UpdateUI();

							// start the user interface update timer
							timerID = SetTimer(hWndMain,WND_TIMER_ID,WND_TIMER_LEN,0);
							if (timerID)
							{
								// kick off the worker thread
								if (BounceWorkerThread())
								{
									result = TRUE;
								}
							}
						}
					}
				}
			}
		}

		// if we failed to successfully initialize, display an error
		// to the user
		if (!result)
		{
			MessageBox(0,ERR_INIT,ERR_TITLE,MB_OK);
		}
	}

	return result;
}

//******************************************************************************
// cleans everything up before we exit
//******************************************************************************
void Shutdown()
{
	// close the mutex handle
	if (hMutex)
	{
		CloseHandle(hMutex);
		hMutex = 0;
	}

	// terminate sound thread
	if (hSoundThread)
	{
		TerminateThread(hSoundThread,0);
		CloseHandle(hSoundThread);
		hSoundThread = 0;
	}

	// terminate the worker thread
	if (hWorkerThread)
	{
		EnterCriticalSection(&critSect);
			TerminateThread(hWorkerThread,0);
			CloseHandle(hWorkerThread);
			hWorkerThread = 0;
		LeaveCriticalSection(&critSect);
	}

	// kill the window user interface update timer
	if (timerID)
	{
		KillTimer(hWndMain,WND_TIMER_ID);
		timerID = 0;
	}

	// remove the tray icon
	if (bIconAdded)
	{
		ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
		nid.cbSize	= sizeof(NOTIFYICONDATA);
		nid.hWnd	= hWndMain;
		nid.uID		= ICON_UID;

		Shell_NotifyIcon(NIM_DELETE,&nid);
		
		bIconAdded = FALSE;
	}

	// free the menu resource
	if (hMenu)
	{
		DestroyMenu(hMenu);
	}

	// destroy the main window
	if (hWndMain)
	{
		DestroyWindow(hWndMain);
		hWndMain = 0;
	}

	WSACleanup();

	return;
}

//******************************************************************************
// program entry point.  everything starts here.
//******************************************************************************
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nCmdShow)
{
	hInst = hInstance;

	// initialize
	if (Init())
	{
		// main loop.  this executes until the program exits
		MSG m;
		BOOL ret;
		while ((ret=GetMessage(&m,0,0,0)) != 0)
		{
			if (ret == -1)
			{
				break;
			}
			else
			{
				TranslateMessage(&m);
				DispatchMessage(&m);
			}
		}
	}

	// clean things up before exiting
	Shutdown();

	return 0;
}

