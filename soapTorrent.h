//gsoap torrentdaemon service name: TorrentDaemon
//gsoap torrentdaemon service namespace: urn:torrentdaemon
//gsoap torrentdaemon service location: http://localhost:18083/torrentdaemon

#import "stlvector.h"

class CTorrentFile
{
	std::string m_Name;
	bool m_Excluded;
}

class CTorrentInfo
{
	int m_TorrentNumber;
	size_t m_WantedDone;
	size_t m_Wanted;
	std::string m_State;
	int m_Progress;
	bool m_Paused;
	size_t m_Uploaded;
	float m_UploadRate;
	float m_DownloadRate;
	std::string m_Remaining;
	std::vector<CTorrentFile> m_Files;
};

class CStatus
{
	int m_NumTorrents;
	float m_UploadRate;
	float m_DownloadRate;
	int m_Peers;
	int m_Connections;
	int m_Uploads;
	std::vector<CTorrentInfo> m_Torrents;
};

int torrentdaemon__Status(CStatus& Status);
int torrentdaemon__Pause(int TorrentID, std::string& Response);
int torrentdaemon__Resume(int TorrentID, std::string& Response);
int torrentdaemon__PauseAll(std::string& Response);
int torrentdaemon__ResumeAll(std::string& Response);
