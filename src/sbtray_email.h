//******************************************************************************
// sends an Email message
//******************************************************************************
#ifndef SBTRAY_EMAIL
#define SBTRAY_EMAIL

void SendEmailAsynch(LPCTSTR szSMTPServer,
					 LPCTSTR szMailFrom,
					 LPCTSTR szMailTo,
					 LPCTSTR szSubject,
					 LPCTSTR szMessage);

#endif
