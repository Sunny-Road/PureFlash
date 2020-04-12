#define _ISOC99_SOURCE //to use strtoll
#include <semaphore.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cxxopts.hpp"
#include "s5_client_api.h"
#include "s5_log.h"
#include "s5_utils.h"

using namespace  std;

void
parse(int argc, char* argv[])
{
	try
	{
		cxxopts::Options options(argv[0], " - example command line options");
		options
				.positional_help("[optional args]")
				.show_positional_help();

		bool apple = false;

		options
				.allow_unrecognised_options()
				.add_options()
//						("a,apple", "an apple", cxxopts::value<bool>(apple))
//						("b,bob", "Bob")
//						("char", "A character", cxxopts::value<char>())
//						("t,true", "True", cxxopts::value<bool>()->default_value("true"))
//						("f, file", "File", cxxopts::value<std::vector<std::string>>(), "FILE")
//						("i,input", "Input", cxxopts::value<std::string>())
//						("o,output", "Output file", cxxopts::value<std::string>()
//								->default_value("a.out")->implicit_value("b.def"), "BIN")
//						("positional",
//						 "Positional arguments: these are the arguments that are entered "
//						 "without an option", cxxopts::value<std::vector<std::string>>())
//						("long-description",
//						 "thisisareallylongwordthattakesupthewholelineandcannotbebrokenataspace")
//						("help", "Print help")
//						("int", "An integer", cxxopts::value<int>(), "N")
//						("float", "A floating point number", cxxopts::value<float>())
//						("vector", "A list of doubles", cxxopts::value<std::vector<double>>())
//						("option_that_is_too_long_for_the_help", "A very long option")
//#ifdef CXXOPTS_USE_UNICODE
//			("unicode", u8"A help option with non-ascii: à. Here the size of the"
//        " string should be correct")
//#endif
				;

//		options.add_options("Group")
//				("c,compile", "compile")
//				("d,drop", "drop", cxxopts::value<std::vector<std::string>>());

		options.parse_positional({"input", "output", "positional"});

		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help({"", "Group"}) << std::endl;
			exit(0);
		}

		if (apple)
		{
			std::cout << "Saw option ‘a’ " << result.count("a") << " times " <<
					  std::endl;
		}

		if (result.count("b"))
		{
			std::cout << "Saw option ‘b’" << std::endl;
		}

		if (result.count("char"))
		{
			std::cout << "Saw a character ‘" << result["char"].as<char>() << "’" << std::endl;
		}

		if (result.count("f"))
		{
			auto& ff = result["f"].as<std::vector<std::string>>();
			std::cout << "Files" << std::endl;
			for (const auto& f : ff)
			{
				std::cout << f << std::endl;
			}
		}

		if (result.count("input"))
		{
			std::cout << "Input = " << result["input"].as<std::string>()
					  << std::endl;
		}

		if (result.count("output"))
		{
			std::cout << "Output = " << result["output"].as<std::string>()
					  << std::endl;
		}

		if (result.count("positional"))
		{
			std::cout << "Positional = {";
			auto& v = result["positional"].as<std::vector<std::string>>();
			for (const auto& s : v) {
				std::cout << s << ", ";
			}
			std::cout << "}" << std::endl;
		}

		if (result.count("int"))
		{
			std::cout << "int = " << result["int"].as<int>() << std::endl;
		}

		if (result.count("float"))
		{
			std::cout << "float = " << result["float"].as<float>() << std::endl;
		}

		if (result.count("vector"))
		{
			std::cout << "vector = ";
			const auto values = result["vector"].as<std::vector<double>>();
			for (const auto& v : values) {
				std::cout << v << ", ";
			}
			std::cout << std::endl;
		}

		std::cout << "Arguments remain = " << argc << std::endl;

		auto arguments = result.arguments();
		std::cout << "Saw " << arguments.size() << " arguments" << std::endl;
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}
}
struct io_waiter
{
	sem_t sem;
	int rc;
};

