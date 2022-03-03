#pragma once

#include <stdio.h>
#include <vector>
#include <string>
#include "Exception.h"
#include "Utils.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

class DBFFile
{
public:
	struct Header
	{
		uint8_t type;
		char lastUpdate[3];

		uint32_t numRecords;

		uint16_t headerSize;
		uint16_t recordSize;

		char reserved[15];
		uint8_t flagProduction;
		uint8_t languageDriverId;

		char reserved2[2];
	};
	struct Field
	{
		char name[11];
		char type;

		uint32_t reserved;
		uint8_t length;
		uint8_t decimalCount;
		uint16_t workAreaId;

		char reserved2[12];
	};

	DBFFile(const char* filepath);
	~DBFFile();

	std::vector<DBFFile::Field>& GetFields();
	json GetRecords();
	json GetHeader();
	bool Insert(json& data);
	bool Update(json& data, int index);
	void WriteData(json& data, int index);
private:
	FILE* pFile = nullptr;
	Header mHeader;

	std::vector<Field> mFields;

	size_t mRowSize = 0;
	size_t mLargestFieldSize = 0;
};

