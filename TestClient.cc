#include "soapTorrentDaemonProxy.h"
#include "TorrentDaemon.nsmap"

#include <sstream>
#include <iomanip>

std::string FormatRate(float Rate);
std::string FormatSize(size_t Size);

int main(int argc, char * const argv[])
{
	TorrentDaemon Daemon;

	int Port=1235;
	std::string Host="localhost";

	int Option=-1;

	do
	{
		Option=getopt(argc,argv,"h:p:");

		if (-1!=Option)
		{
			switch (Option)
			{
				case 'p':
					Port=atoi(optarg);
					break;

				case 'h':
					Host=optarg;
					break;
			}
		}
	} while (Option!=-1);

	std::stringstream os;
	os << "http://" << Host << ":" << Port << "/torrentdaemon";
	std::string Endpoint=os.str();
	Daemon.endpoint = Endpoint.c_str();

	std::string Response;

	if (SOAP_OK==Daemon.torrentdaemon__Delete(23,Response))
	{
		std::cout << "Delete 23 returns '" << Response << "'" << std::endl;
	}
	else
	{
		printf("Endpoint is '%s'\n",Daemon.endpoint);
		soap_print_fault(Daemon.soap, stderr);
	}
	
	if (SOAP_OK==Daemon.torrentdaemon__Delete(23,Response))
	{
		std::cout << "Delete 23 returns '" << Response << "'" << std::endl;
	}
	else
	{
		printf("Endpoint is '%s'\n",Daemon.endpoint);
		soap_print_fault(Daemon.soap, stderr);
	}
	
/*
	if (SOAP_OK==Daemon.torrentdaemon__ResumeAll(Response))
	{
		std::cout << "Resume all returns '" << Response << "'" << std::endl;
	}
	else
	{
		printf("Endpoint is '%s'\n",Daemon.endpoint);
		soap_print_fault(Daemon.soap, stderr);
	}

	sleep(5);

	CStatus Status;

	if (SOAP_OK==Daemon.torrentdaemon__Status(Status))
	{
		if (1==Status.m_NumTorrents)
			std::cout << "There is 1 torrent.";
		else
			std::cout << "There are " << Status.m_NumTorrents << " torrents.";

		std::cout << " Upload: " << FormatRate(Status.m_UploadRate);
		std::cout << " Download: " << FormatRate(Status.m_DownloadRate) << std::endl;
		std::cout << "Peers: " << Status.m_Peers << " Num connections: " << Status.m_Connections << " Num uploads: " << Status.m_Uploads << std::endl << std::endl;

		std::vector<CTorrentInfo>::const_iterator ThisTorrent=Status.m_Torrents.begin();
		while (ThisTorrent!=Status.m_Torrents.end())
		{
			CTorrentInfo Torrent=(*ThisTorrent);

			std::cout << Torrent.m_TorrentNumber << ": " << FormatSize(Torrent.m_WantedDone)
							<< " of " << FormatSize(Torrent.m_Wanted)
							<< " (" << Torrent.m_State << " - " << Torrent.m_Progress << "%)"
							<< " Uploaded: " << FormatSize(Torrent.m_Uploaded);

			if (Torrent.m_Paused)
				std::cout << " - * PAUSED *";
			else
				std::cout << " - Running";

			std::cout << std::endl;

			std::cout << "\tUpload: " << FormatRate(Torrent.m_UploadRate)
							<< ", Download: " << FormatRate(Torrent.m_DownloadRate)
							<< ", Remaining: " << Torrent.m_Remaining << std::endl;

			std::vector<CTorrentFile>::const_iterator ThisFile=Torrent.m_Files.begin();
			while (ThisFile!=Torrent.m_Files.end())
			{
				CTorrentFile File=(*ThisFile);

				std::cout << "  " << File.m_Name;

				if (File.m_Excluded)
					std::cout << " *";

				std::cout << std::endl;

				++ThisFile;
			}

			std::cout << std::endl;

			++ThisTorrent;
		}
	}
	else
	{
		printf("Endpoint is '%s'\n",Daemon.endpoint);
		soap_print_fault(Daemon.soap, stderr);
	}

	sleep(5);

	if (SOAP_OK==Daemon.torrentdaemon__PauseAll(Response))
	{
		std::cout << "Pause 22 returns '" << Response << "'" << std::endl;
	}
	else
	{
		printf("Endpoint is '%s'\n",Daemon.endpoint);
		soap_print_fault(Daemon.soap, stderr);
	}
*/
}

std::string FormatRate(float Rate)
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

std::string FormatSize(size_t Size)
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

