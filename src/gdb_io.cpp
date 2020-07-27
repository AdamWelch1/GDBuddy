#include "gdb_io.h"
#include <cstdarg>

#define TERM_DEFAULT	"\x1B[0m"
#define TERM_RED		"\x1B[31m"
#define TERM_GREEN		"\x1B[32m"
#define TERM_YELLOW		"\x1B[33m"
#define TERM_BLUE		"\x1B[34m"
#define TERM_CYAN		"\x1B[36m"
#define TERM_DEFAULT	"\x1B[0m"

#define logPrintf(a, ...) dologPrintf(__FUNCTION__, __LINE__, a,## __VA_ARGS__)

GDBIO::GDBIO(gdb_read_func_t r, gdb_write_func_t w)
{
	m_exitThreads = false;
	m_readFunc = r;
	m_writeFunc = w;
	m_statusStr = "No inferior loaded";
	m_inferiorIsRunning = false;
	m_inferiorExited = true;
	m_verboseOutput = false;
	m_lookupIsPending = false;
	m_gfaRecvBuffer = "";
	
	m_consoleBuf = 0;
	m_consoleUpdated = false;
	m_consoleBufSize = 0;
	m_consoleBufUsed = 0;
	m_consoleExpandFunc = 0;
	
	pthread_mutex_init(&m_pendingCmdMutex, 0);
	pthread_mutex_init(&m_statusMutex, 0);
	pthread_mutex_init(&m_consoleMutex, 0);
	pthread_mutex_init(&m_symbolsMutex, 0);
	pthread_mutex_init(&m_codeViewMutex, 0);
	pthread_mutex_init(&m_signalsMutex, 0);
	pthread_create(&m_readThreadHandle, 0, GDBIO::readThreadThunk, this);
}

GDBIO::~GDBIO()
{

}

void GDBIO::logToConsole(const std::string prefix, std::string msg)
{
	if(m_consoleBuf == 0)
		return;
		
	lockConsole();
	
	uint32_t consoleSpace = m_consoleBufSize - m_consoleBufUsed;
	
	const int32_t maxLineLen = 240;
	int32_t lastBreakChar = -1;
	int32_t charCtr = 0;
	
	for(int32_t i = 0; i < (int32_t) msg.length(); i++) // I have to manually word-wrap for the multi-line edit widget
	{
		if(msg[i] == ' ' || msg[i] == ',' || msg[i] == ':')
			lastBreakChar = i;
			
		if(msg[i] == '\n')
		{
			// fprintf(stderr, "\treset\n");
			charCtr = 0;
			lastBreakChar = -1;
			continue;
		}
		
		charCtr += 1;
		
		// fprintf(stderr, "main loop (%c) ctr = %d\n", msg[i], charCtr);
		if(charCtr >= maxLineLen)
		{
		
			if(lastBreakChar > 0)
			{
				// printf("padding\n");
				fflush(stdout);
				lastBreakChar++;
				msg.insert(msg.begin() + lastBreakChar, '\n');
				i = lastBreakChar;
			}
			else
				msg.insert(msg.begin() + i, '\n');
				
			// fprintf(stderr, "wrapping text\n");
			charCtr = 0;
			lastBreakChar = -1;
		}
	}
	
	while(consoleSpace <= msg.length() + 1)
		m_consoleBufSize = m_consoleExpandFunc();
		
		
	// printf("console update\n");
	memcpy((m_consoleBuf + m_consoleBufUsed), msg.c_str(), msg.length());
	m_consoleBufUsed += msg.length();
	
	if(m_consoleBuf[m_consoleBufUsed - 1] != '\n')
	{
		m_consoleBuf[m_consoleBufUsed] = '\n';
		m_consoleBufUsed++;
	}
	m_consoleBuf[m_consoleBufUsed] = 0; // Null terminate the buffer
	m_consoleUpdated = true;
	
	unlockConsole();
}

void GDBIO::dologPrintf(std::string func, int32_t line, const char *fmt, ...)
{
	bool noNewline = (fmt[strlen(fmt) - 1] != '\n');
	char tmpBuf[1024 * 32 - 1] = {0};
	char tmpFmt[1024 * 16] = {0};
	
	va_list v;
	va_start(v, fmt);
	vsnprintf(tmpBuf, (1024 * 32), fmt, v);
	
	if(line < 0)
		snprintf(tmpFmt, (1024 * 16 - 1), "[%.30s] ", func.c_str());
	else
		snprintf(tmpFmt, (1024 * 16 - 1), "[%.25s:%d] ", func.c_str(), line);
		
	std::string ltcStr = std::string(tmpFmt) + std::string(tmpBuf);
	// if(noNewline) ltcStr += "\n";
	
	logToConsole("a", ltcStr);
	
	memset(tmpFmt, 0, (1024 * 16));
	if(line < 0)
	{
		snprintf(tmpFmt, (1024 * 16 - 1), "%s[%.30s]%s %s",
				 (line == -99 ? TERM_GREEN : TERM_YELLOW),
				 func.c_str(),
				 TERM_DEFAULT,
				 fmt);
	}
	else
	{
		snprintf(tmpFmt, (1024 * 16 - 1), "%s[%.25s:%d]%s %s",
				 TERM_YELLOW,
				 func.c_str(),
				 line,
				 TERM_DEFAULT,
				 fmt);
	}
	
	// if(noNewline) printf("\n");
	
	va_start(v, fmt);
	vprintf(tmpFmt, v);
}

