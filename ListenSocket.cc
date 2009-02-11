#include "ListenSocket.h"

#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>

CListenSocket::CListenSocket(int Port)
:	m_Port(Port),
	m_Socket(-1)
{
}

CListenSocket::~CListenSocket()
{
	Close();
}

void CListenSocket::Close()
{
	if (-1!=m_Socket)
	{
		shutdown(m_Socket,SHUT_RDWR);
		close(m_Socket);
	}
	
	m_Socket=-1;
}

CListenSocket::operator int() const
{
	return m_Socket;
}

bool CListenSocket::DoListen()
{
	bool Ret=false;

	if (!m_Port)
		return -1;

	m_Socket=socket(PF_INET,SOCK_STREAM,0);
	if (-1 != m_Socket) 
	{
		struct sockaddr_in local_addr;

		bzero(&local_addr, sizeof(local_addr));
		local_addr.sin_family=AF_INET;
		local_addr.sin_port=htons(m_Port);
		local_addr.sin_addr.s_addr=htonl(INADDR_ANY);

		int On=1;
		
		if(!setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &On, sizeof(On)))
		{
			if (0==bind(m_Socket,(struct sockaddr *)&local_addr,sizeof(local_addr))) 
			{
	 			if (0==listen(m_Socket,SOMAXCONN)) 
				{
					Ret=true;
				}
				else 
				{
					perror("Listen");
					close(m_Socket);
					m_Socket=-1;
				}
			}
			else
			{
				perror("bind");
				close(m_Socket);
				m_Socket=-1;
			}
		}
		else
		{
			perror("setsockopt");
			close(m_Socket);
			m_Socket=-1;
		}
	}
	else
		perror("Socket");

	return Ret;
}

int CListenSocket::Accept()
{
	struct sockaddr_in Addr;
	socklen_t AddrSize=sizeof(Addr);
	
	int NewSock=accept(m_Socket,(sockaddr *)&Addr,&AddrSize);
	if (NewSock<0)
		perror("accept");
	
	return NewSock;
}
