// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "DBFFile.h"
#include "httplib/httplib.hpp"

const char* dirFiles = "./0001";
const char* serverIp = "127.0.0.1"; // 0.0.0.0 for all
int serverPort = 8080;

int main(int argc, const char** argv)
{
	// HTTP
	httplib::Server svr;
	auto ret = svr.set_mount_point("/", "./public");

	svr.Get("/api/files", [](const httplib::Request&, httplib::Response& res) {
		json files = getDirFiles(dirFiles);
		res.set_content(files.dump().c_str(), "application/json");
	});
	svr.Get(R"(/api/table/(.+))", [](const httplib::Request& req, httplib::Response& res) {
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
		}
		res.set_content(table.dump().c_str(), "application/json");
	});
	svr.Put("/api/table", [](const httplib::Request&, httplib::Response& res, const httplib::ContentReader& content_reader) {
		std::string body;
		content_reader([&](const char* data, size_t data_length) {
			body.append(data, data_length);
			return true;
		});
		json postData = json::parse(body);
		try
		{
			DBFFile file(postData["table"].get<std::string>().c_str());
			file.Update(postData["data"], postData["index"].get<int>());
		}
		catch (Exception& e)
		{
			printf("Exception: %s\n", e.what());
		}

		res.set_content(postData.dump().c_str(), "application/json");
	});

	svr.listen(serverIp, serverPort);

	return 0;
}
