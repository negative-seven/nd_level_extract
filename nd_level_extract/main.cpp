#include <map>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pugixml.hpp>

#include "handle_memory.h"

#define ATTR_HARDCODED (1 << 16) // Use given value, don't read from memory
#define ATTR_READBYTE (1 << 17) // Read 1 byte from memory
#define ATTR_READUSTR (1 << 18) // Read Unicode string, using additional offset (length of string is at 0x4, string begins at 0x8)
#define ATTR_NOITEM (1 << 19) // Set to "no_item"
#define ATTR_BOOL (1 << 20) // Set to 1 if greater than 1
#define ATTR_READFLOAT (1 << 21) // Read value as float

struct Obj
{
public:
	unsigned int pointer;
	std::map<std::string, std::string> attributes;
};

struct ObjType
{
public:
	std::string name_singular;
	std::string name_plural;
	std::map<std::string, unsigned int> attributes;
	std::vector<Obj *> objList;
	std::vector<unsigned int> offsets_firstObj;
};

int main(int argc, char *argv[])
{
	// Get arguments

	int output = 0;
	std::string filepath = "LEVEL.xml";
	int character = -1;
	std::string dungeonName = "LEVEL";
	int music = 0;

	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];

		if (arg == "-o" || arg == "--output")
		{
			if (i < argc - 1)
			{
				output = std::stoi(argv[++i]);
			}
		}

		if (arg == "-f" || arg == "--filepath")
		{
			if (i < argc - 1)
			{
				filepath = argv[++i];
			}
		}
		if (arg == "-c" || arg == "--character")
		{
			if (i < argc - 1)
			{
				character = std::stoi(argv[++i]);
			}
		}
		if (arg == "-n" || arg == "--dungeonname")
		{
			if (i < argc - 1)
			{
				dungeonName = argv[++i];
			}
		}
		if (arg == "-m" || arg == "--music")
		{
			if (i < argc - 1)
			{
				music = std::stoi(argv[++i]);
			}
		}
	}

	// Initialize log file stream

	std::ofstream logfile("log.txt");

	// Open handle for the game

	HANDLE handle_process;

	try
	{
		handle_process = openProcess("Crypt of the NecroDancer");
	}
	catch (std::exception e)
	{
		logfile << "Could not find game window.";
		return 1;
	}

	unsigned int address_base = getBaseAddress(handle_process, "NecroDancer.exe");

	// Create all instances of objType

	std::map<std::string, ObjType *> objTypeList;

	{
		ObjType *objType_tiles = new ObjType();
		objType_tiles->name_singular = "tile";
		objType_tiles->name_plural = "tiles";
		objType_tiles->attributes = {
			{"cracked", ATTR_READBYTE | 0x60},
			{"torch", ATTR_BOOL | 0x64},
			{"type", 0x58},
			{"x", 0x14},
			{"y", 0x18},
			{"zone", 0x5C}
		};
		objType_tiles->offsets_firstObj = {0x435BEC, 0x10, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("tiles", objType_tiles));
	}

	{
		ObjType *objType_traps = new ObjType();
		objType_traps->name_singular = "trap";
		objType_traps->name_plural = "traps";
		objType_traps->attributes = {
			{"subtype", ATTR_HARDCODED | 0}, // Special case
			{"type", 0xF4},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_traps->offsets_firstObj = {0x43597C, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("traps", objType_traps));
	}

	{
		ObjType *objType_enemies = new ObjType();
		objType_enemies->name_singular = "enemy";
		objType_enemies->name_plural = "enemies";
		objType_enemies->attributes = {
			{"beatDelay", 0x114},
			{"lord", ATTR_READBYTE | 0x118},
			{"type", 0x110},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_enemies->offsets_firstObj = {0x4359E0, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("enemies", objType_enemies));
	}

	{
		ObjType *objType_items = new ObjType();
		objType_items->name_singular = "item";
		objType_items->name_plural = "items";
		objType_items->attributes = {
			{"bloodCost", ATTR_READFLOAT | 0x148},
			{"saleCost", 0x100},
			{"singleChoice", ATTR_READBYTE | 0xF8},
			{"type", ATTR_READUSTR | 0xF4},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_items->offsets_firstObj = {0x435978, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("items", objType_items));
	}

	{
		ObjType *objType_chests = new ObjType();
		objType_chests->name_singular = "chest";
		objType_chests->name_plural = "chests";
		objType_chests->attributes = {
			{"color", 0xF4},
			{"contents", ATTR_READUSTR | 0xF8},
			{"hidden", ATTR_READBYTE | 0x9C},
			{"saleCost", 0x114}, // Buggy
			{"singleChoice", ATTR_READBYTE | 0xFC},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_chests->offsets_firstObj = {0x435938, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("chests", objType_chests));
	}

	{
		ObjType *objType_crates = new ObjType();
		objType_crates->name_singular = "crate";
		objType_crates->name_plural = "crates";
		objType_crates->attributes = {
			{"contents", ATTR_READUSTR | 0x238},
			{"type", 0x234},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_crates->offsets_firstObj = {0x4356AC, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("crates", objType_crates));
	}

	{
		ObjType *objType_shrines = new ObjType();
		objType_shrines->name_singular = "shrine";
		objType_shrines->name_plural = "shrines";
		objType_shrines->attributes = {
			{"type", 0xF4},
			{"x", 0x14},
			{"y", 0x18}
		};
		objType_shrines->offsets_firstObj = {0x4356F8, 0x10, 0x10};
		objTypeList.insert(std::make_pair<>("shrines", objType_shrines));
	}

	// Get all objects of each type (except tiles)

	for (auto const& el : objTypeList)
	{
		if (el.first == "tiles") // Skip tiles
		{
			continue;
		}

		ObjType *objType = el.second;

		int temp = address_base;

		for each (unsigned int offset in objType->offsets_firstObj)
		{
			temp = readMemoryInt(handle_process, temp + offset);
		}

		while (true)
		{
			int temp2 = readMemoryInt(handle_process, temp + 0x18);

			if (!temp2)
			{
				break;
			}

			Obj *obj = new Obj();
			obj->pointer = temp2;
			objType->objList.push_back(obj);

			temp = readMemoryInt(handle_process, temp + 0x10);
		}

		logfile << "Found " << objType->objList.size() << " " << objType->name_plural.c_str() << '\n';
	}

	// Get all tile objects

	int temp = address_base;

	for each (unsigned int offset in objTypeList["tiles"]->offsets_firstObj)
	{
		temp = readMemoryInt(handle_process, temp + offset);
	}

	while (true)
	{
		int temp2 = readMemoryInt(handle_process, temp + 0x18);
		temp = readMemoryInt(handle_process, temp + 0x10);

		if (!temp2)
		{
			break;
		}

		bool isTile = true;

		for each (auto const& el in objTypeList)
		{
			for each (Obj *obj in el.second->objList)
			{
				if (obj->pointer == temp2)
				{
					isTile = false;
					break;
				}
			}

			if (!isTile)
			{
				break;
			}
		}

		if (isTile)
		{
			Obj *obj = new Obj();
			obj->pointer = temp2;
			objTypeList["tiles"]->objList.push_back(obj);
		}
	}

	logfile << "Found " << objTypeList["tiles"]->objList.size() << " " << objTypeList["tiles"]->name_plural.c_str() << '\n';

	objTypeList.insert(std::make_pair<>("tiles", objTypeList["tiles"]));

	// Get attributes of each object

	for each (auto const& el in objTypeList)
	{
		for each (Obj *obj in el.second->objList)
		{
			for (auto const& p : el.second->attributes)
			{
				std::string value;

				if (p.second & ATTR_HARDCODED)
				{
					value = std::to_string(LOWORD(p.second));
				}
				else if (p.second & ATTR_NOITEM)
				{
					value = "no_item";
				}
				else
				{
					int temp_int = readMemoryInt(handle_process, obj->pointer + LOWORD(p.second));

					if (p.second & ATTR_READFLOAT)
					{
						float temp_float = readMemoryFloat(handle_process, obj->pointer + LOWORD(p.second));

						value = std::to_string(temp_float);

						// Remove trailing zeros
						value.erase(value.find_last_not_of('0') + 1, std::string::npos);
						if (value.back() == '.')
						{
							value.erase(value.end() - 1);
						}
					}

					else if (p.second & ATTR_READUSTR)
					{
						int strBase = readMemoryInt(handle_process, obj->pointer + LOWORD(p.second));
						int strLen = readMemoryInt(handle_process, strBase + 0x4);
						std::wstring str = readMemoryUnicodeString(handle_process, strBase + 0x8, strLen);
						value = std::string(str.begin(), str.end());
					}

					else
					{
						if (p.second & ATTR_READBYTE)
						{
							temp_int &= UCHAR_MAX;
						}

						if (p.second & ATTR_BOOL)
						{
							temp_int = (bool)temp_int;
						}

						value = std::to_string(temp_int);
					}
				}

				obj->attributes.insert(std::make_pair(p.first, value));
			}
		}
	}

	// Get subtype of all traps (special case)

	for each (Obj *obj in objTypeList["traps"]->objList)
	{
		if (obj->attributes["type"] == "1") // Bounce trap
		{
			obj->attributes["subtype"] = std::to_string(readMemoryInt(handle_process, obj->pointer + 0x110));
		}
		else if (obj->attributes["type"] == "8") // Travel rune
		{
			obj->attributes["subtype"] = std::to_string(readMemoryInt(handle_process, obj->pointer + 0x10C));
		}
		else if (obj->attributes["type"] == "10") // Fire trap
		{
			obj->attributes["subtype"] = std::to_string(readMemoryInt(handle_process, obj->pointer + 0x10C));
		}
		else
		{
			obj->attributes["subtype"] = "-1";
		}
	}

	// Get run seed

	unsigned int seed = readMemoryInt(handle_process, getBaseAddress(handle_process, "NecroDancer.exe") + 0x435AF4);

	// Close handle for the game

	closeProcess(handle_process);

	// Generate XML document

	pugi::xml_document doc;

	pugi::xml_node node_seedcomment = doc.append_child(pugi::node_comment);
	node_seedcomment.set_value((" Seed: " + std::to_string(seed) + " ").c_str());

	pugi::xml_node node_dungeon = doc.append_child("dungeon");
	node_dungeon.append_attribute("character") = character;
	node_dungeon.append_attribute("name") = dungeonName.c_str();
	node_dungeon.append_attribute("numLevels") = 1;

	pugi::xml_node node_level = node_dungeon.append_child("level");
	node_level.append_attribute("bossNum") = -1;
	node_level.append_attribute("music") = music;
	node_level.append_attribute("num") = 1;

	for (auto const &el : objTypeList)
	{
		ObjType *objType = el.second;

		pugi::xml_node node_level_objects = node_level.append_child(objType->name_plural.c_str());

		for (unsigned int j = 0; j < objType->objList.size(); j++)
		{
			pugi::xml_node node_level_object = node_level_objects.append_child(objType->name_singular.c_str());

			for (auto const& p : objType->objList[j]->attributes)
			{
				node_level_object.append_attribute(p.first.c_str()) = p.second.c_str();
			}
		}
	}

	// Save XML document

	switch (output)
	{
	case 0:
	default:
		doc.save_file(filepath.c_str());
		break;
	case 1:
		doc.save(std::cout);
		break;
	}
	
	logfile << "XML file generated\n";

	return 0;
}
