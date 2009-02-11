#include "ConnectionSocket.h"

#include <sys/socket.h>
#include <netinet/in.h>

CConnectionSocket::CConnectionSocket(int Socket)
:	m_Socket(Socket)
{
}

CConnectionSocket::~CConnectionSocket()
{
}

void CConnectionSocket::Close()
{
	if (-1!=m_Socket)
	{
		shutdown(m_Socket,SHUT_RDWR);
		close(m_Socket);
	}
	
	m_Socket=-1;
}

CConnectionSocket::operator int() const
{
	return m_Socket;
}

bool CConnectionSocket::SendBufferEmpty() const
{
	return m_SendBuffer.empty();
}

bool CConnectionSocket::Read()
{
	bool Closed=false;
	
	unsigned char *Buffer=new unsigned char[1024];

	int Received=recv(m_Socket,Buffer,1024,0);
	if (Received<0)
		perror("recv");
	else if (0==Received)
		Closed=true;
	else
		AppendReceiveBuffer(Buffer,Received);

	delete[] Buffer;
	return Closed;
}

void CConnectionSocket::AppendReceiveBuffer(const unsigned char *Buffer, int Received)
{
	for (int count=0;count<Received;count++)
		m_ReceiveBuffer+=Buffer[count];
}

void CConnectionSocket::AppendSendBuffer(const std::string& Buffer)
{
	m_SendBuffer+=Buffer;
}

int CConnectionSocket::Send()
{
	int Sent=send(m_Socket,m_SendBuffer.c_str(),m_SendBuffer.length(),0);
	if (Sent<0)
		perror("send");
	else
		m_SendBuffer=m_SendBuffer.substr(Sent);
		
	return Sent;
}

std::string CConnectionSocket::GetCommand()
{
	std::string Command;
		
	std::string::size_type FirstDelim=m_ReceiveBuffer.find_first_of("\n\r",0);
	if (FirstDelim!=std::string::npos)
	{
		Command=m_ReceiveBuffer.substr(0,FirstDelim);
		m_ReceiveBuffer=m_ReceiveBuffer.substr(FirstDelim);
		
		Command=StripDelims(Command,true,true);
		m_ReceiveBuffer=StripDelims(m_ReceiveBuffer,true,false);
	}
	
	return Command;
}

std::string CConnectionSocket::StripDelims(const std::string &Source, bool Front, bool Back) const
{
	std::string StrippedString=Source;
			
	if (!Source.empty())
	{
		while (Front && (StrippedString.substr(0,1)=="\n" || StrippedString.substr(0,1)=="\r"))
			StrippedString=StrippedString.substr(1);
			
		while (Back && (StrippedString.substr(StrippedString.length()-1)=="\n" || StrippedString.substr(StrippedString.length()-1)=="\r"))
			StrippedString=StrippedString.substr(0,StrippedString.length()-1);
	}		
	
	return StrippedString;
}
