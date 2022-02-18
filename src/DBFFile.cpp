#include "DBFFile.h"

DBFFile::DBFFile(const char* filepath)
{
	fopen_s(&pFile, filepath, "rb");
	if (pFile == NULL)
	{
		throw Exception("Cannot open file");
	}
	int sizeHeader = 32;
	int sizeField = 32;
	if (fgets(reinterpret_cast<char*>(&mHeader), sizeHeader, pFile) == NULL)
	{
		throw Exception("Cannot read header");
	}
	fseek(pFile, sizeHeader, SEEK_SET);

	for (int i = 0; true; ++i)
	{
		char end;
		if (fread(&end, 1, 1, pFile) == NULL || end == 0x0D)
		{
			break;
		}
		fseek(pFile, sizeHeader + (sizeField * i), SEEK_SET);

		Field field;
		if (fread(reinterpret_cast<char*>(&field), 1, sizeField, pFile) == NULL)
		{
			break;
		}
		mFields.push_back(field);

		mRowSize += field.length;
		mLargestFieldSize = std::max(mLargestFieldSize, static_cast<size_t>(field.length));
		// printf("Name: %s, Tip: %c, Marime: %d\n", field.archName, field.chFieldType, field.uLength);
	}
}

DBFFile::~DBFFile()
{
	if (pFile != nullptr)
	{
		fclose(pFile);
		pFile = nullptr;
	}
}

std::vector<DBFFile::Field> DBFFile::GetFields()
{
	return mFields;
}

json DBFFile::GetRecords()
{
	json records = {};
	std::vector<DBFFile::Field> fields = GetFields();
	char buffer[256];

	for (int i = 0; i < mHeader.numRecords; ++i)
	{
		int pointer = mHeader.headerSize + (mRowSize * i) + i;
		fseek(pFile, pointer, SEEK_SET);
		char deleted;
		if (fread(reinterpret_cast<char*>(&deleted), 1, 1, pFile) == NULL)
		{
			break;
		}
		if (deleted == 0x2a)
		{
			continue;
		}
		if (deleted == 0x1a)
		{
			break;
		}

		json row = {};
		for (Field& field : fields)
		{
			if (fread(reinterpret_cast<char*>(&buffer), 1, field.length, pFile) == NULL)
			{
				break;
			}
			buffer[field.length] = 0;
			std::string value = buffer;
			row[field.name] = trim(value);
		}
		records.push_back(row);
	}

	return records;
}
