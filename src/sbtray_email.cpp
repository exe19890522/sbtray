//******************************************************************************
// sends Email
//******************************************************************************

//******************************************************************************
// system headers
//******************************************************************************
#include <windows.h>
#include <time.h>

//******************************************************************************
// application headers
//******************************************************************************
#include "sbtray_email.h"

//******************************************************************************
// constants
//******************************************************************************
// buffer size
#define EMAIL_BUFSIZE 1024
// string length
#define EMAIL_STRLEN  1024
// end-of-command terminator for SMTP
#define SMTP_EOC      "\xd\xa"

//******************************************************************************
// data structure used by the Email worker thread
//******************************************************************************
typedef struct
{
	char szSMTPServer[EMAIL_STRLEN+1];
	char szMailFrom[EMAIL_STRLEN+1];
	char szMailTo[EMAIL_STRLEN+1];
	char szSubject[EMAIL_STRLEN+1];
	char szMessage[EMAIL_STRLEN+1];

} email_parms;

//******************************************************************************
// send a command to the SMTP server
//******************************************************************************
void smtpPut(SOCKET sock, char * buf)
{
	// send command
	send(sock,buf,strlen(buf),0);

	// send command terminator
	send(sock,SMTP_EOC,strlen(SMTP_EOC),0);
}

//******************************************************************************
// get a response from the SMTP server
//******************************************************************************
void smtpGet(SOCKET sock, char * buf)
{
	// reset buffer to empty
	buf[0] = '\0';

	// holding buffer used for each incoming char
	char b[2];
	b[1] = '\0';

	// read byte-by-byte response
	while (recv(sock,b,1,0))
	{
		// handle end-of-command chars
		if (b[0] == '\xd')
			continue;
		if (b[0] == '\xa')
			break;

		// append to buffer
		strcat(buf,b);
	}
}


//******************************************************************************
// sends the specified Email
//******************************************************************************
DWORD WINAPI _SendEmailAsynch(LPVOID lpParm)
{
	// get parms passed to us as an arg
	email_parms * p = (email_parms*)lpParm;
	if (p)
	{
		// initialize WinSock
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2,0),&wsaData) == 0)
		{
			// create the client socket
			SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
			if (sock != INVALID_SOCKET)
			{
				// initialize SMTP server address/port structure
				struct sockaddr_in sin;
				ZeroMemory(&sin,sizeof(sin));
				sin.sin_family = AF_INET;	
				struct servent * se = getservbyname("smtp","tcp");
				sin.sin_port = se->s_port;
				if (isdigit(p->szSMTPServer[0]))
				{
					struct in_addr iadr;
					iadr.s_addr = inet_addr(p->szSMTPServer);
					sin.sin_addr = iadr;
				}
				else
				{
					struct hostent * he = gethostbyname(p->szSMTPServer);
					sin.sin_addr = *((LPIN_ADDR)*he->h_addr_list);
				}

				// connect to SMTP server
				if (connect(sock,(const struct sockaddr*)&sin,sizeof(sin)) != SOCKET_ERROR)
				{
					// send the Email using the SMTP server

					char sbuf[EMAIL_BUFSIZE+1];

					smtpGet(sock,sbuf);
					
					smtpPut(sock,"HELO localhost");
					smtpGet(sock,sbuf);
					
					wsprintf(sbuf,"MAIL FROM:<%s>",p->szMailFrom);
					smtpPut(sock,sbuf);
					smtpGet(sock,sbuf);

					char tobuf[EMAIL_BUFSIZE+1];
					strcpy(tobuf,p->szMailTo);
					char * nextEmail = strtok(tobuf,",");
					while (nextEmail)
					{
						wsprintf(sbuf,"RCPT TO:<%s>",nextEmail);
						smtpPut(sock,sbuf);
						smtpGet(sock,sbuf);

						nextEmail = strtok(0,",");
					}

					smtpPut(sock,"DATA");
					smtpGet(sock,sbuf);

					time_t tt = time(0);
					struct tm * t = localtime(&tt);
					strftime(sbuf,EMAIL_BUFSIZE,"Date: %a, %d %b %Y %H:%M:%S",t);
					smtpPut(sock,sbuf);
					
					wsprintf(sbuf,"From: %s", p->szMailFrom);
					smtpPut(sock,sbuf);

					wsprintf(sbuf,"Subject: %s", p->szSubject);
					smtpPut(sock,sbuf);

					wsprintf(sbuf,"To: %s%s", p->szMailTo, SMTP_EOC);
					smtpPut(sock,sbuf);

					smtpPut(sock,p->szMessage);

					smtpPut(sock,"\xd\xa\xd\xa.");

					smtpGet(sock,sbuf);

					smtpPut(sock,"QUIT");
					smtpGet(sock,sbuf);

				}

				// close the socket
				closesocket(sock);
			}

			// cleanup WinSock
			WSACleanup();
		}

		// free parms
		free(p);
	}

	return 0;
}

//******************************************************************************
// sends an Email
//******************************************************************************
void SendEmailAsynch(LPCTSTR szSMTPServer,
					 LPCTSTR szMailFrom,
					 LPCTSTR szMailTo,
					 LPCTSTR szSubject,
					 LPCTSTR szMessage)
{
	if (szSMTPServer &&
		szMailFrom &&
		szMailTo &&
		szSubject &&
		szMessage)
	{
		email_parms * p = (email_parms*)malloc(sizeof(email_parms));
		
		strcpy(p->szSMTPServer,szSMTPServer);
		strcpy(p->szMailFrom,szMailFrom);
		strcpy(p->szMailTo,szMailTo);
		strcpy(p->szSubject,szSubject);
		strcpy(p->szMessage,szMessage);

		DWORD threadID;
		CreateThread(0,0,_SendEmailAsynch,(LPVOID)p,0,&threadID);
	}

	return;
}

