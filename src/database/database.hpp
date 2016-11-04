#pragma once


#include <biboumi.h>
#ifdef USE_DATABASE

#include "biboudb.hpp"

#include <memory>

#include <litesql.hpp>
#include <chrono>

class Iid;

class Database
{
public:
  using time_point = std::chrono::system_clock::time_point;
  Database() = default;
  ~Database() = default;

  Database(const Database&) = delete;
  Database(Database&&) = delete;
  Database& operator=(const Database&) = delete;
  Database& operator=(Database&&) = delete;

  static void set_verbose(const bool val);

  template<typename PersistentType>
  static size_t count()
  {
    return litesql::select<PersistentType>(*Database::db).count();
  }
  /**
   * Return the object from the db. Create it beforehand (with all default
   * values) if it is not already present.
   */
  static db::GlobalOptions get_global_options(const std::string& owner);
  static db::IrcServerOptions get_irc_server_options(const std::string& owner,
                                                     const std::string& server);
  static db::IrcChannelOptions get_irc_channel_options(const std::string& owner,
                                                       const std::string& server,
                                                       const std::string& channel);
  static db::IrcChannelOptions get_irc_channel_options_with_server_default(const std::string& owner,
                                                                           const std::string& server,
                                                                           const std::string& channel);
  static db::IrcChannelOptions get_irc_channel_options_with_server_and_global_default(const std::string& owner,
                                                                                      const std::string& server,
                                                                                      const std::string& channel);
  static std::vector<db::MucLogLine> get_muc_logs(const std::string& chan_name, const std::string& server,
                                                  int limit=-1, const std::string& before="", const std::string& after="");

  static void close();
  static void open(const std::string& filename, const std::string& db_type="sqlite3");


private:
  static std::string gen_uuid();
  static std::unique_ptr<db::BibouDB> db;
};
#endif /* USE_DATABASE */