void GDBIO::sendCmd(const std::string &str)
{
	dologPrintf("Send command", -99, "Send: %s", str.c_str());
	m_writeFunc(str);
}

void GDBIO::setStatus(const std::string str)
{
	pthread_mutex_lock(&m_statusMutex);
	m_statusStr = str;
	pthread_mutex_unlock(&m_statusMutex);
}

void *GDBIO::readThread(void *param)
{
	logPrintf("Read thread running\n");
	std::string gdbReadBuf;
	// ReadBuffer gdbReadBuf;
	
	while(true)
	{
		bool readResult = m_readFunc(gdbReadBuf);
		
		// if(!readResult && errno != EAGAIN)
		// break;
		
		if(!readResult)
		{
			usleep(1000 * 10);
			continue;
		}
		
		if(gdbReadBuf.length() > 0)
		{
			// logPrintf("Raw: %s\n", gdbReadBuf.buf);
			std::vector<GDBResponse> respList;
			
			// std::string rawStr = (char *) gdbReadBuf.buf;
			// parseResponse(rawStr, respList);
			parseResponse(gdbReadBuf, respList);
			
			if(respList.size() > 0)
				handleResponses(respList);
		}
		
		usleep(1000 * 50);
	}
	
	logPrintf("Read thread exiting\n");
	return 0;
}

void GDBIO::parseResponse(std::string &response, std::vector<GDBResponse> &out)
{
	size_t findRes;
	while(true)
	{
		// We want to process only one response at a time, so
		// we break the response data up at newlines, which
		// appear at the end of each response.
		findRes = response.find("\n");
		std::string parseStr;
		
		// Perhaps we have a partial read of a response?
		if(findRes == std::string::npos)
		{
			break;
			// parseStr = response;
			// response.clear();
		}
		else
		{
			parseStr = response.substr(0, findRes);
			response = response.substr(findRes + 1);
		}
		
		if(parseStr.length() <= 0)
			break;
			
		/// Now extract any tokens and determine record type
		uint32_t recordIndicator = 0;
		GDBRecordType recordType = GDBRecordType::INVALID;
		
		bool breakLoop = false;
		for(uint32_t i = 0; i < parseStr.length(); i++)
		{
			switch(parseStr[i])
			{
				case '^':
				{
					breakLoop = true;
					recordType = GDBRecordType::Result;
				}
				break;
				
				case '*':
				{
					breakLoop = true;
					recordType = GDBRecordType::ExecAsync;
				}
				break;
				
				case '+':
				{
					breakLoop = true;
					recordType = GDBRecordType::StatusAsync;
				}
				break;
				
				case '=':
				{
					breakLoop = true;
					recordType = GDBRecordType::NotifyAsync;
				}
				break;
				
				case '~':
				{
					breakLoop = true;
					recordType = GDBRecordType::ConsoleStream;
				}
				break;
				
				case '@':
				{
					breakLoop = true;
					recordType = GDBRecordType::TargetStream;
				}
				break;
				
				case '&':
				{
					breakLoop = true;
					recordType = GDBRecordType::LogStream;
				}
				break;
			}
			
			if(breakLoop)
			{
				recordIndicator = i;
				break;
			}
		}
		
		if(recordType == GDBRecordType::INVALID)
			continue;
			
		// Copy the token into responseToken if there is one
		std::string responseToken;
		if(recordIndicator > 0)
		{
			responseToken = parseStr.substr(0, recordIndicator);
			
			// Chop off the token part of the string, plus the record indicator char
			parseStr = parseStr.substr(recordIndicator + 1);
		}
		else
			parseStr = parseStr.substr(1); // Chop off the record indicator char
			
		// Stream record handler
		if(recordType == GDBRecordType::ConsoleStream ||
				recordType == GDBRecordType::TargetStream ||
				recordType == GDBRecordType::LogStream)
		{
		
			GDBResponse tmp;
			tmp.recordToken = responseToken;
			tmp.responseType = recordType;
			tmp.streamText = parseStr;
			
			// logPrintf("Record token: %s; Stream txt: %s\n\n", tmp.recordToken.c_str(), tmp.streamText.c_str());
			out.push_back(tmp);
			
			continue;
		}
		
		// Exec async record handler
		// Result record handler
		if(recordType == GDBRecordType::Result ||
				recordType == GDBRecordType::ExecAsync)
		{
			GDBResponse tmp;
			tmp.recordToken = responseToken;
			tmp.responseType = recordType;
			
			size_t commaPos = parseStr.find(",");
			
			if(commaPos != std::string::npos)
			{
				tmp.recordClass = parseStr.substr(0, commaPos);
				parseStr = parseStr.substr(commaPos + 1);
				tmp.recordData = parseStr;
			}
			else
				tmp.recordClass = parseStr;
				
			// logPrintf("Record token: %s; Class: %s; Data: %s\n\n",
			// 		  tmp.recordToken.c_str(),
			// 		  tmp.recordClass.c_str(),
			// 		  tmp.recordData.c_str());
			
			out.push_back(tmp);
			
			continue;
		}
		
	}
}

