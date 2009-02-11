#ifndef _TORRENT_DAEMON_H
#define _TORRENT_DAEMON_H

#include "ListenSocket.h"
#include "ConnectionSocket.h"

#include <string>

class CTorrentManager;

class CTorrentDaemon
{
public:
	CTorrentDaemon(bool DoDaemon, int Port, int SoapPort, int UploadRate, int DownloadRate, const std::string& DownloadPath);
	~CTorrentDaemon();
	
protected:
	CListenSocket m_ListenSocket;
	tConnectionSocketMap m_Connections;
	bool m_DoDaemon;
	int m_Port;
	int m_SoapPort;
	CTorrentManager *m_Torrents;
	int m_UploadRate;
	int m_DownloadRate;
	std::string m_DownloadPath;
	
	bool BecomeDaemon();
	void MainLoop();
	void HandleSockets();
	void ProcessCommands();
	bool ProcessCommand(CConnectionSocket& Socket, const std::string& Command);
	
	static void SignalHandler(int sig);
};

#endif
