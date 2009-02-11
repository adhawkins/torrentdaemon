#ifndef _LISTEN_SOCKET_H
#define _LISTEN_SOCKET_H

class CListenSocket
{
public:
	CListenSocket(int Port);
	~CListenSocket();
	
	operator int() const;
	
	bool DoListen();
	int Accept();
	void Close();

protected:
	int m_Port;
	int m_Socket;
	
};

#endif
