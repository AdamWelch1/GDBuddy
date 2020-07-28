#include "gdbmi_private.h"
#include "gdbmi.h"

void GDBMI::initState()
{
	m_gdbState = GDBState::Stopped;
	m_stopMsg = "No inferior loaded";
}

void GDBMI::destroyState()
{

}

void GDBMI::setState(GDBState state, const string msg)
{
	m_stateMutex.lock();
	m_gdbState = state;
	m_stopMsg = msg;
	m_stateMutex.unlock();
}

GDBMI::GDBState GDBMI::getState()
{
	GDBState ret;
	m_stateMutex.lock();
	ret = m_gdbState;
	m_stateMutex.unlock();
	
	return ret;
}

string GDBMI::getStatusMsg()
{
	string ret;
	m_stateMutex.lock();
	ret = m_stopMsg;
	m_stateMutex.unlock();
	
	return ret;
}
