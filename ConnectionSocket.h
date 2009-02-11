#ifndef _CONNECTION_SOCKET_H
#define _CONNECTION_SOCKET_H

#include <map>
#include <string>

class CConnectionSocket
{
public:
	CConnectionSocket(int Socket=-1);
	~CConnectionSocket();

	operator int() const;
	
	bool Read();
	void Close();
	int Send();
	
	bool SendBufferEmpty() const;
	void AppendSendBuffer(const std::string& Buffer);
	
	std::string GetCommand();
		
protected:
	int m_Socket;
	std::string m_SendBuffer;
	std::string m_ReceiveBuffer;

	void AppendReceiveBuffer(const unsigned char *Buffer, int Received);
	std::string StripDelims(const std::string &Source, bool Front, bool Back) const;
};

typedef std::map<int,CConnectionSocket> tConnectionSocketMap;
typedef tConnectionSocketMap::const_iterator tConnectionSocketMapConstIterator;
typedef tConnectionSocketMap::iterator tConnectionSocketMapIterator;
	
#endif
