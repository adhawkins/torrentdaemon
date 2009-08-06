#include "HTTPFetch.h"

#include "ne_session.h"
#include "ne_string.h"
#include "ne_request.h"

CHTTPFetch::CHTTPFetch()
{
}

int CHTTPFetch::Fetch(const std::string& URL)
{
	int Ret=0;
	
	m_Data.clear();
			
	ne_uri uri={0};
	
	if (0==ne_uri_parse(URL.c_str(), &uri))
	{
		if (uri.port == 0)
			uri.port = ne_uri_defaultport(uri.scheme);

		ne_sock_init();
	
		ne_session *sess=ne_session_create("http", uri.host, uri.port);
		if (sess) 
		{
			ne_set_useragent(sess, "torrentdaemon");
	
			ne_request *req = ne_request_create(sess, "GET", uri.path);	
			ne_add_response_body_reader(req, ne_accept_2xx, httpResponseReader, &m_Data);

			m_Result = ne_request_dispatch(req);
			m_Status = ne_get_status(req)->code;

			Ret=m_Data.size();
			
			ne_request_destroy(req); 
	
			m_ErrorMessage = ne_get_error(sess);

			ne_session_destroy(sess);
		}
	}
	
	return Ret;
}

int CHTTPFetch::httpResponseReader(void *userdata, const char *buf, size_t len)
{
	std::vector<unsigned char> *buffer = reinterpret_cast<std::vector<unsigned char> *>(userdata);

	buffer->insert(buffer->end(),buf,buf+len);

	return 0;
}

std::vector<unsigned char> CHTTPFetch::Data() const
{
	return m_Data;
}

int CHTTPFetch::Result() const
{
	return m_Result;
}

int CHTTPFetch::Status() const
{
	return m_Status;
}

std::string CHTTPFetch::ErrorMessage() const
{
	return m_ErrorMessage;
}