void GDBIO::handleResponses(std::vector<GDBResponse> &responseList)
{
	auto parseStreamStr = [&](std::string & str) -> void
	{
		bool done = false;
		uint32_t start = 0;
		while(!done)
		{
			done = true;
			for(uint32_t i = start; i < str.length(); i++)
			{
				if(str[i] == '"')
				{
					done = false;
					str.erase(str.begin() + i);
					start = i;
					break;
				}
				
				if(str[i] != '\\')
					continue;
					
				done = false;
				
				// Backslash at end of string
				if((i + 1) >= str.length())
				{
					str.erase(str.begin() + i);
					start = i;
					break;
				}
				
				switch(str[i + 1])
				{
					// Newline
					case 'n':
					{
						str[i] = '\n';
						str.erase(str.begin() + i + 1);
						start = i + 1;
					}
					break;
					
					// Treat backslashed character as a literal
					default:
					{
						str[i] = str[i + 1];
						str.erase(str.begin() + i + 1);
						start = i + 1;
					}
				}
				
				break;
			}
		}
	};
	for(auto &response : responseList)
	{
		if(response.streamText.length() > 0)
			parseStreamStr(response.streamText);
			
		switch(response.responseType)
		{
			case GDBRecordType::Result:
			{
				if(m_verboseOutput)
				{
					logPrintf("Result response\n");
					logPrintf("Token: %s\n", response.recordToken.c_str());
					logPrintf("Class: %s\n", response.recordClass.c_str());
					logPrintf("Data:\n%s\n\n", response.recordData.c_str());
				}
				
				std::string &recordData = response.recordData;
				std::string &recordToken = response.recordToken;
				if(recordToken.length() > 0 && response.recordClass != "error")
				{
					pthread_mutex_lock(&m_pendingCmdMutex);
					if(m_pendingCommands.find(response.recordToken) != m_pendingCommands.end())
					{
						fflush(stdout);
						GDBCallback cbFunc = m_pendingCommands[response.recordToken];
						m_pendingCommands.erase(response.recordToken);
						pthread_mutex_unlock(&m_pendingCmdMutex);
						
						cbFunc(recordData);
						break;
					}
					
					pthread_mutex_unlock(&m_pendingCmdMutex);
				}
				else if(response.recordClass == "error")
				{
					std::string errMsg = response.recordData;
					KVPair errKvp = getKeyValuePair(errMsg);
					
					std::string message = errKvp.second;
					parseStreamStr(message);
					
					logPrintf("[Result] Error: %s", message.c_str());
				}
				else
				
					logPrintf("Unhandled result record: (%s) %s\n%s\n", response.recordToken.c_str(), response.recordClass.c_str(),
							  response.recordData.c_str());
							  
			}
			break;
			
			case GDBRecordType::ExecAsync:
			{
				GDBResponse &resp = response;
				
				if(m_verboseOutput)
				{
					logPrintf("ExecAsync response\n");
					logPrintf("Token: %s\n", resp.recordToken.c_str());
					logPrintf("Class: %s\n", resp.recordClass.c_str());
					logPrintf("Data:\n%s\n\n", resp.recordData.c_str());
				}
				
				handleAsyncRecord(resp);
			}
			break;
			
			case GDBRecordType::StatusAsync:
			{
				logPrintf("Unsupported record\n");
				logPrintf("recordData: \n%s\n\n", response.recordData.c_str());
			}
			break;
			
			case GDBRecordType::NotifyAsync:
			{
				logPrintf("Unsupported record\n");
				logPrintf("recordData: \n%s\n\n", response.recordData.c_str());
			}
			break;
			
			case GDBRecordType::ConsoleStream:
			{
				// logPrintf("Console: %s", response.streamText.c_str());
				// logToConsole("Console: %s", response.streamText.c_str());
			}
			break;
			
			case GDBRecordType::TargetStream:
			{
				logPrintf("Target: %s", response.streamText.c_str());
			}
			break;
			
			case GDBRecordType::LogStream:
			{
				logPrintf("Log: %s", response.streamText.c_str());
			}
			break;
			
			case GDBRecordType::INVALID:
			default:
			{
				logPrintf("Unsupported record\n");
				logPrintf("recordData: \n%s\n\n", response.recordData.c_str());
			}
		}
	}
}

