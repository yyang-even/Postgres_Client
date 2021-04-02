#pragma once

#include <memory>
#include <string>
#include <vector>


namespace cxxopts {

class Options;
class ParseResult;

}

namespace pqxx {

class connection;

}


namespace psqlxx {

namespace internal {

/**
 * @note    password escaping and quoting are currently not supported.
 */
[[nodiscard]]
const std::string overridePassword(std::string connection_string, const char *password);

}//namespace internal

struct DbOptions {
    std::string base_connection_string;
    std::vector<std::string> commands;

    bool prompt_for_password = true;

    bool list_DBs_and_exit = false;
};


[[nodiscard]]
const std::shared_ptr<pqxx::connection> MakeConnection(const DbOptions &options);

[[nodiscard]]
bool DoTransaction(const std::shared_ptr<pqxx::connection> connection_ptr,
                   const std::string_view sql_cmd);

void AddDbOptions(cxxopts::Options &options);

[[nodiscard]]
const DbOptions HandleDbOptions(const cxxopts::ParseResult &parsed_options);


enum class DbParameterKey {
    host,
    port,
    dbname,
    user,
    password,
};

[[nodiscard]]
const std::string
ComposeDbParameter(const DbParameterKey key_enum, std::string value);

[[nodiscard]]
const std::string BuildListDBsSql();

}//namespace psqlxx
