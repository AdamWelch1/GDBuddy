#ifdef BUILD_GDBMI_TESTS
#include "gdbmi.h"

#define CATCH_CONFIG_MAIN
#include "../catch.h"

TEST_CASE("GDBMI response parsers are working correctly", "[parser]")
{
	GDBMI gdb;
	
	SECTION("Can parse tuples")
	{
		string str = "{a simple tuple}";
		string out = gdb.parserGetTuple(str);
		
		REQUIRE(str.length() == 0);
		REQUIRE(out == "a simple tuple");
		
		str = "{{a tuple},{of tuples},{should be},{no problem}}";
		out = gdb.parserGetTuple(str);
		
		REQUIRE(str.length() == 0);
		REQUIRE(out == "{a tuple},{of tuples},{should be},{no problem}");
	}
	
	SECTION("Can parse strings")
	{
		string str = "\"a simple string\"";
		string out = gdb.parserGetItem(str);
		
		REQUIRE(str.length() == 0);
		REQUIRE(out == "a simple string");
		
		str = "\"a string with \\\"an embedded\\\" string within it.\"";
		out = gdb.parserGetItem(str);
		
		REQUIRE(str.length() == 0);
		REQUIRE(out == "a string with \\\"an embedded\\\" string within it.");
	}
	
	SECTION("Can parse lists")
	{
		string str = "item1=\"some stuff\",";
		str += "[a,list,of,things],";
		str += "{and a tuple {too}},",
			   str += "\"and a random string for good measure\"";
			   
		ListItemVector vec;
		gdb.parserGetListItems(str, vec);
		
		REQUIRE(vec.size() == 4);
		REQUIRE(vec[0] == "item1=\"some stuff\"");
		REQUIRE(vec[1] == "[a,list,of,things]");
		REQUIRE(vec[2] == "{and a tuple {too}}");
		REQUIRE(vec[3] == "and a random string for good measure");
	}
	
	SECTION("Can parse key-value pairs")
	{
		string str = "item1=something,"; // I don't think GDB responds with values without quotes, but whatever
		str += "item2=\"something else\",";
		str += "item3=[thing1=\"thingA\",thing2=\"thingB\"],";
		str += "item4={tuple,{tuple},tuple},";
		str += "item5=\"My experience has shown me that correlation=causation is not always true\"";
		
		KVPairVector vec;
		gdb.parserGetKVPairs(str, vec);
		
		REQUIRE(str.length() == 0);
		REQUIRE(vec[0].first == "item1");
		REQUIRE(vec[0].second == "something");
		REQUIRE(vec[1].first == "item2");
		REQUIRE(vec[1].second == "something else");
		REQUIRE(vec[2].first == "item3");
		REQUIRE(vec[2].second == "[thing1=\"thingA\",thing2=\"thingB\"]");
		REQUIRE(vec[3].first == "item4");
		REQUIRE(vec[3].second == "{tuple,{tuple},tuple}");
		REQUIRE(vec[4].first == "item5");
		REQUIRE(vec[4].second == "My experience has shown me that correlation=causation is not always true");
	}
}

#endif
