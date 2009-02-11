#ifndef _PARSER_H
#define _PARSER_H

#include <string>
#include <vector>

class CParser
{
public:
	CParser(const std::string& Message);
	
	std::string Command() const;
	std::vector<std::string> Parameters() const;
	std::string AllParameters() const;
			
protected:
	std::string m_Message;
	std::string m_Command;
	std::vector<std::string> m_Parameters;
		
	void Parse();
};

#endif
