#include "gdbmi_private.h"

#include "gdbmi.h"
#ifndef SOMETHING_UNIQUE_GDBMI_H
// #include "gdbmi_pipe.h"
#endif

void GDBMI::initPipe()
{
	m_gdbPipeIn[0] = 0;
	m_gdbPipeIn[1] = 0;
	m_gdbPipeOut[0] = 0;
	m_gdbPipeOut[1] = 0;
	m_gdbPID = 0;
	
	runGDB("/home/aj/code/official-gdb/gdb_bin/bin/gdb");
	m_readThreadHandle = thread(GDBMI::readThreadThunk, this);
}

void GDBMI::destroyPipe()
{
	m_readThreadHandle.join();
}

void GDBMI::readThread()
{
	std::string gdbResponseBuf;
	while(!m_exitThreads)
	{
		if(readPipe(gdbResponseBuf))
		{
			while(gdbResponseBuf.length() > 0)
			{
				// Strip any newlines off the beginning of the string
				while(gdbResponseBuf.length() > 0 && gdbResponseBuf[0] == '\n')
					gdbResponseBuf.erase(gdbResponseBuf.begin());
					
				// Search for the next newline in the string
				size_t nlPos = gdbResponseBuf.find_first_of('\n');
				if(nlPos == string::npos)
					break;
					
				// GDB sends MI responses separated by newlines.
				// We want to parse only one command at a time,
				// So we just grab the text up to the first newline.
				
				string respStr = gdbResponseBuf.substr(0, nlPos);
				gdbResponseBuf = gdbResponseBuf.substr(nlPos + 1);
				
				// printf("Raw input: %s\n", respStr.c_str());
				
				// Pass the response string off to the response handlers
				handleResponse(respStr);
			}
			
			usleep(1000 * 10);
		}
		else usleep(1000 * 100);
	}
	
	fprintf(stderr, "readThread() is exiting!\n");
}

bool GDBMI::readPipe(string &out)
{
	char readBuf[4096];
	memset(readBuf, 0, 4096);
	
	bool noDataRead = true;
	int32_t readRes = 0;
	int32_t readFailCount = 0;
	
	std::string tmpStr;
	
	while((readRes = read(m_gdbPipeOut[0], readBuf, 4095)) > 0)
	{
		if(readRes == -1 && errno == EAGAIN)
		{
			if(readFailCount >= 3)
				break;
				
			readFailCount++; // So we don't wait forever
			usleep(1000 * 10);
			continue;
		}
		
		if(readRes == -1 && errno != EAGAIN)
		{
			if(readFailCount >= 3)
				break;
				
			fprintf(stderr, "[%s:%u] read() failed (errno = %d)\n", __FILE__, __LINE__, errno);
			perror("Error");
			
			readFailCount++;
			usleep(1000 * 100);
			continue;
		}
		
		if(readRes > 0)
			noDataRead = false;
			
		readFailCount = 0;
		
		tmpStr.append(readBuf);
		memset(readBuf, 0, 4096);
	}
	
	out.append(tmpStr);
	return !(noDataRead);
}

bool GDBMI::writePipe(string cmd)
{
	const char *sendBuf = cmd.c_str();
	// printf("Raw write: %s\n", sendBuf);
	int32_t sentBytes = 0;
	int32_t writeRes = 0;
	
	errno = 0;
	while((writeRes = write(m_gdbPipeIn[1], sendBuf, (cmd.length() - sentBytes))) > 0)
	{
		sentBytes += writeRes;
		sendBuf += writeRes;
		usleep(1000 * 10);
	}
	
	if(writeRes < 0 && errno != EAGAIN)
	{
		fprintf(stderr, "[%s:%u] write() failed (errno = %d)\n", __FILE__, __LINE__, errno);
		perror("Error");
	}
	
	if(writeRes < 0 && errno == EAGAIN)
		writeRes = 0;
		
	return (writeRes >= 0);
}

bool GDBMI::runGDB(std::string gdbPath)
{

	int pipeRes1 = pipe(m_gdbPipeIn);
	int pipeRes2 = pipe(m_gdbPipeOut);
	
	if(pipeRes1 < 0 || pipeRes2 < 0)
	{
		if(pipeRes1 >= 0)
		{
			close(m_gdbPipeIn[0]);
			close(m_gdbPipeIn[1]);
		}
		
		if(pipeRes2 >= 0)
		{
			close(m_gdbPipeOut[0]);
			close(m_gdbPipeOut[1]);
		}
		
		printf("Failed to allocate pipes for GDB child process\n");
		return false;
	}
	
	pid_t forkRet = fork();
	
	if(forkRet == -1)
	{
		if(pipeRes1 >= 0)
		{
			close(m_gdbPipeIn[0]);
			close(m_gdbPipeIn[1]);
		}
		
		if(pipeRes2 >= 0)
		{
			close(m_gdbPipeOut[0]);
			close(m_gdbPipeOut[1]);
		}
		
		printf("The fork() call failed when trying to launch GDB\n");
		return false;
	}
	
	if(forkRet == 0) // Child process
	{
		dup2(m_gdbPipeIn[0], STDIN_FILENO);
		dup2(m_gdbPipeOut[1], STDOUT_FILENO);
		dup2(m_gdbPipeOut[1], STDERR_FILENO);
		
		close(m_gdbPipeIn[0]);
		close(m_gdbPipeIn[1]);
		close(m_gdbPipeOut[0]);
		close(m_gdbPipeOut[1]);
		
		int32_t execRet = execl(gdbPath.c_str(), gdbPath.c_str(), "--interpreter=mi", "--nx", (char *) 0);
		
		exit(execRet);
	}
	else // Parent process - us :)
	{
		close(m_gdbPipeIn[0]);
		close(m_gdbPipeOut[1]);
		m_gdbPID = forkRet;
		
		int flags[2] = {0};
		flags[0] = fcntl(m_gdbPipeIn[1], F_GETFD);
		flags[1] = fcntl(m_gdbPipeOut[0], F_GETFD);
		
		flags[0] |= FD_CLOEXEC;
		flags[1] |= FD_CLOEXEC;
		
		fcntl(m_gdbPipeIn[1], F_SETFD, flags[0]);
		fcntl(m_gdbPipeOut[0], F_SETFD, flags[1]);
		
		int nbFlags = fcntl(m_gdbPipeOut[0], F_GETFL, 0);
		nbFlags |= O_NONBLOCK;
		fcntl(m_gdbPipeOut[0], F_SETFL, nbFlags);
	}
	
	
	return true;
}
