#ifndef BOOST_LOGGER_H
#define BOOST_LOGGER_H

#include <string>
#include <boost/current_function.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>


namespace src = boost::log::sources;
namespace logging = boost::log;

#define LOG_SAVE BOOST_LOG_SEV(boost::log::trivial::logger::get(), boost::log::trivial::severity_level::trace)	\
	<< "<" << ru_logger::path_to_filename(__FILE__) << ":" << __LINE__ << "> " BOOST_CURRENT_FUNCTION << " | " 	\
	<< boost::log::add_value("Line", __LINE__)

namespace ru_logger {
	void init_logging();
	void log_formatter(logging::record_view const& rec, logging::formatting_ostream& strm);
	std::string path_to_filename(const std::string &path);
	std::string get_log_directory_path();
};

#endif