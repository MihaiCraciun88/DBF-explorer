#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "DBFFile.h"
#include "httplib/httplib.hpp"


int main(int argc, const char** argv)
{
	// HTTP
	httplib::Server svr;
	auto ret = svr.set_mount_point("/", "./public");

	svr.Get("/api/files", [](const httplib::Request&, httplib::Response& res) {
		json files = getDirFiles("./0001");
		res.set_content(files.dump().c_str(), "application/json");
	});
	svr.Get(R"(/api/table/(.+))", [](const httplib::Request& req, httplib::Response& res) {
		json records = {};
		std::ssub_match filename = req.matches[1];
		try
		{
			DBFFile file(filename.str().c_str());

			std::vector<DBFFile::Field> fields = file.GetFields();
			records = file.GetRecords();
		}
		catch (Exception& e)
		{
			printf("Exception: %s\n", e.what());
		}
		res.set_content(records.dump().c_str(), "application/json");
	});

	svr.listen("0.0.0.0", 8080);

	return 0;
}