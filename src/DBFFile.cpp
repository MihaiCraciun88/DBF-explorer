#include "DBFFile.h"

DBFFile::DBFFile(std::string filepath)
{
	fopen_s(&pFile, filepath.c_str(), "r+b");
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

DBFFile DBFFile::Create(std::string filepath, std::vector<Field> fields)
{
	FILE* file = NULL;
	fopen_s(&file, filepath.c_str(), "w+b");
	if (file == NULL)
	{
		throw Exception("Cannot open file");
	}

	// write header
	Header header;
	header.type = 0;
	header.lastUpdate[0] = 0;
	header.lastUpdate[1] = 0;
	header.lastUpdate[2] = 0;
	header.numRecords = 0;
	header.headerSize = 33;
	header.recordSize = 0;
	header.flagProduction = 0;
	header.languageDriverId = 0;
	for (int i = 0; i < 15; i++)
	{
		header.reserved[i] = 0;
	}
	for (int i = 0; i < 2; i++)
	{
		header.reserved2[i] = 0;
	}
	fwrite(&header, sizeof(header), 1, file);

	// write fields
	fseek(file, 32, SEEK_SET);
	for (Field& field : fields)
	{
		fwrite(&field, sizeof(field), 1, file);
		header.headerSize += 32;
	}
	char eoh = 0x0D;
	fwrite(&eoh, sizeof(eoh), 1, file);
	

	// add header size
	fseek(file, 8, SEEK_SET);
	fwrite(&header.headerSize, sizeof(header.headerSize), 1, file);

	fclose(file);

	return DBFFile(filepath);
}