std::vector<std::string> GDBIO::parseList(std::string &listStr)
{
	std::vector<std::string> ret;
	
	if(listStr[0] != '[' || listStr[listStr.length() - 1] != ']')
	{
		logPrintf("GDBIO::parseList(): Invalid parameter\n");
		return ret;
	}
	
	listStr = listStr.substr(1);
	if(listStr[0] != '{')
	{
		logPrintf("GDBIO::parseList(): List is not a list of tuples. Unsupported\n");
		return ret;
	}
	
	while(true)
	{
		int32_t depth = 1;
		uint32_t stopIndex = 0;
		for(uint32_t i = 1; i < listStr.length(); i++)
		{
			if(listStr[i] == '{') depth++;
			if(listStr[i] == '}') depth--;
			
			if(depth == 0)
			{
				stopIndex = i;
				break;
			}
		}
		
		if(depth != 0) // We read an invalid tuple
			break;
			
		std::string listItem = listStr.substr(0, stopIndex + 1);
		ret.push_back(listItem);
		
		if(listStr[stopIndex + 1] == ']') // End of list
			break;
			
		// We have another item to read
		if((stopIndex + 2) < listStr.length() && listStr[stopIndex + 1] == ',' && listStr[stopIndex + 2] == '{')
		{
			listStr = listStr.substr(stopIndex + 2);
			continue;
		}
		
		// Either the list ended abruptly, or the next list item isn't a tuple
		break;
	}
	
	return ret;
}

KVPair GDBIO::getKeyValuePair(std::string &str)
{
	uint8_t valChar = 0;
	size_t findEq = str.find("=");
	
	if(findEq == std::string::npos)
		return {"11", "11"};
		
	std::string retKey = str.substr(0, findEq);
	
	if((findEq + 1) >= str.length() || str[findEq + 1] == ',')
		return {retKey, "22"};
		
	if(str[findEq + 1] == '"') valChar = '"';
	if(str[findEq + 1] == '{') valChar = '}';
	if(str[findEq + 1] == '[') valChar = ']';
	
	if(valChar == 0)
	{
		size_t commaPos = str.find(",");
		if(commaPos != std::string::npos)
		{
			std::string retVal = str.substr((findEq + 1), (commaPos - findEq - 1));
			str = str.substr(commaPos + 1);
			
			return {retKey, retVal};
		}
		
		std::string retVal = str.substr(findEq + 1);
		return {retKey, retVal};
	}
	
	int32_t count = 1;
	uint32_t stopIndex = 0;
	for(uint32_t i = findEq + 2; i < str.length(); i++)
	{
		if(valChar == '"')
		{
			char nxt = str[i + 1];
			// if(str[i] == str[findEq + 1] && nxt != ',' && nxt != ']' && ) count++;
			if(str[i] == valChar && (nxt == ',' || nxt == ']' || nxt == '}')) count--;
			else if(str[i] == valChar) count++;
		}
		else
		{
			if(str[i] == str[findEq + 1]) count++;
			if(str[i] == valChar) count--;
		}
		
		if(count == 0)
		{
			stopIndex = i;
			break;
		}
	}
	
	if(stopIndex == 0 && valChar == '"' && *(str.end() - 1) == '"')
		stopIndex = str.length() - 1;
		
	if(stopIndex == 0) // We didn't find the end of the string/tuple/list
		return {"33", "33"};
		
	std::string retVal = str.substr((findEq + 1), (stopIndex - findEq));
	
	if((stopIndex + 2) < str.length() && str[stopIndex + 1] == ',')
		str = str.substr(stopIndex + 2);
	else
		str = str.substr(stopIndex + 1);
		
	return {retKey, retVal};
}

std::string GDBIO::getTuple(std::string &str)
{
	if(str[0] != '{') // Not a tuple
		return "";
		
	int32_t count = 1;
	uint32_t stopIndex = 0;
	for(uint32_t i = 1; i < str.length(); i++)
	{
		if(str[i] == '{') count++;
		if(str[i] == '}') count--;
		
		if(count == 0)
		{
			stopIndex = i;
			break;
		}
	}
	
	if(stopIndex == 0) // Tuple had no ending
		return "";
		
	std::string ret = str.substr(1, stopIndex - 1);
	str = str.substr(stopIndex + 1);
	
	return ret;
}

std::string GDBIO::getRandomToken()
{
	FILE *randFD = fopen("/dev/urandom", "rb");
	uint32_t randNum = 0;
	fread((char *) &randNum, 1, 4, randFD);
	fclose(randFD);
	
	char numOut[256] = {0};
	sprintf(numOut, "%u", randNum);
	
	return std::string(numOut);
}

