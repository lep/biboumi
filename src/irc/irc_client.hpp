#ifndef IRC_CLIENT_INCLUDED
# define IRC_CLIENT_INCLUDED

#include <irc/irc_message.hpp>
#include <irc/irc_channel.hpp>
#include <irc/iid.hpp>

#include <network/socket_handler.hpp>

#include <unordered_map>
#include <string>

class Bridge;

/**
 * Represent one IRC client, i.e. an endpoint connected to a single IRC
 * server, through a TCP socket, receiving and sending commands to it.
 *
 * TODO: TLS support, maybe, but that's not high priority
 */
class IrcClient: public SocketHandler
{
public:
  explicit IrcClient(const std::string& hostname, const std::string& username, Bridge* bridge);
  ~IrcClient();
  /**
   * Connect to the IRC server
   */
  void start();
  /**
   * Called when successfully connected to the server
   */
  void on_connected() override final;
  /**
   * Close the connection, remove us from the poller
   */
  void on_connection_close() override final;
  /**
   * Parse the data we have received so far and try to get one or more
   * complete messages from it.
   */
  void parse_in_buffer() override final;
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
  void send_private_message(const std::string& username, const std::string& body);
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
  /**
   * Send the QUIT irc command
   */
  void send_quit_command();
  /**
   * Forward the server message received from IRC to the XMPP component
   */
  void forward_server_message(const IrcMessage& message);
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
   * Save the topic in the IrcChannel
   */
  void on_topic_received(const IrcMessage& message);
  /**
   * The channel has been completely joined (self presence, topic, all names
   * received etc), send the self presence and topic to the XMPP user.
   */
  void on_channel_completely_joined(const IrcMessage& message);
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
   * A list of chan we want to join, but we need a response 001 from
   * the server before sending the actual JOIN commands. So we just keep the
   * channel names in a list, and send the JOIN commands for each of them
   * whenever the WELCOME message is received.
   */
  std::vector<std::string> channels_to_join;
  bool welcomed;
  /**
   * Each motd line received is appended to this string, which we send when
   * the motd is completely received
   */
  std::string motd;
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
  {"NOTICE", &IrcClient::forward_server_message},
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
  {"001", &IrcClient::on_welcome_message},
  {"PART", &IrcClient::on_part},
  {"ERROR", &IrcClient::on_error},
  {"QUIT", &IrcClient::on_quit},
  {"NICK", &IrcClient::on_nick},
  {"MODE", &IrcClient::on_mode},
  {"PING", &IrcClient::send_pong_command},
  {"KICK", &IrcClient::on_kick},
};

#endif // IRC_CLIENT_INCLUDED
