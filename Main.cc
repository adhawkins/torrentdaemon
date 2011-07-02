#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "TorrentDaemon.h"

int ParseRate(const std::string& RateStr);

int main(int argc, char * const argv[])
{
	bool DoDaemon=true;
	int Port=1234;
	int SoapPort=1235;
	int DownloadRate=-1;
	int UploadRate=-1;
	std::string DownloadPath="";
	bool ShowHelp=false;

	int Option=-1;

	do
	{
		Option=getopt(argc,argv,"d:u:r:s:p:fh");

		if (-1!=Option)
		{
			switch (Option)
			{
				case 'p':
					Port=atoi(optarg);
					break;

				case 'r':
					SoapPort=atoi(optarg);
					break;

				case 'd':
					DownloadRate=ParseRate(optarg);
					break;

				case 'u':
					UploadRate=ParseRate(optarg);
					break;

				case 's':
					DownloadPath=optarg;
					break;

				case 'f':
					DoDaemon=false;
					break;
			}
		}
	} while (Option!=-1);

	if (ShowHelp || DownloadPath.empty())
	{
		std::cout << "Usage: " << argv[0] << " -u upload -d download -s downloadpath [ -f ] [ -p port ] [ -r soap port ]" << std::endl << std::endl;

		std::cout << "Upload and download rates are in bits/second." << std::endl;
		std::cout << "Modifiers 'K' and 'M' can be applied if required." << std::endl << std::endl;

		std::cout << "Specify -f to run in the forground" << std::endl;
		std::cout << "Specify -p option to set listen port" << std::endl;
	}
	else
	{
		CTorrentDaemon Daemon(DoDaemon,Port,SoapPort,UploadRate,DownloadRate,DownloadPath);
	}
}

int ParseRate(const std::string& RateStr)
{
	int Rate=-1;

	if (isdigit(RateStr[RateStr.length()-1]))
		Rate=atoi(RateStr.c_str());
	else
	{
		std::string NumberStr=RateStr.substr(0,RateStr.length()-1);
		float TmpRate=atof(NumberStr.c_str());

		switch (RateStr[RateStr.length()-1])
		{
			case 'K':
			case 'k':
				TmpRate*=1024;
				break;

			case 'm':
			case 'M':
				TmpRate*=1024*1024;
				break;

			default:
				TmpRate=0.0;
				std::cout << "Invalid multiplier" << std::endl;
				break;
		}

		Rate=(int)TmpRate;
	}

	Rate=(int)((double)Rate/8.0);

	return Rate;
}
