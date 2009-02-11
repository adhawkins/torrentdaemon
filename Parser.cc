#include "Parser.h"

#include <iostream>

CParser::CParser(const std::string& Message)
:	m_Message(Message)
{
	Parse();
}

std::string CParser::Command() const
{
	return m_Command;
}

std::vector<std::string> CParser::Parameters() const
{
	return m_Parameters;
}

void CParser::Parse()
{
	//Just split everything at each space. The command handlers will 
	//know how to handle arguments that have been split incorrectly
	
	m_Parameters.clear();
	
	std::string::size_type SpacePos=m_Message.find(' ');
	if (SpacePos!=std::string::npos)
	{
		std::string::size_type SearchStart=SpacePos+1;
			
		m_Command=m_Message.substr(0,SpacePos);
		
		do
		{
			SpacePos=m_Message.find(' ',SearchStart);
			
			if (SpacePos!=std::string::npos)
			{
				m_Parameters.push_back(m_Message.substr(SearchStart,SpacePos-SearchStart));
				SearchStart=SpacePos+1;
			}
			else
				m_Parameters.push_back(m_Message.substr(SearchStart));
				
		}	while (SpacePos!=std::string::npos);
	}
	else
		m_Command=m_Message;
}

std::string CParser::AllParameters() const
{
	std::string RetStr;

	std::vector<std::string>::const_iterator ThisParameter=m_Parameters.begin();
	while (ThisParameter!=m_Parameters.end())
	{
		if (!RetStr.empty())
			RetStr+=" ";
			
		RetStr+=*ThisParameter;
		
		++ThisParameter;
	}
	
	return RetStr;
}
