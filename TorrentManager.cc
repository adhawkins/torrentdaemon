#include "TorrentManager.h"

#include <dirent.h>

#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "HTTPFetch.h"

#include "libtorrent/bencode.hpp"

#include "log4cpp/Category.hh"
#include "log4cpp/SyslogAppender.hh"
#include "log4cpp/SimpleLayout.hh"

CTorrentManager::CTorrentManager(int UploadRate, int DownloadRate, const std::string& DownloadPath)
:	m_MaxTorrent(1),
	m_DownloadPath(DownloadPath)
{
	//boost::filesystem::path::default_name_check(boost::filesystem::native);

	m_Session.listen_on(std::make_pair(6881, 6889));

	m_Session.set_upload_rate_limit(UploadRate);
	m_Session.set_download_rate_limit(DownloadRate);

	m_Session.set_severity_level(libtorrent::alert::debug);
	m_Session.set_severity_level(libtorrent::alert::warning);

	libtorrent::session_settings SessionSettings;
	SessionSettings.user_agent="ADH - Torrent Daemon/1.0 Alpha libtorrent/v0.13";
	m_Session.set_settings(SessionSettings);

	log4cpp::Appender *app=new log4cpp::SyslogAppender("SyslogAppender","torrentdaemon");
	log4cpp::Layout *layout=new log4cpp::SimpleLayout;
	app->setLayout(layout);

	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.setAdditivity(false);
	Category.setAppender(app);
	Category.setPriority(log4cpp::Priority::DEBUG);

	ScanTorrents();
}

CTorrentManager::~CTorrentManager()
{
	tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
	while (ThisTorrent!=m_Torrents.end())
	{
		int TorrentNum=(*ThisTorrent).first;
		CTorrent Torrent=(*ThisTorrent).second;

		try
		{
			Torrent.Pause();

			std::stringstream File;
			File << m_DownloadPath << "/" << TorrentNum << ".status";

			Torrent.SaveStatus(File.str());
			m_Session.remove_torrent(Torrent);

		}

		catch (std::exception& e)
		{
	  		std::cout << __PRETTY_FUNCTION__ << " - " << e.what() << "\n";
		}

		++ThisTorrent;
	}
}

