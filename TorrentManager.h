#ifndef _TORRENT_MANAGER_H
#define _TORRENT_MANAGER_H

#define TORRENT_MAX_ALERT_TYPES 28
#include "libtorrent/alert.hpp"

#include "Torrent.h"

#include "libtorrent/session.hpp"
#include "libtorrent/alert_types.hpp"

#include "soapH.h"

class CTorrentManager
{
public:

	struct CAlertHandler
	{
			CAlertHandler(CTorrentManager *Manager)
			:	m_Manager(Manager)
			{
			}

			template <class T>
			void operator()(T const& Alert) const
			{
				if (m_Manager)
					m_Manager->HandleAlert(Alert);
			}

			CTorrentManager *m_Manager;
	};

	CTorrentManager(int UploadRate, int DownloadRate, const std::string& DownloadPath);
	~CTorrentManager();

	CStatus Status() const;
	std::string PauseTorrent(int TorrentNumber);
	std::string ResumeTorrent(int TorrentNumber);
	std::string PauseAll();
	std::string ResumeAll();

	void AddTorrentURL(CConnectionSocket& Socket, const std::string& TorrentURL);
	void AddTorrentFile(CConnectionSocket& Socket, const std::string& TorrentURL);
	void RemoveTorrent(CConnectionSocket& Socket, int Torrent);
	void ListTorrents(CConnectionSocket& Socket);
	void TorrentFiles(CConnectionSocket& Socket, int Torrent);
	void ExcludeFile(CConnectionSocket& Socket, int Torrent, int File);
	void IncludeFile(CConnectionSocket& Socket, int Torrent, int File);
	void PauseTorrent(CConnectionSocket& Socket, int Torrent);
	void ResumeTorrent(CConnectionSocket& Socket, int Torrent);
	void DisplayLimits(CConnectionSocket& Socket);

	void UpdateStartTimes();
	void CheckComplete();
	void CheckAlerts();

	void HandleAlert(libtorrent::tracker_alert const& Alert) const;
	void HandleAlert(libtorrent::tracker_warning_alert const& Alert) const;
	void HandleAlert(libtorrent::scrape_reply_alert const& Alert) const;
	void HandleAlert(libtorrent::scrape_failed_alert const& Alert) const;
	void HandleAlert(libtorrent::tracker_reply_alert const& Alert) const;
	void HandleAlert(libtorrent::tracker_announce_alert const& Alert) const;
	void HandleAlert(libtorrent::hash_failed_alert const& Alert) const;
	void HandleAlert(libtorrent::peer_ban_alert const& Alert) const;
	void HandleAlert(libtorrent::peer_error_alert const& Alert) const;
	void HandleAlert(libtorrent::invalid_request_alert const& Alert) const;
	void HandleAlert(libtorrent::torrent_finished_alert const& Alert) const;
	void HandleAlert(libtorrent::piece_finished_alert const& Alert) const;
	void HandleAlert(libtorrent::block_finished_alert const& Alert) const;
	void HandleAlert(libtorrent::block_downloading_alert const& Alert) const;
	void HandleAlert(libtorrent::storage_moved_alert const& Alert) const;
	void HandleAlert(libtorrent::torrent_deleted_alert const& Alert) const;
	void HandleAlert(libtorrent::torrent_paused_alert const& Alert) const;
	void HandleAlert(libtorrent::torrent_checked_alert const& Alert) const;
	void HandleAlert(libtorrent::url_seed_alert const& Alert) const;
	void HandleAlert(libtorrent::file_error_alert const& Alert) const;
	void HandleAlert(libtorrent::metadata_failed_alert const& Alert) const;
	void HandleAlert(libtorrent::metadata_received_alert const& Alert) const;
	void HandleAlert(libtorrent::listen_failed_alert const& Alert) const;
	void HandleAlert(libtorrent::listen_succeeded_alert const& Alert) const;
	void HandleAlert(libtorrent::portmap_error_alert const& Alert) const;
	void HandleAlert(libtorrent::portmap_alert const& Alert) const;
	void HandleAlert(libtorrent::fastresume_rejected_alert const& Alert) const;
	void HandleAlert(libtorrent::peer_blocked_alert const& Alert) const;

protected:
	libtorrent::session m_Session;
	tTorrentMap m_Torrents;
	int m_MaxTorrent;
	std::string m_DownloadPath;

	void AddTorrent(const libtorrent::entry& Entry, int TorrentNumber, const libtorrent::entry& Resume, CConnectionSocket *Socket);
	static int TorrentSelect(const struct dirent *Entry);
	void ScanTorrents();
	std::string FormatRate(float Rate) const;
	std::string FormatSize(libtorrent::size_type Size) const;
	std::string FormatTime(time_t Time) const;
	std::string FormatState(libtorrent::torrent_status::state_t State) const;
	int GetNextTorrentNumber();
};

#endif
