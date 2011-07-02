#include "Torrent.h"

#include <iomanip>
#include <fstream>

#include "libtorrent/bencode.hpp"

CTorrent::CTorrent(const libtorrent::torrent_handle& Torrent, int Number)
:	m_Torrent(Torrent),
	m_StartTime(0),
	m_StartDone(0),
	m_Complete(false),
	m_StartUploaded(0),
	m_Number(Number)
{
	try
	{
		if (Torrent.is_valid())
		{
			libtorrent::torrent_info Info=m_Torrent.get_torrent_info();
			m_ExcludeFiles.resize(Info.num_files());
		}
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << ": " << m_Number << " - " << e.what() << "\n";
	}
}

void CTorrent::SetStartTime(time_t StartTime)
{
	if (m_StartTime==0 || m_StartDone==0)
	{
		try
		{
			libtorrent::torrent_status Status=m_Torrent.status();
			if (Status.state==libtorrent::torrent_status::downloading)
			{
				m_StartDone=Status.total_wanted_done;
				m_StartTime=StartTime;
			}
		}

		catch (std::exception& e)
		{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
		}
	}
}

std::string CTorrent::GetRemaining() const
{
	std::string Ret="Unknown";

	if (!IsPaused())
	{
		try
		{
			libtorrent::torrent_status Status=m_Torrent.status();

			int BytesRemaining=Status.total_wanted-Status.total_wanted_done;
			switch (Status.state)
			{
				case libtorrent::torrent_status::finished:
				case libtorrent::torrent_status::seeding:
					Ret="Complete";
					break;

				case libtorrent::torrent_status::downloading:
					if (m_StartDone==0)
						m_StartDone=Status.total_wanted_done;

					if (BytesRemaining>0)
					{
						time_t TimeTaken=time(NULL)-m_StartTime;

						if (TimeTaken>0)
						{
							int BytesDone=Status.total_wanted_done-m_StartDone;
							double BytesPerSec=(double)BytesDone/(double)TimeTaken;

							if (BytesPerSec>0.5)
							{
								time_t Remaining=(time_t)((double)BytesRemaining/BytesPerSec);

								Ret=FormatTime(Remaining);
							}
						}
					}
					break;

				case libtorrent::torrent_status::queued_for_checking:
				case libtorrent::torrent_status::checking_files:
				case libtorrent::torrent_status::downloading_metadata:
				case libtorrent::torrent_status::allocating:
				case libtorrent::torrent_status::checking_resume_data:
					break;
			}
		}

		catch (std::exception& e)
		{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
		}
	}

	return Ret;
}

CTorrent::operator libtorrent::torrent_handle() const
{
	return m_Torrent;
}

