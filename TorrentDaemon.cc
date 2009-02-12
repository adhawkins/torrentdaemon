#include "TorrentDaemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <vector>

#include "TorrentManager.h"
#include "Parser.h"

#include "soapH.h"
#include "TorrentDaemon.nsmap"

static bool gv_DoExit=false;

CTorrentDaemon::CTorrentDaemon(bool DoDaemon, int Port, int SoapPort, int UploadRate, int DownloadRate, const std::string& DownloadPath)
:	m_ListenSocket(Port),
	m_DoDaemon(DoDaemon),
	m_Port(Port),
	m_SoapPort(SoapPort),
	m_Torrents(0),
	m_UploadRate(UploadRate),
	m_DownloadRate(DownloadRate),
	m_DownloadPath(DownloadPath)
{
	if (DoDaemon)
		BecomeDaemon();

	signal(SIGHUP,SignalHandler);
	signal(SIGTERM,SignalHandler);

	MainLoop();
}

CTorrentDaemon::~CTorrentDaemon()
{
	if (m_Torrents)
		delete m_Torrents;

	m_Torrents=0;
}

void CTorrentDaemon::SignalHandler(int sig)
{
	switch (sig)
	{
		case SIGHUP:
			break;

		case SIGTERM:
			gv_DoExit=true;
	}
}

bool CTorrentDaemon::BecomeDaemon()
{
	pid_t i=fork();

	//if fork failed, exit
	if (i<0)
	{
		perror("fork");
		exit (1);
	}

	//If we're the parent, exit

	if (i>0)
		exit(0);

	//Start a new process group

	setsid();

	//Close all open file descriptors

/*
	for (i=getdtablesize();i>=0;--i)
		close(i);
*/

	//Ignore SIG_CHLD

	signal(SIGCHLD,SIG_IGN);

	return true;
}

void CTorrentDaemon::MainLoop()
{
	m_Torrents=new CTorrentManager(m_UploadRate,m_DownloadRate,m_DownloadPath);

	struct soap soap;
	soap_init(&soap);

	#define uSec *-1
	#define mSec *-1000
	soap.accept_timeout = 100 mSec;
	soap.user = (void *)m_Torrents;
	soap.bind_flags=SO_REUSEADDR;

	if (m_ListenSocket.DoListen() && soap_bind(&soap, 0, m_SoapPort, 100)>=0)
	{
		while (!gv_DoExit)
		{
			HandleSockets();
			ProcessCommands();
			m_Torrents->UpdateStartTimes();
			m_Torrents->CheckComplete();
			m_Torrents->CheckAlerts();

			int s = soap_accept(&soap);
			if (s < 0)
			{
				if (soap.errnum)  //if timeout, errnum will be zero
					soap_print_fault(&soap, stderr);
			}
			else
			{
				if (soap_serve(&soap) != SOAP_OK)	// process RPC request
					soap_print_fault(&soap, stderr); // print error

				soap_destroy(&soap);	// clean up class instances
				soap_end(&soap);	// clean up everything and close socket
			}
		}

		soap_done(&soap);
	}

	tConnectionSocketMapConstIterator ThisSocket=m_Connections.begin();
	while (m_Connections.end()!=ThisSocket)
	{
		CConnectionSocket Socket=(*ThisSocket).second;
		Socket.Close();
		++ThisSocket;
	}

	m_ListenSocket.Close();

	delete m_Torrents;
	m_Torrents=0;
}

void CTorrentDaemon::HandleSockets()
{
	int MaxSocket=m_ListenSocket;

	struct timeval Timeout;
	bzero(&Timeout,sizeof(Timeout));
	Timeout.tv_sec=0;
	Timeout.tv_usec=100000;

	fd_set Read;
	FD_ZERO(&Read);
	FD_SET(m_ListenSocket,&Read);

	fd_set Write;
	FD_ZERO(&Write);

	tConnectionSocketMapConstIterator ThisSocket=m_Connections.begin();
	while (m_Connections.end()!=ThisSocket)
	{
		int Socket=(*ThisSocket).first;
		FD_SET(Socket,&Read);

		CConnectionSocket ConnectionSocket=(*ThisSocket).second;
		if (!ConnectionSocket.SendBufferEmpty())
			FD_SET(Socket,&Write);

		if (Socket>MaxSocket)
			MaxSocket=Socket;

		++ThisSocket;
	}

	MaxSocket++;

	int Ret=select(MaxSocket,&Read,&Write,NULL,&Timeout);
	if (Ret<0)
	{
		perror("select");
	}
	else if (Ret>0)
	{
		if (FD_ISSET(m_ListenSocket,&Read))
		{
			int NewSocket=m_ListenSocket.Accept();

			if (-1!=NewSocket)
				m_Connections[NewSocket]=CConnectionSocket(NewSocket);
		}

		std::vector<int> ClosedSockets;

		tConnectionSocketMapConstIterator ThisSocket=m_Connections.begin();
		while (m_Connections.end()!=ThisSocket)
		{
			int Socket=(*ThisSocket).first;
			CConnectionSocket ConnectionSocket=(*ThisSocket).second;
			bool Closed=false;

			if (FD_ISSET(Socket,&Read))
			{
				Closed=ConnectionSocket.Read();

				if (Closed)
					ClosedSockets.push_back(Socket);
			}

			if (FD_ISSET(Socket,&Write) && !Closed)
				ConnectionSocket.Send();

			m_Connections[Socket]=ConnectionSocket;
			++ThisSocket;
		}

		//Now go through closing any sockets that have closed

		std::vector<int>::const_iterator ClosedSocket=ClosedSockets.begin();
		while (ClosedSockets.end()!=ClosedSocket)
		{
			CConnectionSocket Socket=m_Connections[*ClosedSocket];

			Socket.Close();
			m_Connections.erase(*ClosedSocket);

			++ClosedSocket;
		}
	}
}