void GDBIO::loadInferior(std::string inferiorPath)
{
	m_inferiorPath = inferiorPath;
	size_t tmp = inferiorPath.find_last_of("/");
	m_projectDir = inferiorPath.substr(0, tmp);
	logPrintf("Inferior: %s\nProject dir: %s\n", m_inferiorPath.c_str(), m_projectDir.c_str());
	
	// if(!createInferiorProcess())
	// {
	// 	logPrintf("Failed to create inferior process!\n");
	// 	return;
	// }
	
	char tmpPid[64] = {0};
	sprintf(tmpPid, "%d", m_inferiorPID);
	std::string cmdToken = getRandomToken();
	std::string cmdOut = cmdToken + "-file-exec-and-symbols ";
	// std::string cmdOut = cmdToken + "-target-attach ";
	// cmdOut += tmpPid;
	// cmdOut += "\n";
	cmdOut += inferiorPath + "\n";
	sendCmd(cmdOut);
	
	auto handleResponse = [&](std::string & response) -> void
	{
		setStatus("Inferior loaded");
		logPrintf("Inferior loaded\n\n");
		fflush(stdout);
		
		usleep(1000 * 10);
		loadFunctionSymbols();
		loadGlobalVarSymbols();
		
		// sendCmd("-data-evaluate-expression close(0)\n");
		// sendCmd("-data-evaluate-expression close(1)\n");
		// sendCmd("-data-evaluate-expression close(2)\n");
		// disassembleFunc("main");
	};
	
	pthread_mutex_lock(&m_pendingCmdMutex);
	m_pendingCommands[cmdToken] = handleResponse;
	pthread_mutex_unlock(&m_pendingCmdMutex);
}

void GDBIO::loadFunctionSymbols()
{
	setStatus("Loading function symbols...");
	
	std::string cmdToken = getRandomToken();
	std::string cmdOut = cmdToken + "-symbol-info-functions\n";
	m_functionSymbols.clear();
	sendCmd(cmdOut);
	
	auto handleResponse = [&](std::string & response) -> void
	{
		lockSymbols();
		
		KVPair kvp = getKeyValuePair(response);
		if(kvp.first == "symbols" && kvp.second.length() > 0)
		{
			if(kvp.second[0] == '{')
			{
				std::string	tupleVal = getTuple(kvp.second);
				KVPair kvp2 = getKeyValuePair(tupleVal);
				
				if(kvp2.first == "debug" && kvp2.second.length() > 0)
				{
					std::vector<std::string> listItems = parseList(kvp2.second);
					
					if(listItems.size() > 0)
					{
						// logPrintf("Items:\n");
						
						for(auto &iter : listItems)
						{
							FileSymbolSet symSet;
							
							std::string itemTuple = getTuple(iter);
							while(itemTuple.length() > 0)
							{
								KVPair itemPair = getKeyValuePair(itemTuple);
								{
									if(itemPair.second[0] != '[')
									{
										if(itemPair.first == "filename")
										{
											symSet.shortName = itemPair.second;
											symSet.shortName.erase(symSet.shortName.begin());
											symSet.shortName.erase(symSet.shortName.end() - 1);
										}
										
										if(itemPair.first == "fullname")
										{
											symSet.fullName = itemPair.second;
											symSet.fullName.erase(symSet.fullName.begin());
											symSet.fullName.erase(symSet.fullName.end() - 1);
										}
										// logPrintf("\t\tKey = %s; Val = %s\n", itemPair.first.c_str(), itemPair.second.c_str());
									}
									else
									{
									
										// logPrintf("\t\tKey = %s:\n", itemPair.first.c_str());
										std::vector<std::string> subList = parseList(itemPair.second);
										
										for(auto &iter2 : subList)
										{
											std::string subItem = getTuple(iter2);
											int cnt = 0;
											
											std::string lineNum;
											std::string descr;
											
											while(subItem.length() > 1)
											{
												KVPair slKvp = getKeyValuePair(subItem);
												// logPrintf("subitem = '%s'\n", subItem.c_str());
												
												if(slKvp.first == "line")
												{
													lineNum = slKvp.second;
													// Delete the double-quotes wrapping the value
													lineNum.erase(lineNum.begin());
													lineNum.erase(lineNum.end() - 1);
												}
												
												if(slKvp.first == "description")
												{
													descr = slKvp.second;
													// Delete the double-quotes wrapping the value
													descr.erase(descr.begin());
													descr.erase(descr.end() - 1);
													
													symSet.symbolList.push_back({lineNum, descr});
												}
												
												// logPrintf("\t\t\t%s = %s\n", slKvp.first.c_str(), slKvp.second.c_str());
												
												if(cnt++ > 4)
													break;
											}
										}
									}
									
								}
							}
							
							m_functionSymbols.push_back(symSet);
						}
						
						
					}
				}
			}
		}
		
		unlockSymbols();
		setStatus("Function symbols loaded");
	};
	
	pthread_mutex_lock(&m_pendingCmdMutex);
	m_pendingCommands[cmdToken] = handleResponse;
	pthread_mutex_unlock(&m_pendingCmdMutex);
}

