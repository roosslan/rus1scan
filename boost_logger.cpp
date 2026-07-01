#include <Windows.h>
#include <Shlwapi.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "boost_logger.h"

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

std::string ru_logger::path_to_filename(const std::string &path) {

	return path.substr(path.find_last_of("/\\") + 1);
}

void ru_logger::log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << rec[expr::smessage];
}

void ru_logger::init_logging() {
    boost::log::add_common_attributes();

    auto consoleSink = boost::log::add_console_log(std::clog);
    consoleSink->set_formatter(&log_formatter);
    logging::core::get()->add_sink(consoleSink);

    const auto fs_sink = boost::log::add_file_log(
        boost::log::keywords::file_name = get_log_directory_path() + "\\logs\\rus1scan_%d.%m.%Y-%H_%M_%S.log",
        keywords::format = "%TimeStamp% % Message % ",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);

    fs_sink->locked_backend()->auto_flush(true);

    LOG_SAVE << "ru_logger init";
}

std::string ru_logger::get_log_directory_path() {
    char current_path[MAX_PATH];
    GetModuleFileName(nullptr, current_path, MAX_PATH);

    /* remove filename from the path */
    PathRemoveFileSpec(current_path);

    return std::string(current_path);
}