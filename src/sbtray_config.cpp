//******************************************************************************
// provides persistent options
//******************************************************************************

//******************************************************************************
// system headers
//******************************************************************************
#include <windows.h>

//******************************************************************************
// application headers
//******************************************************************************
#include "sbtray_config.h"

//******************************************************************************
// constants
//******************************************************************************
#define TOPLEVEL_REGKEY        "Software\\coder_1024\\sbtray"
#define REGVALUE_ENABLESOUNDS  "bEnableSounds"
#define REGVALUE_UPDATEPERIOD  "dwUpdatePeriod"
#define REGVALUE_ENABLELOGGING "bEnableLogging"
#define REGVALUE_ENABLEEMAIL   "bEnableEmail"
#define REGVALUE_SMTPSERVER    "szSMTPServer"
#define REGVALUE_MAILFROM      "szMailFrom"
#define REGVALUE_MAILTO        "szMailTo"
#define REGVALUE_SELECTEDWORLD "dwSelectedWorld"

//******************************************************************************
// initialize parameters structure to default values
//******************************************************************************
void InitConfig(sbtray_parms * p)
{
	// sanity check
	if (!p)
		return;

	ZeroMemory(p,sizeof(sbtray_parms));

	p->bEnableSounds   = FALSE;
	p->dwUpdatePeriod  = 30;
	p->bEnableLogging  = FALSE;
	p->bEnableEmail    = FALSE;
	p->dwSelectedWorld = 0;

	return;
}

//******************************************************************************
// load parameters from the registry
//******************************************************************************
void LoadConfig(sbtray_parms * p)
{
	LONG err;
	HKEY hKey;
	DWORD value;
	DWORD value_size;

#define REGLOAD_DWORD(vn,vv) \
		value_size = sizeof(DWORD); \
		err = RegQueryValueEx(hKey,vn,0,0, \
					(LPBYTE)&value,&value_size); \
		if ((err == ERROR_SUCCESS) && (value_size == sizeof(DWORD))) \
		{ \
			vv = value; \
		}

#define REGLOAD_STR(vn,vv) \
		ZeroMemory(vv,SBTRAY_PARMS_STRLEN+1); \
		value_size = SBTRAY_PARMS_STRLEN+1; \
		err = RegQueryValueEx(hKey,vn,0,0, \
					(LPBYTE)vv,&value_size); \
		if (err != ERROR_SUCCESS) \
		{ \
			vv[0] = '\0'; \
		}

	// sanity check
	if (!p)
		return;

	InitConfig(p);

	err = RegOpenKeyEx(HKEY_CURRENT_USER,TOPLEVEL_REGKEY,
				0,KEY_READ,&hKey);
	if (err == ERROR_SUCCESS)
	{
		REGLOAD_DWORD( REGVALUE_ENABLESOUNDS,  p->bEnableSounds )
		REGLOAD_DWORD( REGVALUE_UPDATEPERIOD,  p->dwUpdatePeriod )
		REGLOAD_DWORD( REGVALUE_ENABLELOGGING, p->bEnableLogging )
		REGLOAD_DWORD( REGVALUE_ENABLEEMAIL,   p->bEnableEmail )
		REGLOAD_STR(   REGVALUE_SMTPSERVER,    p->szSMTPServer )
		REGLOAD_STR(   REGVALUE_MAILFROM,      p->szMailFrom )
		REGLOAD_STR(   REGVALUE_MAILTO,        p->szMailTo )
		REGLOAD_DWORD( REGVALUE_SELECTEDWORLD, p->dwSelectedWorld )

		RegCloseKey(hKey);
	}
	
	return;
}

//******************************************************************************
// save parameters to the registry
//******************************************************************************
void SaveConfig(sbtray_parms * p)
{
	LONG err;
	HKEY hKey;
	DWORD disp;
	DWORD value;

#define REGSAVE_DWORD(vn,vv) \
		value = vv; \
		RegSetValueEx(hKey,vn,0,REG_DWORD, \
			(LPBYTE)&value,sizeof(DWORD));

#define REGSAVE_STR(vn,vv) \
		RegSetValueEx(hKey,vn,0,REG_SZ, \
			(LPBYTE)vv,strlen(vv)+1);

	// sanity check
	if (!p)
		return;

	err = RegCreateKeyEx(HKEY_CURRENT_USER,TOPLEVEL_REGKEY,
				0,0,0,KEY_WRITE,0,&hKey,&disp);
	if (err == ERROR_SUCCESS)
	{
		REGSAVE_DWORD( REGVALUE_ENABLESOUNDS,  p->bEnableSounds )
		REGSAVE_DWORD( REGVALUE_UPDATEPERIOD,  p->dwUpdatePeriod )
		REGSAVE_DWORD( REGVALUE_ENABLELOGGING, p->bEnableLogging )
		REGSAVE_DWORD( REGVALUE_ENABLEEMAIL,   p->bEnableEmail )
		REGSAVE_STR(   REGVALUE_SMTPSERVER,    p->szSMTPServer )
		REGSAVE_STR(   REGVALUE_MAILFROM,      p->szMailFrom )
		REGSAVE_STR(   REGVALUE_MAILTO,        p->szMailTo )
		REGSAVE_DWORD( REGVALUE_SELECTEDWORLD, p->dwSelectedWorld )

		RegCloseKey(hKey);
	}

	return;
}