void GDBIO::loadGlobalVarSymbols()
{
	setStatus("Loading global variable symbols\n");
	
	std::string cmdToken = getRandomToken();
	std::string cmdOut = cmdToken + "-symbol-info-variables\n";
	sendCmd(cmdOut);
	m_globalVarSymbols.clear();
	
	auto handleResponse = [&](std::string & response) -> void
	{
		lockSymbols();
		
		KVPair kvp = getKeyValuePair(response);
		if(kvp.first == "symbols" && kvp.second.length() > 0)
		{
			if(kvp.second[0] == '{')
			{
				std::string	tupleVal = getTuple(kvp.second);
				KVPair kvp2 = getKeyValuePair(tupleVal);
				
				if(kvp2.first == "debug" && kvp2.second.length() > 0)
				{
					std::vector<std::string> listItems = parseList(kvp2.second);
					
					if(listItems.size() > 0)
					{
						// logPrintf("Items:\n");
						
						for(auto &iter : listItems)
						{
							FileSymbolSet symSet;
							
							std::string itemTuple = getTuple(iter);
							while(itemTuple.length() > 0)
							{
								KVPair itemPair = getKeyValuePair(itemTuple);
								{
									if(itemPair.second[0] != '[')
									{
										if(itemPair.first == "filename")
										{
											symSet.shortName = itemPair.second;
											symSet.shortName.erase(symSet.shortName.begin());
											symSet.shortName.erase(symSet.shortName.end() - 1);
										}
										
										if(itemPair.first == "fullname")
										{
											symSet.fullName = itemPair.second;
											symSet.fullName.erase(symSet.fullName.begin());
											symSet.fullName.erase(symSet.fullName.end() - 1);
										}
										// logPrintf("\t\tKey = %s; Val = %s\n", itemPair.first.c_str(), itemPair.second.c_str());
									}
									else
									{
									
										// logPrintf("\t\tKey = %s:\n", itemPair.first.c_str());
										std::vector<std::string> subList = parseList(itemPair.second);
										
										for(auto &iter2 : subList)
										{
											std::string subItem = getTuple(iter2);
											int cnt = 0;
											
											std::string lineNum;
											std::string descr;
											
											while(subItem.length() > 1)
											{
												KVPair slKvp = getKeyValuePair(subItem);
												// logPrintf("subitem = '%s'\n", subItem.c_str());
												
												if(slKvp.first == "line")
												{
													lineNum = slKvp.second;
													// Delete the double-quotes wrapping the value
													lineNum.erase(lineNum.begin());
													lineNum.erase(lineNum.end() - 1);
												}
												
												if(slKvp.first == "description")
												{
													descr = slKvp.second;
													// Delete the double-quotes wrapping the value
													descr.erase(descr.begin());
													descr.erase(descr.end() - 1);
													
													symSet.symbolList.push_back({lineNum, descr});
												}
												
												// logPrintf("\t\t\t%s = %s\n", slKvp.first.c_str(), slKvp.second.c_str());
												
												if(cnt++ > 4)
													break;
											}
										}
									}
									
								}
							}
							
							m_globalVarSymbols.push_back(symSet);
						}
						
						
					}
				}
			}
		}
		
		unlockSymbols();
		setStatus("Inferior loaded; Not running");
	};
	
	pthread_mutex_lock(&m_pendingCmdMutex);
	m_pendingCommands[cmdToken] = handleResponse;
	pthread_mutex_unlock(&m_pendingCmdMutex);
}

void GDBIO::disassembleFunc(std::string location, bool noFlags)
{
	std::string cmdToken = getRandomToken();
	
	std::string cmdOut = cmdToken + "-data-disassemble -a ";
	if(noFlags)
		cmdOut = cmdToken + "-data-disassemble ";
	cmdOut += location + " 0\n";
	sendCmd(cmdOut);
	
	auto handleResponse = [&](std::string & response) -> void
	{
		KVPair rootObject = getKeyValuePair(response);
		
		if(rootObject.first != "asm_insns")
			return;
			
		std::vector<std::string> instructionList = parseList(rootObject.second);
		
		lockCodeView();
		
		if(instructionList.size() > 0)
			m_disassemblyLines.clear();
			
		for(auto &instrSpec : instructionList)
		{
			std::string line = getTuple(instrSpec);
			DisasLineInfo lineInfo;
			
			while(line.length() > 1)
			{
				KVPair kvp = getKeyValuePair(line);
				kvp.second.erase(kvp.second.begin());
				kvp.second.erase(kvp.second.end() - 1);
				
				if(kvp.first == "address")
					lineInfo.addr = kvp.second;
					
				if(kvp.first == "func-name")
					lineInfo.funcName = kvp.second;
					
				if(kvp.first == "offset")
					lineInfo.offset = kvp.second;
					
				if(kvp.first == "inst")
					lineInfo.instr = kvp.second;
			}
			
			m_disassemblyLines.push_back(lineInfo);
		}
		unlockCodeView();
	};
	
	pthread_mutex_lock(&m_pendingCmdMutex);
	m_pendingCommands[cmdToken] = handleResponse;
	pthread_mutex_unlock(&m_pendingCmdMutex);
}

