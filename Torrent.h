#ifndef _TORRENT_H
#define _TORRENT_H

#include "ConnectionSocket.h"

#include <vector>
#include <map>

#include "libtorrent/torrent_handle.hpp"

class CTorrent
{
public:
	CTorrent(const libtorrent::torrent_handle& Torrent=libtorrent::torrent_handle(), int Number=0);

	operator libtorrent::torrent_handle() const;
		
	void SaveStatus(const std::string& File) const;
	static libtorrent::entry LoadStatus(const std::string& file);
	static libtorrent::entry ExtractResume(const libtorrent::entry& StatusFile);
	void SetExcludeFiles(const libtorrent::entry& StatusFile);
		
	libtorrent::size_type Uploaded() const;
	libtorrent::size_type WantedDone() const;
	libtorrent::size_type Wanted() const;
	float UploadRate() const;
	float DownloadRate() const;
	libtorrent::torrent_status::state_t State() const;
	int Progress() const;
	std::string Name() const;
	std::string GetRemaining() const;

	void SetStartTime(time_t StartTime);
	void SetComplete();
	
	std::vector<std::string> Files() const;
	std::vector<bool> ExcludeFiles() const;
	void SetExcludeFiles(std::vector<bool> ExcludeFiles);
	void LoadState(const libtorrent::entry& StatusFile);
					
	void Pause();
	void Resume();
	bool IsPaused() const;
			
protected:
	libtorrent::torrent_handle m_Torrent;
	std::vector<bool> m_ExcludeFiles;
	time_t m_StartTime;
	mutable libtorrent::size_type m_StartDone;
	bool m_Complete;
	libtorrent::size_type m_StartUploaded;
	int m_Number;
			
	std::string FormatTime(time_t Time) const;
};

typedef std::map<int,CTorrent> tTorrentMap;
typedef tTorrentMap::const_iterator tTorrentMapConstIterator;
typedef tTorrentMap::iterator tTorrentMapIterator;
	
#endif
