#ifndef UNIQUE_GDBMI_PARSE_H
#define UNIQUE_GDBMI_PARSE_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{
#endif

		#ifdef BUILD_GDBMI_TESTS
	public:
		#else
	private:
		#endif
	
		enum class ParseItemType : uint8_t
		{
			Invalid,
			String,
			List,
			Tuple
		};
		
		ParseItemType getItemType(char x);
		
		/*
			These functions parse MI response strings from GDB.
			All of these functions consume the parsed portion
			of the input string they are given.
			For example:
		
			If the input is "[some text],[some more text]",
			then the parser would parse "[some text]," leaving
			"[some more text"] remaining in the input string.
			Note the comma has also been removed.
		
		*/
		
		string parserGetItem(string &str);
		void parserGetListItems(string &str, ListItemVector &liVector);
		
		KVPair parserGetKVPair(string &str);
		void parserGetKVPairs(string &str, KVPairVector &kvpVector);
		
		string parserGetTuple(string &str);
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