void GDBIO::runInferior()
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-run\n");
	sendCmd(tmpCmd);
}

void GDBIO::contInferior()
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-continue\n");
	sendCmd(tmpCmd);
}

void GDBIO::pauseInferior()
{
	logPrintf("Pause inferior\n");
	std::string tmpCmd = getRandomToken() + std::string("-exec-interrupt\n");
	sendCmd(tmpCmd);
}

void GDBIO::execFinInferior() // Execute until return from current stack frame
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-finish\n");
	sendCmd(tmpCmd);
}

void GDBIO::stepOverInferior() // Execute until next source line, don't follow calls
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-next\n");
	sendCmd(tmpCmd);
}

void GDBIO::stepOverInstInferior() // Execute next instruction, don't follow
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-next-instruction\n");
	sendCmd(tmpCmd);
}

void GDBIO::execRetInferior() // Return immediately from current stack frame
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-return\n");
	sendCmd(tmpCmd);
}

void GDBIO::stepInferior() // Execute until next source line, follow calls
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-step\n");
	sendCmd(tmpCmd);
}

void GDBIO::stepInstInferior() // Execute to next source line, don't follow
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-step-instruction\n");
	sendCmd(tmpCmd);
}

void GDBIO::execUntilInferior() // Execute until [location]
{
	std::string tmpCmd = getRandomToken() + std::string("-exec-until\n");
	sendCmd(tmpCmd);
}

void GDBIO::handleAsyncRecord(GDBResponse &record)
{
	auto unwrapQuotes = [](std::string & str)
	{
		if(str[0] == '"' && str[str.length() - 1] == '"')
		{
			str.erase(str.begin());
			str.erase(str.end() - 1);
		}
	};
	
	if(record.recordClass == "running")
	{
		setStatus("Inferior is running.");
		
		if(m_inferiorExited)
			logPrintf("Executing inferior\n");
			
		m_inferiorExited = false;
		m_inferiorIsRunning = true;
		return;
	}
	
	if(record.recordClass == "stopped")
	{
		setStatus("Inferior stopped.");
		m_inferiorIsRunning = false;
		
		// std::string addrFunc;
		getFunctionAtAddress("main");
		
		std::string recordData = record.recordData;
		KVPair rootPair = getKeyValuePair(recordData);
		
		// if(!m_lookupIsPending)
		// disassembleFunc(addrFunc);
		
		if(rootPair.first == "reason")
		{
			std::string reasonVal = rootPair.second;
			unwrapQuotes(reasonVal);
			
			if(reasonVal == "signal-received")
			{
				//
				SignalInfo sig = parseSignal(recordData);
				
				logPrintf("Caught signal: [%s] '%s' in thread %s\n",
						  sig.signalName.c_str(),
						  sig.signalMeaning.c_str(),
						  sig.signalThread.c_str());
				logPrintf("Stack frame: Function '%s' in file '%s', Line %s\n\n",
						  sig.signalFrame.frameFunc.c_str(),
						  sig.signalFrame.frameShortname.c_str(),
						  sig.signalFrame.frameLine.c_str());
						  
				char status[512] = {0};
				sprintf(status, "Inferior Stopped; Signal: %s (%s)",
						sig.signalName.c_str(),
						sig.signalMeaning.c_str());
				setStatus(status);
				
				pthread_mutex_lock(&m_signalsMutex);
				m_signalList.push_back(sig);
				
				while(m_signalList.size() > 1024)
					m_signalList.erase(m_signalList.begin());
				pthread_mutex_unlock(&m_signalsMutex);
			}
			else if(reasonVal == "exited-signalled")
			{
				SignalInfo sig = parseSignal(recordData);
				char status[512] = {0};
				sprintf(status, "Inferior Exited; Signal: %s (%s)", sig.signalName.c_str(), sig.signalMeaning.c_str());
				m_inferiorExited = true;
				setStatus(status);
			}
			else
			{
				//
				logPrintf("Unhandled AsyncStop:\n\tReason: %s\n\tRecord data: %s\n\n", reasonVal.c_str(), recordData.c_str());
			}
		}
	}
}

SignalInfo GDBIO::parseSignal(std::string &signalData)
{
	auto unwrapQuotes = [](std::string & str)
	{
		if(str[0] == '"' && str[str.length() - 1] == '"')
		{
			str.erase(str.begin());
			str.erase(str.end() - 1);
		}
	};
	
	SignalInfo sig;
	while(signalData.length() > 1)
	{
		size_t preLength = signalData.length();
		KVPair kvp = getKeyValuePair(signalData);
		// logPrintf("%s = %s\n", kvp.first.c_str(), kvp.second.c_str());
		
		// If signalData isn't getting any shorter, we'll loop forever.
		// Basically, we have something we don't know how to parse
		if(signalData.length() == preLength)
			break;
			
		// Empty key or value (or both). Skip it
		if(kvp.first.length() == 0 || kvp.second.length() == 0)
			continue;
			
		std::string key = kvp.first;
		std::string val = kvp.second;
		unwrapQuotes(key);
		unwrapQuotes(val);
		
		if(key == "signal-name")
		{
			sig.signalName = val;
			continue;
		}
		
		if(key == "signal-meaning")
		{
			sig.signalMeaning = val;
			continue;
		}
		
		if(key == "frame")
		{
			parseFrameData(val, sig.signalFrame);
			continue;
		}
		
		if(key == "thread-id")
		{
			sig.signalThread = val;
			continue;
		}
		
		if(key == "stopped-threads")
		{
			sig.signalThreadStop = val;
			continue;
		}
		
		if(key == "core")
		{
			sig.signalCpuCore = val;
			continue;
		}
	}
	
	return sig;
}

