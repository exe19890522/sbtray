//******************************************************************************
// provides an options dialog box
//******************************************************************************

//******************************************************************************
// system headers
//******************************************************************************
#include <windows.h>

//******************************************************************************
// application headers
//******************************************************************************
#include "resource.h"
#include "sbtray_config.h"
#include "sbtray_options_dlg.h"

sbtray_parms dlgParms;

//******************************************************************************
// sets visibility of the Email parameters
//******************************************************************************
void ShowEmailParms(HWND hWndDlg,BOOL bVisible)
{
	int cmd = (bVisible ? SW_SHOW : SW_HIDE);

	ShowWindow(GetDlgItem(hWndDlg,IDC_SMTP_SERVER),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_MAIL_FROM),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_MAIL_TO),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_ML1),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_ML2),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_ML3),cmd);
	ShowWindow(GetDlgItem(hWndDlg,IDC_ML4),cmd);
}

//******************************************************************************
// transfers data between the comfig parms structure and the dialog
//******************************************************************************
void OptionsDlgDataTransfer(BOOL bLoadToDlg, HWND hWndDlg)
{
	// loading from the parms structure into the dialog
	if (bLoadToDlg)
	{
		SendDlgItemMessage(hWndDlg,IDC_ENABLE_SOUNDS,BM_SETCHECK,
			(dlgParms.bEnableSounds ? BST_CHECKED : BST_UNCHECKED),0);
		
		SetDlgItemInt(hWndDlg,IDC_UPDATE_PERIOD,dlgParms.dwUpdatePeriod,FALSE);

		SendDlgItemMessage(hWndDlg,IDC_ENABLE_LOGGING,BM_SETCHECK,
			(dlgParms.bEnableLogging ? BST_CHECKED : BST_UNCHECKED),0);

		SendDlgItemMessage(hWndDlg,IDC_ENABLE_EMAIL,BM_SETCHECK,
			(dlgParms.bEnableEmail ? BST_CHECKED : BST_UNCHECKED),0);

		ShowEmailParms(hWndDlg,dlgParms.bEnableEmail);

		SendDlgItemMessage(hWndDlg,IDC_SMTP_SERVER,WM_SETTEXT,
			0,(LPARAM)dlgParms.szSMTPServer);

		SendDlgItemMessage(hWndDlg,IDC_MAIL_FROM,WM_SETTEXT,
			0,(LPARAM)dlgParms.szMailFrom);

		SendDlgItemMessage(hWndDlg,IDC_MAIL_TO,WM_SETTEXT,
			0,(LPARAM)dlgParms.szMailTo);
	}
	// storing from the dialog to the parms structure
	else
	{
		dlgParms.bEnableSounds =
			(SendDlgItemMessage(hWndDlg,IDC_ENABLE_SOUNDS,BM_GETCHECK,0,0)
			== BST_CHECKED);

		dlgParms.dwUpdatePeriod = GetDlgItemInt(hWndDlg,IDC_UPDATE_PERIOD,0,FALSE);

		dlgParms.bEnableLogging =
			(SendDlgItemMessage(hWndDlg,IDC_ENABLE_LOGGING,BM_GETCHECK,0,0)
			== BST_CHECKED);

		dlgParms.bEnableEmail =
			(SendDlgItemMessage(hWndDlg,IDC_ENABLE_EMAIL,BM_GETCHECK,0,0)
			== BST_CHECKED);

		SendDlgItemMessage(hWndDlg,IDC_SMTP_SERVER,WM_GETTEXT,SBTRAY_PARMS_STRLEN+1,
			(LPARAM)dlgParms.szSMTPServer);

		SendDlgItemMessage(hWndDlg,IDC_MAIL_FROM,WM_GETTEXT,SBTRAY_PARMS_STRLEN+1,
			(LPARAM)dlgParms.szMailFrom);

		SendDlgItemMessage(hWndDlg,IDC_MAIL_TO,WM_GETTEXT,SBTRAY_PARMS_STRLEN+1,
			(LPARAM)dlgParms.szMailTo);
	}

	return;
}

//******************************************************************************
// handles messages for the dialog window
//******************************************************************************
WINAPI CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	// initialization message sent before dialog is displayed
	case WM_INITDIALOG:
		// load dialog content from the options
		OptionsDlgDataTransfer(TRUE,hDlg);
		break;

	// command message from the dialog
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		// user checked/unchecked the Enable Email checkbox
		case IDC_ENABLE_EMAIL:
			ShowEmailParms(hDlg,(SendDlgItemMessage(hDlg,IDC_ENABLE_EMAIL,BM_GETCHECK,0,0)
				== BST_CHECKED));
			break;

		// user clicked OK
		case IDOK:
			// store the options
			OptionsDlgDataTransfer(FALSE,hDlg);
			// end dialog
			EndDialog(hDlg,IDOK);
			break;

		// user clicked CANCEL
		case IDCANCEL:
			EndDialog(hDlg,IDCANCEL);
			break;
		}
		break;
	}

	return 0;
}

//******************************************************************************
// displayes the options dialog and updates the provided config parms
//******************************************************************************
void DoOptionsDlg(HINSTANCE hInst, HWND hWndParent, sbtray_parms * p)
{
	// sanity check
	if (!p)
		return;

	// store the provided parameters
	dlgParms = *p;

	// display the dialog
	if (DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS),hWndParent,OptionsDlgProc)
			== IDOK)
	{
		// return parameters
		*p = dlgParms;
	}

	return;
}

