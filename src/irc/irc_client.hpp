#ifndef IRC_CLIENT_INCLUDED
# define IRC_CLIENT_INCLUDED

#include <irc/irc_message.hpp>
#include <irc/irc_channel.hpp>
#include <irc/iid.hpp>

#include <network/tcp_socket_handler.hpp>

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <set>

class Bridge;

/**
 * Represent one IRC client, i.e. an endpoint connected to a single IRC
 * server, through a TCP socket, receiving and sending commands to it.
 */
class IrcClient: public TCPSocketHandler
{
public:
  explicit IrcClient(std::shared_ptr<Poller> poller, const std::string& hostname, const std::string& username, Bridge* bridge);
  ~IrcClient();
  /**
   * Connect to the IRC server
   */
  void start();
  /**
   * Called when the connection to the server cannot be established
   */
  void on_connection_failed(const std::string& reason) override final;
  /**
   * Called when successfully connected to the server
   */
  void on_connected() override final;
  /**
   * Close the connection, remove us from the poller
   */
  void on_connection_close(const std::string& error) override final;
  /**
   * Parse the data we have received so far and try to get one or more
   * complete messages from it.
   */
  void parse_in_buffer(const size_t) override final;
  /**
   * Return the channel with this name, create it if it does not yet exist
   */
  IrcChannel* get_channel(const std::string& name);
  /**
   * Returns true if the channel is joined
   */
  bool is_channel_joined(const std::string& name);
  /**
   * Return our own nick
   */
  std::string get_own_nick() const;
  /**
   * Serialize the given message into a line, and send that into the socket
   * (actually, into our out_buf and signal the poller that we want to wach
   * for send events to be ready)
   */
  void send_message(IrcMessage&& message);
  /**
   * Send the PONG irc command
   */
  void send_pong_command(const IrcMessage& message);
  void send_ping_command();
  /**
   * Send the USER irc command
   */
  void send_user_command(const std::string& username, const std::string& realname);
  /**
   * Send the NICK irc command
   */
  void send_nick_command(const std::string& username);
  /**
   * Send the JOIN irc command.
   */
  void send_join_command(const std::string& chan_name);
  /**
   * Send a PRIVMSG command for a channel
   * Return true if the message was actually sent
   */
  bool send_channel_message(const std::string& chan_name, const std::string& body);
  /**
   * Send a PRIVMSG command for an user
   */
  void send_private_message(const std::string& username, const std::string& body, const std::string& type);
  /**
   * Send the PART irc command
   */
  void send_part_command(const std::string& chan_name, const std::string& status_message);
  /**
   * Send the MODE irc command
   */
  void send_mode_command(const std::string& chan_name, const std::vector<std::string>& arguments);
  /**
   * Send the KICK irc command
   */
  void send_kick_command(const std::string& chan_name, const std::string& target, const std::string& reason);
  void send_topic_command(const std::string& chan_name, const std::string& topic);
  /**
   * Send the QUIT irc command
   */
  void send_quit_command(const std::string& reason);
  /**
   * Send a message to the gateway user, not generated by the IRC server,
   * but that might be useful because we want to be verbose (for example we
   * might want to notify the user about the connexion state)
   */
  void send_gateway_message(const std::string& message, const std::string& from="");
  /**
   * Forward the server message received from IRC to the XMPP component
   */
  void forward_server_message(const IrcMessage& message);
  /**
   * When receiving the isupport informations.  See
   * http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt
   */
  void on_isupport_message(const IrcMessage& message);
  /**
   * Just empty the motd we kept as a string
   */
  void empty_motd(const IrcMessage& message);
  /**
   * Send the MOTD string as one single "big" message
   */
  void send_motd(const IrcMessage& message);
  /**
   * Append this line to the MOTD
   */
  void on_motd_line(const IrcMessage& message);
  /**
   * Forward the join of an other user into an IRC channel, and save the
   * IrcUsers in the IrcChannel
   */
  void set_and_forward_user_list(const IrcMessage& message);
  /**
   * Remember our nick and host, when we are joined to the channel. The list
   * of user comes after so we do not send the self-presence over XMPP yet.
   */
  void on_channel_join(const IrcMessage& message);
  /**
   * When a channel message is received
   */
  void on_channel_message(const IrcMessage& message);
  /**
   * A notice is received
   */
  void on_notice(const IrcMessage& message);
  /**
   * Save the topic in the IrcChannel
   */
  void on_topic_received(const IrcMessage& message);
  /**
   * The channel has been completely joined (self presence, topic, all names
   * received etc), send the self presence and topic to the XMPP user.
   */
  void on_channel_completely_joined(const IrcMessage& message);
  /**
   * We tried to set an invalid nickname
   */
  void on_erroneous_nickname(const IrcMessage& message);
  /**
   * When the IRC servers denies our nickname because of a conflict.  Send a
   * presence conflict from all channels, because the name is server-wide.
   */
  void on_nickname_conflict(const IrcMessage& message);
  /**
   * Idem, but for when the user changes their nickname too quickly
   */
  void on_nickname_change_too_fast(const IrcMessage& message);
  /**
   * Handles most errors from the server by just forwarding the message to the user.
   */
  void on_generic_error(const IrcMessage& message);
  /**
   * When a message 001 is received, join the rooms we wanted to join, and set our actual nickname
   */
  void on_welcome_message(const IrcMessage& message);
  void on_part(const IrcMessage& message);
  void on_error(const IrcMessage& message);
  void on_nick(const IrcMessage& message);
  void on_kick(const IrcMessage& message);
  void on_mode(const IrcMessage& message);
  /**
   * A mode towards our own user is received (note, that is different from a
   * channel mode towards or own nick, see
   * http://tools.ietf.org/html/rfc2812#section-3.1.5 VS #section-3.2.3)
   */
  void on_user_mode(const IrcMessage& message);
  /**
   * A mode towards a channel. Note that this can change the mode of the
   * channel itself or an IrcUser in it.
   */
  void on_channel_mode(const IrcMessage& message);
  void on_quit(const IrcMessage& message);
  /**
   * Return the number of joined channels
   */
  size_t number_of_joined_channels() const;
  /**
   * Get a reference to the unique dummy channel
   */
  DummyIrcChannel& get_dummy_channel();
  /**
   * Leave the dummy channel: forward a message to the user to indicate that
   * he left it, and mark it as not joined.
   */
  void leave_dummy_channel(const std::string& exit_message);

