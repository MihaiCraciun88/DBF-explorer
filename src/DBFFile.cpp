#include "DBFFile.h"

DBFFile::DBFFile(const char* filepath)
{
	fopen_s(&pFile, filepath, "r+b");
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

	mRowSize = 1;
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

std::vector<DBFFile::Field>& DBFFile::GetFields()
{
	return mFields;
}

json DBFFile::GetRecords()
{
	json records = {};
	std::vector<DBFFile::Field>& fields = GetFields();
	char buffer[256];

	for (int i = 0; i < mHeader.numRecords; ++i)
	{
		int pointer = mHeader.headerSize + (mRowSize * i);
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

		json row = json::array();
		for (Field& field : fields)
		{
			if (fread(reinterpret_cast<char*>(&buffer), 1, field.length, pFile) == NULL)
			{
				break;
			}
			buffer[field.length] = 0;
			std::string value = buffer;
			row.push_back(trim(value));
		}
		records.push_back(row);
	}

	return records;
}

json DBFFile::GetHeader()
{
	json header = {};
	header["date"] = { mHeader.lastUpdate[2], mHeader.lastUpdate[1], mHeader.lastUpdate[0] };
	header["size"] = mHeader.numRecords;
	header["fields"] = json::array();
	std::vector<DBFFile::Field>& fields = GetFields();
	for (Field& field : fields)
	{
		json row = json::object();
		const char type[2] = {field.type, 0};
		row["name"] = field.name;
		row["type"] = type;
		row["length"] = field.length;
		row["decimalCount"] = field.decimalCount;
		header["fields"].push_back(row);
	}
	return header;
}

bool DBFFile::Insert(json& data)
{
	WriteData(data, mHeader.numRecords);

	char eof = 0x1a;
	fwrite(&eof, 1, 1, pFile);

	// update row count
	mHeader.numRecords++;
	fseek(pFile, 4, SEEK_SET);
	fwrite(reinterpret_cast<char*>(&mHeader.numRecords), 4, 1, pFile);

	return true;
}

bool DBFFile::Update(json& data, int index)
{
	if (index > (mHeader.numRecords - 1))
	{
		return false;
	}

	WriteData(data, index);

	return true;
}

void DBFFile::WriteData(json& data, int index)
{
	// move pointer at the bottom
	int pointer = mHeader.headerSize + (mRowSize * index);
	fseek(pFile, pointer, SEEK_SET);

	// validate and insert data
	char buffer[256];
	buffer[0] = ' ';
	fwrite(buffer, 1, 1, pFile);
	std::vector<DBFFile::Field>& fields = GetFields();
	for (Field& field : fields)
	{
		memset(buffer, 0, 256);
		if (data.contains(field.name))
		{
			std::string fieldName = data[field.name].get<std::string>();
			memcpy(buffer, fieldName.c_str(), fieldName.size());
		}
		fwrite(buffer, field.length, 1, pFile);
	}
}
