// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "DBFFile.h"
#include "httplib/httplib.hpp"

using namespace httplib;

const char* dirFiles = "./data";
const char* serverIp = "127.0.0.1"; // 0.0.0.0 for all
int serverPort = 8080;

int main(int argc, const char** argv)
{
	// HTTP
	Server svr;
	auto ret = svr.set_mount_point("/", "./public");

	svr.Get("/api/files", [](const Request&, Response& res) {
		json files = getDirFiles(dirFiles);
		res.set_content(files.dump().c_str(), "application/json");
	});
	svr.Get(R"(/api/table/(.+))", [](const Request& req, Response& res) {
		json table = {};
		std::ssub_match filename = req.matches[1];
		try
		{
			DBFFile file(filename.str().c_str());

			table["header"] = file.GetHeader();
			table["records"] = file.GetRecords();
		}
		catch (Exception& e)
		{
			printf("Exception: %s\n", e.what());
			res.status = 404;
			res.set_content("File not found", "text/html");
			return;
		}
		res.set_content(table.dump().c_str(), "application/json");
	});
	svr.Put(R"(/api/table/(.+))", [](const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& content_reader) {
		json data = {};
		std::string body;
		content_reader([&](const char* data, size_t data_length) {
			body.append(data, data_length);
			return true;
		});

		std::ssub_match filename = req.matches[1];
		try
		{
			data = json::parse(body);
			DBFFile file(filename.str().c_str());
			file.Update(data["data"], data["index"].get<int>());
		}
		catch (Exception& e)
		{
			printf("Exception: %s\n", e.what());
		}

		res.set_content(data.dump().c_str(), "application/json");
	});
	svr.Post(R"(/api/table/(.+))", [](const Request& req, Response& res, const ContentReader& content_reader) {
		json data = {};
		std::string body;
		content_reader([&](const char* data, size_t data_length) {
			body.append(data, data_length);
			return true;
		});

		std::ssub_match filename = req.matches[1];
		try
		{
			data = json::parse(body);
			try
			{
				// insert
				DBFFile file(filename.str());
				file.Insert(data["row"]);
			}
			catch (Exception& e)
			{
				printf("Exception: %s\n", e.what());
				// create
				std::vector<DBFFile::Field> fields;
				for (json line : data["header"]["fields"].get<json>())
				{
					DBFFile::Field field;
					std::string name = line["name"].get<std::string>();
					memcpy(field.name, name.c_str(), (name.size() + 1) * sizeof(char));
					field.type = line["type"].get<std::string>().c_str()[0];
					field.decimalCount = line["decimalCount"].get<int>();
					field.length = line["length"].get<int>();

					fields.push_back(field);
				}
				DBFFile file = DBFFile::Create(std::string(dirFiles) + "/" + filename.str(), fields);
			}
		}
		catch (Exception& e)
		{
			printf("Exception: %s\n", e.what());
		}

		res.status = 201;
		res.set_content(data.dump().c_str(), "application/json");
	});

	//printVectorString(split("hello, there is lots of, delimiters, that may be even together,  ", " "));
	//printVectorString(split("hello, there is lots of, delimiters", "|"));

	printf("server started at: http://%s:%d\n", serverIp, serverPort);

	svr.listen(serverIp, serverPort);

	return 0;
}