void GDBIO::parseFrameData(std::string frameTuple, FrameInfo &frame)
{
	auto unwrapQuotes = [](std::string & str)
	{
		if(str[0] == '"' && str[str.length() - 1] == '"')
		{
			str.erase(str.begin());
			str.erase(str.end() - 1);
		}
	};
	
	std::string tup = getTuple(frameTuple);
	FrameInfo &fInfo = frame;
	while(tup.length() > 1)
	{
		KVPair kvp = getKeyValuePair(tup);
		
		std::string key = kvp.first;
		std::string val = kvp.second;
		unwrapQuotes(key);
		unwrapQuotes(val);
		
		// logPrintf("[frame] %s = %s\n", key.c_str(), val.c_str());
		
		if(key == "addr")
		{
			fInfo.frameAddr = val;
			continue;
		}
		
		if(key == "func")
		{
			fInfo.frameFunc = val;
			continue;
		}
		
		if(key == "args")
		{
			std::vector<std::string> argList = parseList(val);
			
			for(auto iter : argList)
			{
				KVPair tmp = getKeyValuePair(iter);
				fInfo.frameArgs.push_back(tmp);
			}
			
			continue;
		}
		
		if(key == "file")
		{
			fInfo.frameShortname = val;
			continue;
		}
		
		if(key == "fullname")
		{
			fInfo.frameFullname = val;
			continue;
		}
		
		if(key == "line")
		{
			fInfo.frameLine = val;
			continue;
		}
		
		if(key == "arch")
		{
			fInfo.frameArch = val;
			continue;
		}
	}
}

void GDBIO::getFunctionAtAddress(std::string location)
{
	std::string cmdToken = getRandomToken();
	std::string cmdOut = cmdToken + "-data-evaluate-expression ";
	cmdOut += location + "\n";
	
	m_lookupIsPending = true;
	
	auto handleResponse = [&](std::string & response) -> void
	{
		/*
			[Send command] Send: -data-evaluate-expression $pc
			[handleResponses:471] Unhandled result record: () done
			value="0x55555555588b <processRowFixed(unsigned char*, unsigned int, unsigned int)+59>"
		*/
		
		logPrintf("data-evaluate-expression callback\n");
		
		std::string responseData = response;
		KVPair kvp = getKeyValuePair(responseData);
		
		if(kvp.first == "value" && kvp.second.length() > 8)
		{
			printf("Value = %s\n", kvp.second.c_str());
			std::string funcSig = kvp.second;
			
			uint32_t startIndex = 0;
			for(uint32_t i = 0; i < funcSig.length(); i++)
			{
				if(funcSig[i] == '<')
				{
					startIndex = i;
					break;
				}
			}
			
			uint32_t stopIndex = 0;
			for(uint32_t i = funcSig.length() - 1; i > startIndex; i--)
			{
				if(i == ')')
				{
					stopIndex = i;
					break;
				}
				
				if(i == 0)
					break;
			}
			
			uint32_t len = (stopIndex - startIndex);
			std::string ret;
			
			if(len > 0)
				ret = funcSig.substr(startIndex + 1, len);
				
			m_gfaRecvBuffer = ret;
			
			lockSymbols();
			bool clearRemaining = false;
			for(auto &iter : m_functionSymbols)
			{
				if(clearRemaining)
				{
					FileSymbolSet &fss = iter;
					fss.activeFunctionIndex = -1;
					continue;
				}
				
				int32_t indexVal = 0;
				FileSymbolSet &fss = iter;
				fss.activeFunctionIndex = -1;
				
				for(auto &funcs : fss.symbolList)
				{
					if(funcs.second.find(ret) != std::string::npos)
					{
						logPrintf("Found active item\n");
						fss.activeFunctionIndex = indexVal;
						break;
					}
					
					indexVal++;
				}
				
				if(fss.activeFunctionIndex != -1)
					clearRemaining = true;
			}
			unlockSymbols();
			
			disassembleFunc(ret);
		}
		
		m_lookupIsPending = false;
	};
	
	pthread_mutex_lock(&m_pendingCmdMutex);
	m_pendingCommands[cmdToken] = handleResponse;
	pthread_mutex_unlock(&m_pendingCmdMutex);
	
	sendCmd(cmdOut);
}
