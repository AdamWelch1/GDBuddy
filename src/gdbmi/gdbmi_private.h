#ifndef GDBMI_PRIVATE_H
#define GDBMI_PRIVATE_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <vector>
#include <deque>
#include <map>
#include <string>
#include <functional>
#include <thread>
#include <mutex>

#include <unistd.h>
#include <fcntl.h>

#define GDB_HANDLER_THREAD_COUNT	32

using std::string;
using std::vector;
using std::pair;
using std::deque;
using std::thread;
using std::mutex;

#ifndef GDBMI_TYPES
typedef string 					ListItem;
typedef vector<ListItem> 		ListItemVector;
typedef pair<string, string> 	KVPair;
typedef vector<KVPair>			KVPairVector;
#define GDBMI_TYPES
#endif

#endif