  const std::string& get_hostname() const { return this->hostname; }
  std::string get_nick() const { return this->current_nick; }
  bool is_welcomed() const { return this->welcomed; }

private:
  /**
   * The hostname of the server we are connected to.
   */
  const std::string hostname;
  /**
   * The user name used in the USER irc command
   */
  const std::string username;
  /**
   * Our current nickname on the server
   */
  std::string current_nick;
  /**
   * Raw pointer because the bridge owns us.
   */
  Bridge* bridge;
  /**
   * The list of joined channels, indexed by name
   */
  std::unordered_map<std::string, std::unique_ptr<IrcChannel>> channels;
  /**
   * A single channel with a iid of the form "hostname" (normal channel have
   * an iid of the form "chan%hostname".
   */
  DummyIrcChannel dummy_channel;
  /**
   * A list of chan we want to join, but we need a response 001 from
   * the server before sending the actual JOIN commands. So we just keep the
   * channel names in a list, and send the JOIN commands for each of them
   * whenever the WELCOME message is received.
   */
  std::vector<std::string> channels_to_join;
  /**
   * This flag indicates that the server is completely joined (connection
   * has been established, we are authentified and we have a nick)
   */
  bool welcomed;
  /**
   * See http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt section 3.3
   * We store the possible chanmodes in this object.
   * chanmodes[0] contains modes of type A, [1] of type B etc
   */
  std::vector<std::string> chanmodes;
  /**
   * See http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt
   * section 3.5
   */
  std::set<char> chantypes;
  /**
   * Each motd line received is appended to this string, which we send when
   * the motd is completely received
   */
  std::string motd;
  /**
   * See http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt section 3.14
   * The example given would be transformed into
   * modes_to_prefix = {{'&', 'a'}, {'*', 'b'}}
   */
  std::map<char, char> prefix_to_mode;
  /**
   * Available user modes, sorted from most significant to least significant
   * (for example 'ahov' is a common order).
   */
  std::vector<char> sorted_user_modes;
  /**
   * A list of ports to which we will try to connect, in reverse. Each port
   * is associated with a boolean telling if we should use TLS or not if the
   * connection succeeds on that port.
   */
  std::stack<std::pair<std::string, bool>> ports_to_try;
  /**
   * A set of (lowercase) nicknames to which we sent a private message.
   */
  std::set<std::string> nicks_to_treat_as_private;