void CTorrentDaemon::ProcessCommands()
{
	std::vector<int> ClosedSockets;

	tConnectionSocketMapConstIterator ThisSocket=m_Connections.begin();
	while (ThisSocket!=m_Connections.end())
	{
		int SocketHandle=(*ThisSocket).first;
		CConnectionSocket Socket=(*ThisSocket).second;

		std::string Command=Socket.GetCommand();
		if (!Command.empty())
		{
			bool Closed=ProcessCommand(Socket,Command);
			if (Closed)
				ClosedSockets.push_back(SocketHandle);
		}

		m_Connections[SocketHandle]=Socket;

		++ThisSocket;
	}

	std::vector<int>::const_iterator ClosedSocket=ClosedSockets.begin();
	while (ClosedSockets.end()!=ClosedSocket)
	{
		m_Connections.erase(*ClosedSocket);

		++ClosedSocket;
	}
}

bool CTorrentDaemon::ProcessCommand(CConnectionSocket& Socket, const std::string& Command)
{
	bool Closed=false;

	if (!Command.empty())
	{
		CParser Parser(Command);

		if (Parser.Command()=="quit")
		{
			Socket.Close();
			Closed=true;
		}
		else if (Parser.Command()=="exit")
		{
			gv_DoExit=true;
		}
		else if (Parser.Command()=="help")
		{
			std::stringstream os;

			os << std::endl;

			os << "quit         - close this connection" << std::endl;
			os << "exit         - kill the daemon" << std::endl;
			os << "addurl url   - Add the specified url as a torrent to download" << std::endl;
			os << "addfile file - Add the specified file as a torrent to download" << std::endl;
			os << "list         - list the current torrents" << std::endl;
			os << "del n        - Delete torrent number 'n'" << std::endl;
			os << "files n      - list the files in torrent 'n'" << std::endl;
			os << "pause n      - pause torrent 'n'" << std::endl;
			os << "resume n     - resume torrent 'n'" << std::endl;
			os << "exclude n f  - exclude file number 'f' from torrent 'n'" << std::endl;
			os << "include n f  - include file number 'f' from torrent 'n'" << std::endl;
			os << "limits       - display current upload and download limits" << std::endl;

			os << std::endl;

			Socket.AppendSendBuffer(os.str());
		}
		else if (Parser.Command()=="addurl")
		{
			if (Parser.Parameters().size()==1)
				m_Torrents->AddTorrentURL(Socket,Parser.AllParameters());
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="addfile")
		{
			if (Parser.Parameters().size()==1)
				m_Torrents->AddTorrentFile(Socket,Parser.AllParameters());
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="del")
		{
			if (Parser.Parameters().size()==1)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				m_Torrents->RemoveTorrent(Socket,TorrentNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="list")
		{
			m_Torrents->ListTorrents(Socket);
		}
		else if (Parser.Command()=="files")
		{
			if (Parser.Parameters().size()==1)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				m_Torrents->TorrentFiles(Socket,TorrentNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="pause")
		{
			if (Parser.Parameters().size()==1)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				m_Torrents->PauseTorrent(Socket,TorrentNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="resume")
		{
			if (Parser.Parameters().size()==1)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				m_Torrents->ResumeTorrent(Socket,TorrentNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="exclude")
		{
			if (Parser.Parameters().size()==2)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				int FileNum=atoi(Parser.Parameters()[1].c_str());
				m_Torrents->ExcludeFile(Socket,TorrentNum,FileNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="include")
		{
			if (Parser.Parameters().size()==2)
			{
				int TorrentNum=atoi(Parser.Parameters()[0].c_str());
				int FileNum=atoi(Parser.Parameters()[1].c_str());
				m_Torrents->IncludeFile(Socket,TorrentNum,FileNum);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else if (Parser.Command()=="limits")
		{
			if (Parser.Parameters().size()==0)
			{
				m_Torrents->DisplayLimits(Socket);
			}
			else
				Socket.AppendSendBuffer("Invalid number of parameters\n");
		}
		else
			Socket.AppendSendBuffer("Invalid command " + Parser.Command() + "\n");
	}

	return Closed;
}

int torrentdaemon__Status(struct soap *Soap, CStatus& Status)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Status=Manager->Status();

	return SOAP_OK;
}

int torrentdaemon__Pause(struct soap *Soap, int TorrentNumber, std::string& Response)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Response=Manager->PauseTorrent(TorrentNumber);

	return SOAP_OK;
}

int torrentdaemon__Resume(struct soap *Soap, int TorrentNumber, std::string& Response)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Response=Manager->ResumeTorrent(TorrentNumber);

	return SOAP_OK;
}

int torrentdaemon__PauseAll(struct soap *Soap, std::string& Response)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Response=Manager->PauseAll();

	return SOAP_OK;
}

int torrentdaemon__ResumeAll(struct soap *Soap, std::string& Response)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Response=Manager->ResumeAll();

	return SOAP_OK;
}

int torrentdaemon__Delete(struct soap *Soap, int TorrentID, std::string& Response)
{
	CTorrentManager *Manager=(CTorrentManager *)Soap->user;

	if (Manager)
		Response=Manager->RemoveTorrent(TorrentID);

	return SOAP_OK;
}