void CTorrentManager::AddTorrent(const libtorrent::entry& Entry, int TorrentNumber, const libtorrent::entry& Resume, CConnectionSocket *Socket)
{
	try
	{
		std::stringstream os;
		os << m_DownloadPath << "/" << TorrentNumber;

		boost::intrusive_ptr<libtorrent::torrent_info> Info=new libtorrent::torrent_info(Entry);
		libtorrent::torrent_handle Handle=m_Session.add_torrent(Info, os.str(), Resume, libtorrent::storage_mode_sparse, false, libtorrent::default_storage_constructor, 0);
		Handle.set_ratio(1.0);
		CTorrent Torrent(Handle,TorrentNumber);

		m_Torrents[TorrentNumber]=Torrent;

		if (Socket)
			Socket->AppendSendBuffer("Torrent added successfully\n");
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";

  		if (Socket)
	  		Socket->AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::AddTorrentURL(CConnectionSocket& Socket, const std::string& TorrentURL)
{
	try
	{
		CHTTPFetch Fetcher;
		char *Buffer=0;
		int Bytes=Fetcher.Fetch(TorrentURL.c_str());
		if (Bytes>0)
		{
			int Number=GetNextTorrentNumber();
			std::stringstream TorrentFile;
			TorrentFile << m_DownloadPath << "/" << Number << ".torrent";

			FILE *fptr=fopen(TorrentFile.str().c_str(),"wb");
			if (fptr)
			{
				std::vector<unsigned char> Data=Fetcher.Data();
				fwrite(&Data[0], 1, Bytes, fptr);
				fclose(fptr);
			}

			libtorrent::entry Entry = libtorrent::bdecode(Buffer,Buffer+Bytes);
			AddTorrent(Entry,Number,libtorrent::entry(),&Socket);
		}
		else
			Socket.AppendSendBuffer(std::string("HTTP download error: ") + Fetcher.ErrorMessage() + "\n");

		if (Buffer)
			free(Buffer);
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::AddTorrentFile(CConnectionSocket& Socket, const std::string& TorrentFile)
{
	std::ifstream in(TorrentFile.c_str(), std::ios_base::binary);
	in.unsetf(std::ios_base::skipws);

	int Number=GetNextTorrentNumber();
	std::stringstream OutFile;
	OutFile << m_DownloadPath << "/" << Number << ".torrent";

	std::ofstream out(OutFile.str().c_str(), std::ios_base::binary);
	in.unsetf(std::ios_base::skipws);

	std::istream_iterator<char> InIt(in);
	std::ostream_iterator<char> OutIt(out);
	copy(InIt,std::istream_iterator<char>(),OutIt);

	try
	{
		std::ifstream TorrentIn(TorrentFile.c_str(), std::ios_base::binary);
		TorrentIn.unsetf(std::ios_base::skipws);
		libtorrent::entry Entry = libtorrent::bdecode(std::istream_iterator<char>(TorrentIn), std::istream_iterator<char>());
		AddTorrent(Entry,Number,libtorrent::entry(),&Socket);
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::RemoveTorrent(CConnectionSocket& Socket, int Torrent)
{
	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.find(Torrent);
		if (ThisTorrent!=m_Torrents.end())
		{
			CTorrent RemoveTorrent=(*ThisTorrent).second;
			m_Session.remove_torrent(RemoveTorrent);
			m_Torrents.erase(Torrent);

			std::stringstream TorrentFile;
			TorrentFile << m_DownloadPath << "/" << Torrent << ".torrent";
			unlink(TorrentFile.str().c_str());
		}
		else
			Socket.AppendSendBuffer("Invalid torrent ID\n");
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

CStatus CTorrentManager::Status () const
{
	libtorrent::session_status Status=m_Session.status();

	CStatus RetStatus;

	RetStatus.m_NumTorrents=m_Torrents.size();
	RetStatus.m_UploadRate=Status.upload_rate;
	RetStatus.m_DownloadRate=Status.download_rate;
	RetStatus.m_Peers=Status.num_peers;
	RetStatus.m_Connections=m_Session.num_connections();
	RetStatus.m_Uploads=m_Session.num_uploads();

	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
		while (ThisTorrent!=m_Torrents.end())
		{
			int TorrentNum=(*ThisTorrent).first;
			CTorrent Torrent=(*ThisTorrent).second;

			CTorrentInfo RetTorrent;

			RetTorrent.m_TorrentNumber=TorrentNum;
			RetTorrent.m_WantedDone=Torrent.WantedDone();
			RetTorrent.m_Wanted=Torrent.Wanted();

			std::stringstream os;
			os << Torrent.State();

			RetTorrent.m_State=os.str();
			RetTorrent.m_Progress=Torrent.Progress();
			RetTorrent.m_Uploaded=Torrent.Uploaded();
			RetTorrent.m_Paused=Torrent.IsPaused();
			RetTorrent.m_UploadRate=Torrent.UploadRate();
			RetTorrent.m_DownloadRate=Torrent.DownloadRate();
			RetTorrent.m_Remaining=Torrent.GetRemaining();

			std::vector<std::string> Files=Torrent.Files();
			std::vector<bool> ExcludeFiles=Torrent.ExcludeFiles();

			for (std::vector<std::string>::size_type count=0;count<Files.size();count++)
			{
				CTorrentFile File;
				File.m_Name=Files[count];
				File.m_Excluded=ExcludeFiles[count];

				RetTorrent.m_Files.push_back(File);
			}

			RetStatus.m_Torrents.push_back(RetTorrent);

			++ThisTorrent;
		}
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
	}
	return RetStatus;
}

void CTorrentManager::ListTorrents(CConnectionSocket& Socket)
{
	std::stringstream Response;

	if (1==m_Torrents.size())
		Response << "There is 1 torrent.";
	else
		Response << "There are " << m_Torrents.size() << " torrents.";

	libtorrent::session_status Status=m_Session.status();

	Response << " Upload: " << FormatRate(Status.upload_rate);
	Response << " Download: " << FormatRate(Status.download_rate) << std::endl;
	Response << "Peers: " << Status.num_peers << " Num connections: " << m_Session.num_connections() << " Num uploads: " << m_Session.num_uploads() << std::endl << std::endl;

	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
		while (ThisTorrent!=m_Torrents.end())
		{
			int TorrentNum=(*ThisTorrent).first;
			CTorrent Torrent=(*ThisTorrent).second;

			Response << TorrentNum << ": " << FormatSize(Torrent.WantedDone())
							<< " of " << FormatSize(Torrent.Wanted())
							<< " (" << FormatState(Torrent.State()) << " - " << Torrent.Progress() << "%)"
							<< " Uploaded: " << FormatSize(Torrent.Uploaded());

			if (Torrent.IsPaused())
				Response << " - * PAUSED *";
			else
				Response << " - Running";

			Response << std::endl;

			Response << "\tUpload: " << FormatRate(Torrent.UploadRate())
							<< ", Download: " << FormatRate(Torrent.DownloadRate())
							<< ", Remaining: " << Torrent.GetRemaining() << std::endl;

			Response << std::endl;

			++ThisTorrent;
		}

		Socket.AppendSendBuffer(Response.str());
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::ScanTorrents()
{
	struct dirent **Files=0;

	int NumFiles=scandir(m_DownloadPath.c_str(),
									&Files,
									TorrentSelect,
									alphasort);

	for (int count=0;count<NumFiles;count++)
	{
		try
		{
			std::string NumberStr=Files[count]->d_name;
			std::string::size_type DotPos=NumberStr.find(".");
			if (DotPos!=std::string::npos)
				NumberStr=NumberStr.substr(0,DotPos);

			int Number=atoi(NumberStr.c_str());
			if (Number>=m_MaxTorrent)
				m_MaxTorrent=Number+1;

			std::string FileName=m_DownloadPath+"/"+Files[count]->d_name;

			std::ifstream in(FileName.c_str(), std::ios_base::binary);
			in.unsetf(std::ios_base::skipws);

			std::string StatusFile=m_DownloadPath+"/"+NumberStr+".status";

			libtorrent::entry Resume;
			libtorrent::entry Status;
			try
			{
				Status=CTorrent::LoadStatus(StatusFile);

				Resume=CTorrent::ExtractResume(Status);
			}

			catch (std::exception& e)
			{
		  		std::cout << __PRETTY_FUNCTION__ << " - " << e.what() << "\n";
			}

			unlink(StatusFile.c_str());

			libtorrent::entry Entry = libtorrent::bdecode(std::istream_iterator<char>(in), std::istream_iterator<char>());
			AddTorrent(Entry,Number,Resume,0);

			tTorrentMapConstIterator ThisTorrent=m_Torrents.find(Number);
			if (ThisTorrent!=m_Torrents.end())
			{
				CTorrent Torrent=(*ThisTorrent).second;

				Torrent.SetExcludeFiles(Status);
				Torrent.LoadState(Status);

				m_Torrents[Number]=Torrent;
			}
		}

		catch (std::exception& e)
		{
	  		std::cout << __PRETTY_FUNCTION__ << " - " << e.what() << "\n";
		}

		free(Files[count]);
	}

	if (Files)
		free(Files);
}

int CTorrentManager::TorrentSelect(const struct dirent *Entry)
{
	if (strstr(Entry->d_name,".torrent"))
		return 1;
	else
		return 0;
}

void CTorrentManager::TorrentFiles(CConnectionSocket& Socket, int Torrent)
{
	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.find(Torrent);
		if (ThisTorrent!=m_Torrents.end())
		{
			CTorrent Torrent=(*ThisTorrent).second;

			std::vector<std::string> Files=Torrent.Files();
			std::vector<bool> ExcludeFiles=Torrent.ExcludeFiles();

			int Excluded=0;

			for (std::vector<bool>::size_type count=0;count<ExcludeFiles.size();count++)
				if (ExcludeFiles[count])
					Excluded++;

			std::stringstream Response;
			Response << Files.size() << " files, " << Excluded << " excluded" << std::endl;

			for (std::vector<std::string>::size_type count=0;count<Files.size();count++)
			{
				Response << count+1 << ": " << Files[count];
				if (ExcludeFiles[count])
					Response << " *";

				Response << std::endl;
			}

			Socket.AppendSendBuffer(Response.str());
		}
		else
			Socket.AppendSendBuffer("Invalid torrent ID\n");
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::PauseTorrent(CConnectionSocket& Socket, int Torrent)
{
	tTorrentMapConstIterator ThisTorrent=m_Torrents.find(Torrent);
	if (ThisTorrent!=m_Torrents.end())
	{
		CTorrent Torrent=(*ThisTorrent).second;

		Torrent.Pause();
	}
	else
		Socket.AppendSendBuffer("Invalid torrent ID\n");
}

void CTorrentManager::ResumeTorrent(CConnectionSocket& Socket, int Torrent)
{
	tTorrentMapConstIterator ThisTorrent=m_Torrents.find(Torrent);
	if (ThisTorrent!=m_Torrents.end())
	{
		CTorrent Torrent=(*ThisTorrent).second;

		Torrent.Resume();
	}
	else
		Socket.AppendSendBuffer("Invalid torrent ID\n");
}

void CTorrentManager::ExcludeFile(CConnectionSocket& Socket, int TorrentNum, int File)
{
	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.find(TorrentNum);
		if (ThisTorrent!=m_Torrents.end())
		{
			CTorrent Torrent=(*ThisTorrent).second;

			std::vector<bool> ExcludeFiles=Torrent.ExcludeFiles();

			if (File>0 && (std::vector<bool>::size_type)File<ExcludeFiles.size()+1)
			{
				ExcludeFiles[File-1]=true;
				Torrent.SetExcludeFiles(ExcludeFiles);
				m_Torrents[TorrentNum]=Torrent;
			}
			else
				Socket.AppendSendBuffer("Invalid file number\b");
		}
		else
			Socket.AppendSendBuffer("Invalid torrent ID\n");
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

void CTorrentManager::IncludeFile(CConnectionSocket& Socket, int TorrentNum, int File)
{
	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.find(TorrentNum);
		if (ThisTorrent!=m_Torrents.end())
		{
			CTorrent Torrent=(*ThisTorrent).second;

			std::vector<bool> ExcludeFiles=Torrent.ExcludeFiles();

			if (File>0 && (std::vector<bool>::size_type)File<ExcludeFiles.size()+1)
			{
				ExcludeFiles[File-1]=false;
				Torrent.SetExcludeFiles(ExcludeFiles);
				m_Torrents[TorrentNum]=Torrent;
			}
			else
				Socket.AppendSendBuffer("Invalid file number\b");
		}
		else
			Socket.AppendSendBuffer("Invalid torrent ID\n");
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Socket.AppendSendBuffer(std::string(e.what()) + "\n");
	}
}

std::string CTorrentManager::FormatRate(float Rate) const
{
	std::stringstream os;

	if (Rate>1024.0*1024.0*1024.0)
		os << std::setprecision(3) << Rate/(1024.0*1024.0*1024.0) << " GB/Sec";
	else if (Rate>1024.0*1024.0)
		os << std::setprecision(3) << Rate/(1024.0*1024.0) << " MB/Sec";
	else if (Rate>1024)
		os << std::setprecision(3) << Rate/1024.0 << " KB/Sec";
	else
		os << std::setprecision(3) << Rate << " B/Sec";

	return os.str();
}

void CTorrentManager::UpdateStartTimes()
{
	tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
	while (ThisTorrent!=m_Torrents.end())
	{
		int Number=(*ThisTorrent).first;
		CTorrent Torrent=(*ThisTorrent).second;

		Torrent.SetStartTime(time(NULL));
		m_Torrents[Number]=Torrent;

		++ThisTorrent;
	}
}

void CTorrentManager::CheckAlerts()
{
	try
	{
		std::auto_ptr<libtorrent::alert> Alert=m_Session.pop_alert();

		CAlertHandler AlertHandler(this);
		while (Alert.get())
		{
			try
			{
				libtorrent::handle_alert<
																	libtorrent::tracker_alert,
																	libtorrent::tracker_warning_alert,
																	libtorrent::scrape_reply_alert,
																	libtorrent::scrape_failed_alert,
																	libtorrent::tracker_reply_alert,
																	libtorrent::tracker_announce_alert,
																	libtorrent::hash_failed_alert,
																	libtorrent::peer_ban_alert,
																	libtorrent::peer_error_alert,
																	libtorrent::invalid_request_alert,
																	libtorrent::torrent_finished_alert,
																	libtorrent::piece_finished_alert,
																	libtorrent::block_finished_alert,
																	libtorrent::block_downloading_alert,
																	libtorrent::storage_moved_alert,
																	libtorrent::torrent_deleted_alert,
																	libtorrent::torrent_paused_alert,
																	libtorrent::torrent_checked_alert,
																	libtorrent::url_seed_alert,
																	libtorrent::file_error_alert,
																	libtorrent::metadata_failed_alert,
																	libtorrent::metadata_received_alert,
																	libtorrent::listen_failed_alert,
																	libtorrent::listen_succeeded_alert,
																	libtorrent::portmap_error_alert,
																	libtorrent::portmap_alert,
																	libtorrent::fastresume_rejected_alert,
																	libtorrent::peer_blocked_alert
																>::handle_alert(Alert, AlertHandler);
			}
			catch (std::exception& e)
			{
				//std::cout << "Got alert of type " << typeid(Alert.get()).name() << " - " << Alert.get()->msg() << std::endl;
			}

			Alert=m_Session.pop_alert();
		}
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << " outside - " << e.what() << "\n";
	}

}
void CTorrentManager::CheckComplete()
{
	tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
	while (ThisTorrent!=m_Torrents.end())
	{
		int Number=(*ThisTorrent).first;
		CTorrent Torrent=(*ThisTorrent).second;

		if (!Torrent.IsPaused() && Torrent.State()==libtorrent::torrent_status::seeding &&
				Torrent.Wanted()!=0 && Torrent.Uploaded()>((double)Torrent.Wanted()*1.5))
		{
			Torrent.SetComplete();
			m_Torrents[Number]=Torrent;
		}

		++ThisTorrent;
	}
}

std::string CTorrentManager::FormatSize(libtorrent::size_type Size) const
{
	std::stringstream os;

	if (Size>1024*1024*1024)
		os << std::fixed << std::setprecision(1) << (double)Size/(1024.0*1024.0*1024.0) << " GB";
	else if (Size>1024*1024)
		os << std::fixed << std::setprecision(1) << (double)Size/(1024.0*1024.0) << " MB";
	else if (Size>1024)
		os << std::fixed << std::setprecision(1) << (double)Size/1024.0 << " KB";
	else
		os << Size << " B";

	return os.str();
}

std::string CTorrentManager::FormatState(libtorrent::torrent_status::state_t State) const
{
	std::string Ret;

	switch (State)
	{
		case libtorrent::torrent_status::queued_for_checking:
			Ret="Queued for checking";
			break;

		case libtorrent::torrent_status::checking_files:
			Ret="Checking files";
			break;

		case libtorrent::torrent_status::connecting_to_tracker:
			Ret="Connecting to tracker";
			break;

		case libtorrent::torrent_status::downloading:
			Ret="Downloading";
			break;

		case libtorrent::torrent_status::downloading_metadata:
			Ret="Downloading metadata";
			break;

		case libtorrent::torrent_status::finished:
			Ret="Finished";
			break;

		case libtorrent::torrent_status::seeding:
			Ret="Seeding";
			break;

		case libtorrent::torrent_status::allocating:
			Ret="Allocating";
			break;
	}

	return Ret;
}

int CTorrentManager::GetNextTorrentNumber()
{
	int Number=m_MaxTorrent;

	bool FileExists=true;

	while(FileExists)
	{
		struct stat Status;
		std::stringstream DownloadPath;
		DownloadPath << m_DownloadPath << "/" << Number;

		FileExists=(0==stat(DownloadPath.str().c_str(),&Status));
		if (FileExists)
		{
			std::cout << "Torrent " << DownloadPath.str() << " exists." << std::endl;
			++Number;
		}
	}

	m_MaxTorrent=Number+1;
	return Number;
}

void CTorrentManager::DisplayLimits(CConnectionSocket& Socket)
{
	std::stringstream Response;

	Response << "Upload rate: " << FormatRate(m_Session.upload_rate_limit()) << ", Download rate: " << FormatRate(m_Session.download_rate_limit()) << std::endl;
	Socket.AppendSendBuffer(Response.str());
}

void CTorrentManager::HandleAlert(libtorrent::tracker_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.warn(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::tracker_warning_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.warn(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::scrape_reply_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::scrape_failed_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.error(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::tracker_reply_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::tracker_announce_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::hash_failed_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::peer_ban_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::peer_error_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.debug(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::invalid_request_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.debug(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::torrent_finished_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::piece_finished_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.debug(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::block_finished_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.debug(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::block_downloading_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.debug(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::storage_moved_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::torrent_deleted_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::torrent_paused_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::torrent_checked_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::url_seed_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.warn(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::file_error_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.fatal(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::metadata_failed_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::metadata_received_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::listen_failed_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.fatal(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::listen_succeeded_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::portmap_error_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.warn(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::portmap_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::fastresume_rejected_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.warn(Alert.msg());
}

void CTorrentManager::HandleAlert(libtorrent::peer_blocked_alert const& Alert) const
{
	log4cpp::Category& Category=log4cpp::Category::getInstance("main_cat");
	Category.info(Alert.msg());
}

std::string CTorrentManager::PauseTorrent(int TorrentNumber)
{
	std::string Response="Success";

	tTorrentMapConstIterator ThisTorrent=m_Torrents.find(TorrentNumber);
	if (ThisTorrent!=m_Torrents.end())
	{
		CTorrent Torrent=(*ThisTorrent).second;

		Torrent.Pause();
	}
	else
		Response="Invalid torrent ID";

	return Response;
}

std::string CTorrentManager::ResumeTorrent(int TorrentNumber)
{
	std::string Response="Success";

	tTorrentMapConstIterator ThisTorrent=m_Torrents.find(TorrentNumber);
	if (ThisTorrent!=m_Torrents.end())
	{
		CTorrent Torrent=(*ThisTorrent).second;

		Torrent.Resume();
	}
	else
		Response="Invalid torrent ID";

	return Response;
}

std::string CTorrentManager::PauseAll()
{
	std::stringstream Response;

	if (m_Torrents.size())
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
		while (ThisTorrent!=m_Torrents.end())
		{
			int TorrentNum=(*ThisTorrent).first;
			CTorrent Torrent=(*ThisTorrent).second;

			Torrent.Pause();

			Response << "Torrent " << TorrentNum << " paused" << std::endl;

			++ThisTorrent;
		}
	}
	else
		Response << "No torrents found" << std::endl;

	return Response.str();
}

std::string CTorrentManager::ResumeAll()
{
	std::stringstream Response;

	if (m_Torrents.size())
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.begin();
		while (ThisTorrent!=m_Torrents.end())
		{
			int TorrentNum=(*ThisTorrent).first;
			CTorrent Torrent=(*ThisTorrent).second;

			Torrent.Resume();

			Response << "Torrent " << TorrentNum << " resumed" << std::endl;

			++ThisTorrent;
		}
	}
	else
		Response << "No torrents found" << std::endl;

	return Response.str();
}

std::string CTorrentManager::RemoveTorrent(int TorrentID)
{
	std::string Status="Torrent deleted";
		
	try
	{
		tTorrentMapConstIterator ThisTorrent=m_Torrents.find(TorrentID);
		if (ThisTorrent!=m_Torrents.end())
		{
			CTorrent RemoveTorrent=(*ThisTorrent).second;
			m_Session.remove_torrent(RemoveTorrent);
			m_Torrents.erase(TorrentID);

			std::stringstream TorrentFile;
			TorrentFile << m_DownloadPath << "/" << TorrentID << ".torrent";
			unlink(TorrentFile.str().c_str());
		}
		else
			Status="Invalid torrent ID";
	}

	catch (std::exception& e)
	{
  		std::cout << e.what() << "\n";
  		Status=e.what();
	}
	
	return Status;
}
