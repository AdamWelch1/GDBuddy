#include "gdbmi_private.h"

#include "gdbmi.h"
#ifndef SOMETHING_UNIQUE_GDBMI_H
// #include "gdbmi_parse.h"
#endif

GDBMI::ParseItemType GDBMI::getItemType(char x)
{
	ParseItemType ret = ParseItemType::Invalid;
	
	// *INDENT-OFF*
	switch(x)
	{
		case '"': ret = ParseItemType::String; 	break;
		case '[': ret = ParseItemType::List; 	break;
		case '{': ret = ParseItemType::Tuple; 	break;
	}
	// *INDENT-ON*
	
	return ret;
}

string GDBMI::parserGetItem(string &str)
{
	char startChar = 0;
	if(str[0] == '[')
		startChar = '[';
	else if(str[0] == '{')
		startChar = '{';
	else if(str[0] == '"')
		startChar = '"';
		
	switch(startChar)
	{
		case '[':
		{
			int32_t tokenCnt = 1;
			int32_t endPos = -1;
			for(int32_t i = 1; i < (int32_t) str.length(); i++)
			{
				if(str[i] == '[') tokenCnt++;
				if(str[i] == ']') tokenCnt--;
				
				if(tokenCnt <= 0)
				{
					endPos = i;
					break;
				}
			}
			
			if(endPos == -1) // Invalid list item. Return empty string
				return "";
				
			string ret = str.substr(0, endPos + 1);
			str = str.substr(endPos + 1);
			
			if(str.length() > 0 && str[0] == ',')
				str.erase(str.begin());
				
			return ret;
		}
		break;
		
		case '{':
		{
			int32_t tokenCnt = 1;
			int32_t endPos = -1;
			for(int32_t i = 1; i < (int32_t) str.length(); i++)
			{
				if(str[i] == '{') tokenCnt++;
				if(str[i] == '}') tokenCnt--;
				
				if(tokenCnt <= 0)
				{
					endPos = i;
					break;
				}
			}
			
			if(endPos == -1) // Invalid list item. Return empty string
				return "";
				
			string ret = str.substr(0, endPos + 1);
			str = str.substr(endPos + 1);
			
			if(str.length() > 0 && str[0] == ',')
				str.erase(str.begin());
				
			return ret;
		}
		break;
		
		case '"':
		{
			// fprintf(stderr, "Parsing string ('%s')\n", str.c_str());
			int32_t endPos = -1;
			for(int32_t i = 1; i < (int32_t) str.length(); i++)
			{
				if(str[i] == '"')
				{
					if(str[i - 1] == '\\')
						continue;
						
					if(i == (int32_t)(str.length() - 1) || str[i + 1] == ',' || str[i + 1] == '}' || str[i + 1] == ']')
					{
						endPos = i;
						break;
					}
				}
			}
			
			if(endPos == -1) // Invalid list item. Return empty string
				return "";
				
			string ret = str.substr(0, endPos + 1);
			str = str.substr(endPos + 1);
			
			if(ret[0]  == '"')
				ret.erase(ret.begin());
				
			if(*(ret.end() - 1) == '"')
				ret.erase(ret.end() - 1);
				
			if(str.length() > 0 && str[0] == ',')
				str.erase(str.begin());
				
			return ret;
		}
		break;
		
		default:
		{
			// fprintf(stderr, "Parsing default ('%s')\n", str.c_str());
			int32_t endPos = -1;
			for(int32_t i = 0; i < (int32_t) str.length(); i++)
			{
				if(str[i] == ',' || str[i] == '}' || str[i] == ']' || i == (int32_t)(str.length() - 1))
				{
					endPos = i;
					break;
				}
			}
			
			if(endPos == -1) // Should never happen...
				return "";
				
			string ret = str.substr(0, endPos);
			str = str.substr(endPos + 1);
			
			if(str.length() > 0 && str[0] == ',')
				str.erase(str.begin());
				
			return ret;
		}
	}
	
	return "";
}

void GDBMI::parserGetListItems(string &str, ListItemVector &liVector)
{
	while(str.length() > 0)
	{
		ListItem item = parserGetItem(str);
		
		if(item.length() > 0)
			liVector.push_back(item);
	}
}

KVPair GDBMI::parserGetKVPair(string &str)
{
	size_t findEq = str.find_first_of('=');
	
	if(findEq == string::npos)
		return {"", ""};
		
	string key = str.substr(0, findEq);
	str = str.substr(findEq + 1);
	
	if(key[0]  == '"')
		key.erase(key.begin());
		
	if(*(key.end() - 1) == '"')
		key.erase(key.end() - 1);
		
	string val = parserGetItem(str);
	
	return {key, val};
}

void GDBMI::parserGetKVPairs(string &str, KVPairVector &kvpVector)
{
	size_t lastLen = 0;
	while(str.length() > 0 && str.length() != lastLen)
	{
		lastLen = str.length();
		KVPair kvp = parserGetKVPair(str);
		
		if(str.length() != lastLen)
			kvpVector.push_back(kvp);
	}
}

string GDBMI::parserGetTuple(string &str)
{
	int32_t tokenCnt = 1;
	int32_t endPos = -1;
	for(int32_t i = 1; i < (int32_t) str.length(); i++)
	{
		if(str[i] == '{') tokenCnt++;
		if(str[i] == '}') tokenCnt--;
		
		if(tokenCnt <= 0)
		{
			endPos = i;
			break;
		}
	}
	
	if(endPos == -1) // Invalid list item. Return empty string
		return "";
		
	string ret = str.substr(1, endPos - 1);
	str = str.substr(endPos + 1);
	
	if(str.length() > 0 && str[0] == ',')
		str.erase(str.begin());
		
	return ret;
}