libtorrent::size_type CTorrent::WantedDone() const
{
	libtorrent::size_type Ret=0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=Status.total_wanted_done;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

libtorrent::size_type CTorrent::Wanted() const
{
	libtorrent::size_type Ret=0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=Status.total_wanted;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

libtorrent::size_type CTorrent::Uploaded() const
{
	libtorrent::size_type Ret=0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=Status.total_payload_upload+m_StartUploaded;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

float CTorrent::UploadRate() const
{
	float Ret=0.0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=Status.upload_payload_rate;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

float CTorrent::DownloadRate() const
{
	float Ret=0.0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=Status.download_payload_rate;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

std::string CTorrent::Name() const
{
	std::string Ret;

	try
	{
		libtorrent::torrent_info Info=m_Torrent.get_torrent_info();
		Ret=Info.name();
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

int CTorrent::Progress() const
{
	int Ret=0;

	try
	{
		libtorrent::torrent_status Status=m_Torrent.status();
		Ret=(int)(Status.progress*100.0);
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return Ret;
}

libtorrent::torrent_status::state_t CTorrent::State() const
{
	libtorrent::torrent_status::state_t Ret;

	if (m_Complete)
		Ret=libtorrent::torrent_status::finished;
	else
	{
		try
		{
			Ret=m_Torrent.status().state;
		}

		catch (std::exception& e)
		{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
		}
	}

	return Ret;
}

std::vector<bool> CTorrent::ExcludeFiles() const
{
	return m_ExcludeFiles;
}

std::vector<std::string> CTorrent::Files() const
{
	std::vector<std::string> Ret;

	libtorrent::torrent_info Info=m_Torrent.get_torrent_info();
	libtorrent::torrent_info::file_iterator ThisFile=Info.begin_files();
	while (ThisFile!=Info.end_files())
	{
		libtorrent::file_entry File=(*ThisFile);

		std::string FileName=File.path.string();
		std::string::size_type SlashPos=FileName.rfind('/');
		if (SlashPos!=std::string::npos)
			FileName=FileName.substr(SlashPos+1);

		Ret.push_back(FileName);

		++ThisFile;
	}

	return Ret;
}

void CTorrent::SetExcludeFiles(std::vector<bool> ExcludeFiles)
{
	if (ExcludeFiles.size()==m_ExcludeFiles.size())
	{
		std::vector<int> Priorities;
		Priorities.resize(m_ExcludeFiles.size());
		for (std::vector<int>::size_type count=0;count<m_ExcludeFiles.size();count++)
		{
			if (ExcludeFiles[count])
				Priorities[count]=0;
			else
				Priorities[count]=1;
		}

		try
		{
			m_ExcludeFiles=ExcludeFiles;
			m_Torrent.prioritize_files(Priorities);
		}

		catch (std::exception& e)
		{
	  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
		}
	}
	else
		std::cout << "Invalid size (" << ExcludeFiles.size() << " - " << m_ExcludeFiles.size() << std::endl;
}

void CTorrent::SaveStatus(const std::string& File) const
{
	try
	{
		libtorrent::entry::dictionary_type ExcludeFiles;
		libtorrent::entry::dictionary_type StatusFile;
		libtorrent::entry::dictionary_type State;

		State["complete"]=libtorrent::entry(m_Complete?1:0);
		State["uploaded"]=libtorrent::entry(Uploaded());
		StatusFile["state"]=State;

		for (std::vector<bool>::size_type count=0;count<m_ExcludeFiles.size();count++)
		{
			std::stringstream os;
			os << count;

			ExcludeFiles[os.str()]=m_ExcludeFiles[count];
		}

		StatusFile["exclude"]=ExcludeFiles;

		try
		{
			//Pause the torrent and save the resume status

			m_Torrent.pause();

			libtorrent::entry Resume=m_Torrent.write_resume_data();
			if (Resume.type()==libtorrent::entry::dictionary_t)
				StatusFile["resume"]=Resume;
		}

		catch (std::exception& e)
		{
	  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
		}

		std::ofstream out(File.c_str(), std::ios_base::binary);
		out.unsetf(std::ios_base::skipws);

		bencode(std::ostream_iterator<char>(out),StatusFile);
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}
}

void CTorrent::LoadState(const libtorrent::entry& StatusFile)
{
	try
	{
		const libtorrent::entry *State=StatusFile.find_key("state");
		if (State)
		{
			const libtorrent::entry *Complete=State->find_key("complete");
			m_Complete=Complete->integer()?true:false;
			if (m_Complete)
				Pause();

			const libtorrent::entry *Uploaded=State->find_key("uploaded");
			if (Uploaded)
				m_StartUploaded=Uploaded->integer();
		}
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}
}

libtorrent::entry CTorrent::ExtractResume(const libtorrent::entry& StatusFile)
{
	libtorrent::entry Resume;

	try
	{
		const libtorrent::entry *Search=StatusFile.find_key("resume");
		if (Search)
			Resume=*Search;
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << " - " << e.what() << "\n";
	}

	return Resume;
}
libtorrent::entry CTorrent::LoadStatus(const std::string& File)
{
	libtorrent::entry Ret;

	try
	{
		std::ifstream in(File.c_str(), std::ios_base::binary);
		in.unsetf(std::ios_base::skipws);
		Ret = libtorrent::bdecode(std::istream_iterator<char>(in), std::istream_iterator<char>());
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << " - " << e.what() << "\n";
	}

	return Ret;
}

void CTorrent::SetExcludeFiles(const libtorrent::entry& StatusFile)
{
	try
	{
		const libtorrent::entry *Excludes=StatusFile.find_key("exclude");
		if (Excludes)
		{
			std::vector<bool> ExcludeFiles=m_ExcludeFiles;

			for (std::vector<bool>::size_type count=0;count<ExcludeFiles.size();count++)
			{
				std::stringstream os;
				os << count;

				const libtorrent::entry *ExcludeFile=Excludes->find_key(os.str().c_str());
				if (ExcludeFile)
					ExcludeFiles[count]=ExcludeFile->integer()?true:false;

				SetExcludeFiles(ExcludeFiles);
			}
		}
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}
}

void CTorrent::Pause()
{
	try
	{
		m_Torrent.pause();
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}
}

void CTorrent::Resume()
{
	try
	{
		m_Torrent.resume();
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}
}

bool CTorrent::IsPaused() const
{
	bool RetVal=false;

	try
	{
		RetVal=m_Torrent.is_paused();
	}

	catch (std::exception& e)
	{
  		std::cout << __PRETTY_FUNCTION__ << ": " << m_Number << " - " << e.what() << "\n";
	}

	return RetVal;
}

std::string CTorrent::FormatTime(time_t Time) const
{
	int Hours=Time/3600;
	int Left=Time%3600;
	int Mins=Left/60;
	int Secs=Left%60;

	std::stringstream os;
	os << std::setw(2) << std::setfill('0') << Hours << ":"
		<< std::setw(2) << std::setfill('0') << Mins << ":"
		<< std::setw(2) << std::setfill('0') << Secs;

	return os.str();
}

void CTorrent::SetComplete()
{
	m_Complete=true;
	Pause();
}