void io_cbk(int complete_status, void* cbk_arg)
{
	struct io_waiter* w = (struct io_waiter*)cbk_arg;
	w->rc = complete_status;
	sem_post(&w->sem);
}
static int64_t parseNumber(string str)
{
	if(str.length() == 1)
		return strtoll(str.c_str(), NULL, 10);
	int64_t l = strtoll(str.substr(0, str.length() - 1).c_str(), NULL, 10);
	switch(str.at(str.length()-1)){
		case 'k':
		case 'K':
			return l << 10;
		case 'm':
		case 'M':
			return l <<20;
		case 'g':
		case 'G':
			return l <<30;
		case 't':
		case 'T':
			return l <<40;
	}
	return strtoll(str.c_str(), NULL, 10);
}


int main(int argc, char* argv[])
{
	cxxopts::Options options(argv[0], " - PureFlash dd tool");
	string rw, bs_str, ifname, ofname, vol_name, cfg_file;
	int count;
	off_t offset;

	options.positional_help("[optional args]")
			.show_positional_help();
	options
			.add_options()
			("rw", "Read/Write", cxxopts::value<std::string>(rw)->default_value("read"), "read/write")
					("count", "Block count", cxxopts::value<int>(count)->default_value("1"))
					("bs", "Block size", cxxopts::value<string>(bs_str)->default_value("4k"))
					("if", "Input file name", cxxopts::value<string>(ifname)->default_value(""))
					("of", "Output file name", cxxopts::value<string>(ofname)->default_value(""))
					("c", "Config file name", cxxopts::value<string>(cfg_file)->default_value("/etc/pureflash/s5.conf"))
					("offset", "Offset in volume", cxxopts::value<off_t>(offset)->default_value("0"))
					("v", "Volume name", cxxopts::value<string>(vol_name))
					("h,help", "Print usage")
					;
	if(argc == 1) {
		std::cout << options.help() << std::endl;
		exit(1);
	}
	try {
		auto result = options.parse(argc, argv);
		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			exit(0);
		}
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		options.help();
		exit(1);
	}
	int64_t bs = parseNumber(bs_str);
	//TODO: need argments checking

	void* buf = malloc(bs);
	DeferCall _c([buf](){free (buf);});
	int fd;
	int is_write = 0;
	if(rw == "read") {
		fd = open(ifname.c_str(), O_RDONLY);
		is_write = 0;
	} else if(rw == "write") {
		fd = open(ofname.c_str(), O_WRONLY|O_CREAT|O_TRUNC);
		is_write = 1;
	} else {
		S5LOG_FATAL("Invalid argument rw:%s", rw.c_str());
	}
	if(fd == -1) {
		S5LOG_FATAL("Failed open file:%s, errno:%d", ifname.c_str(), errno);
	}
	DeferCall _c3([fd](){close(fd);});
	io_waiter arg;
	sem_init(&arg.sem, 0, 0);
	struct S5ClientVolumeInfo* vol = s5_open_volume(vol_name.c_str(),cfg_file.c_str(), NULL, S5_LIB_VER);
	if(vol == NULL) {
		S5LOG_FATAL("Failed open volume:%s", vol_name.c_str());
	}
	DeferCall _c2([vol](){s5_close_volume(vol);});

	S5LOG_INFO("%s with block size:%s", is_write ? "Write":"Read", bs);
	int64_t offset_in_file = 0;
	for(int i=0;i<count;i++) {
		if(is_write) {
			s5_io_submit(vol, buf, bs, offset + i * bs, io_cbk, &arg, is_write);
			sem_wait(&arg.sem);
			if(arg.rc != 0) {
				S5LOG_FATAL("Failed read data from volume, rc:%d", arg.rc);
			}
			ssize_t rc = pwrite(fd, buf, bs, offset_in_file + i * bs);
			if(rc != bs) {
				S5LOG_FATAL("Failed write data from file, rc:%l, errno:%d", rc, errno);
			}

		} else {
			ssize_t rc = pread(fd, buf, bs, offset_in_file + i * bs);
			if(rc != bs) {
				S5LOG_FATAL("Failed read data from file, rc:%l, errno:%d", rc, errno);
			}
			s5_io_submit(vol, buf, bs, offset + i * bs, io_cbk, &arg, is_write);
			sem_wait(&arg.sem);
			if(arg.rc != 0) {
				S5LOG_FATAL("Failed read data from volume, rc:%d", arg.rc);
			}
		}
	}
	S5LOG_INFO("Succeeded %s %d blocks", is_write ? "Write" : "Read", count);
	return 0;
}