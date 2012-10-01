//******************************************************************************
// provides an about dialog box
//******************************************************************************

//******************************************************************************
// system headers
//******************************************************************************
#include <windows.h>

//******************************************************************************
// application headers
//******************************************************************************
#include "resource.h"
#include "sbtray_about_dlg.h"

HFONT hBoldFont = 0;

//******************************************************************************
// handles messages for the dialog window
//******************************************************************************
WINAPI CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;

	switch (uMsg)
	{
	// initialization message sent before dialog is displayed
	case WM_INITDIALOG:

		LOGFONT lf;
		ZeroMemory(&lf,sizeof(LOGFONT));
		hDC = GetDC(hDlg);
		lf.lfHeight = -MulDiv(8,GetDeviceCaps(hDC,LOGPIXELSY),72);
		ReleaseDC(hDlg,hDC);
		lf.lfWeight = FW_BOLD;
		strcpy(lf.lfFaceName,"MS Sans Serif");
		hBoldFont = CreateFontIndirect(&lf);

		SendMessage(GetDlgItem(hDlg,IDC_TITLE),WM_SETFONT,(WPARAM)hBoldFont,0);

		break;

	// command message from the dialog
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		// user clicked OK
		case IDOK:
			// clean up
			if (hBoldFont)
			{
				DeleteObject(hBoldFont);
				hBoldFont = 0;
			}

			// end dialog
			EndDialog(hDlg,IDOK);
			break;
		}
		break;
	}

	return 0;
}

//******************************************************************************
// displayes the about dialog
//******************************************************************************
void DoAboutDlg(HINSTANCE hInst, HWND hWndParent)
{
	// display the dialog
	DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),hWndParent,AboutDlgProc);

	return;
}