  IrcClient(const IrcClient&) = delete;
  IrcClient(IrcClient&&) = delete;
  IrcClient& operator=(const IrcClient&) = delete;
  IrcClient& operator=(IrcClient&&) = delete;
};

/**
 * Define a map of functions to be called for each IRC command we can
 * handle.
 */
typedef void (IrcClient::*irc_callback_t)(const IrcMessage&);

static const std::unordered_map<std::string, irc_callback_t> irc_callbacks = {
  {"NOTICE", &IrcClient::on_notice},
  {"002", &IrcClient::forward_server_message},
  {"003", &IrcClient::forward_server_message},
  {"005", &IrcClient::on_isupport_message},
  {"RPL_MOTDSTART", &IrcClient::empty_motd},
  {"375", &IrcClient::empty_motd},
  {"RPL_MOTD", &IrcClient::on_motd_line},
  {"372", &IrcClient::on_motd_line},
  {"RPL_MOTDEND", &IrcClient::send_motd},
  {"376", &IrcClient::send_motd},
  {"JOIN", &IrcClient::on_channel_join},
  {"PRIVMSG", &IrcClient::on_channel_message},
  {"353", &IrcClient::set_and_forward_user_list},
  {"332", &IrcClient::on_topic_received},
  {"TOPIC", &IrcClient::on_topic_received},
  {"366", &IrcClient::on_channel_completely_joined},
  {"432", &IrcClient::on_erroneous_nickname},
  {"433", &IrcClient::on_nickname_conflict},
  {"438", &IrcClient::on_nickname_change_too_fast},
  {"001", &IrcClient::on_welcome_message},
  {"PART", &IrcClient::on_part},
  {"ERROR", &IrcClient::on_error},
  {"QUIT", &IrcClient::on_quit},
  {"NICK", &IrcClient::on_nick},
  {"MODE", &IrcClient::on_mode},
  {"PING", &IrcClient::send_pong_command},
  {"KICK", &IrcClient::on_kick},

  {"401", &IrcClient::on_generic_error},
  {"402", &IrcClient::on_generic_error},
  {"403", &IrcClient::on_generic_error},
  {"404", &IrcClient::on_generic_error},
  {"405", &IrcClient::on_generic_error},
  {"406", &IrcClient::on_generic_error},
  {"407", &IrcClient::on_generic_error},
  {"408", &IrcClient::on_generic_error},
  {"409", &IrcClient::on_generic_error},
  {"410", &IrcClient::on_generic_error},
  {"411", &IrcClient::on_generic_error},
  {"412", &IrcClient::on_generic_error},
  {"414", &IrcClient::on_generic_error},
  {"421", &IrcClient::on_generic_error},
  {"422", &IrcClient::on_generic_error},
  {"423", &IrcClient::on_generic_error},
  {"424", &IrcClient::on_generic_error},
  {"431", &IrcClient::on_generic_error},
  {"436", &IrcClient::on_generic_error},
  {"441", &IrcClient::on_generic_error},
  {"442", &IrcClient::on_generic_error},
  {"443", &IrcClient::on_generic_error},
  {"444", &IrcClient::on_generic_error},
  {"446", &IrcClient::on_generic_error},
  {"451", &IrcClient::on_generic_error},
  {"461", &IrcClient::on_generic_error},
  {"462", &IrcClient::on_generic_error},
  {"463", &IrcClient::on_generic_error},
  {"464", &IrcClient::on_generic_error},
  {"465", &IrcClient::on_generic_error},
  {"467", &IrcClient::on_generic_error},
  {"470", &IrcClient::on_generic_error},
  {"471", &IrcClient::on_generic_error},
  {"472", &IrcClient::on_generic_error},
  {"473", &IrcClient::on_generic_error},
  {"474", &IrcClient::on_generic_error},
  {"475", &IrcClient::on_generic_error},
  {"476", &IrcClient::on_generic_error},
  {"477", &IrcClient::on_generic_error},
  {"481", &IrcClient::on_generic_error},
  {"482", &IrcClient::on_generic_error},
  {"483", &IrcClient::on_generic_error},
  {"484", &IrcClient::on_generic_error},
  {"485", &IrcClient::on_generic_error},
  {"487", &IrcClient::on_generic_error},
  {"491", &IrcClient::on_generic_error},
  {"501", &IrcClient::on_generic_error},
  {"502", &IrcClient::on_generic_error},
};

#endif // IRC_CLIENT_INCLUDED
