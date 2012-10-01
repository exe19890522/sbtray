//******************************************************************************
// provides persistent options
//******************************************************************************
#ifndef SBTRAY_CONFIG_H
#define SBTRAY_CONFIG_H

#define SBTRAY_PARMS_STRLEN 1024

typedef struct
{
	// controls whether or not sound effects are played when the
	// game server state changes
	BOOL bEnableSounds;

	// how often the WarCry status page is checked (secs)
	DWORD dwUpdatePeriod;

	// controls whether or not server status is logged
	BOOL bEnableLogging;

	// controls whether or not Email notification is used
	BOOL bEnableEmail;

	// if Email is enabled, the following are used
	char szSMTPServer[SBTRAY_PARMS_STRLEN+1];
	char szMailFrom[SBTRAY_PARMS_STRLEN+1];
	char szMailTo[SBTRAY_PARMS_STRLEN+1];

	// which world is currently selected for display
	// zero-based value which indexes into the array of
	// world objects
	DWORD dwSelectedWorld;

} sbtray_parms;

void InitConfig(sbtray_parms * p);
void LoadConfig(sbtray_parms * p);
void SaveConfig(sbtray_parms * p);

#endif
