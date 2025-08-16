#pragma once

#include <boost/json.hpp>
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <optional>

namespace logger {

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;
namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

#define LOG_WITH_DATA(lvl, data, msg) \
    BOOST_LOG_TRIVIAL(lvl) << logging ::add_value("AdditionalData", json::value(data)) << msg

void ServerLogFormater(logging::record_view const& rec, logging::formatting_ostream& strm);

void InitLogger();

json::object ServerStartedData(const std::string& address, uint32_t port);

json::object ServerStopedData(int code, const std::optional<std::string>& exception_description);

json::object ServerStopedData(int code);

json::object ServerErrorData(int code, const std::string& text, const std::string& where);

json::object ServerRequestData(const std::string& ip, const std::string& uri, const std::string& method);

json::object ServerResponseData(int32_t response_duration, int code, const std::string& content_type);

} // logger