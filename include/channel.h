/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#ifndef CHANNEL_H
#define CHANNEL_H
/**
 * \include ircproto.h
 */
#include "ircproto.h"
/**
 * \file channel.h
 * \brief Contains the Channel class as well as all of it's contents as prototype declarations.
 */
/**
 * \class Channel
 * \brief A wrapper class for IRC Channels.
 * Wraps around IRC Channels and contains their respective properties
 * as well as IRC channel related actions as functions.
 */
/**
 * \var Flux::string name
 * \brief Name of the IRC Channel.
 */
/**
 * \var Flux::string topic
 * \brief Topic of the IRC Channel.
 */
/**
 * \var Flux::string topic_setter
 * \brief Name of the user who last set the topic of the IRC Channel.
 */
/**
 * \var time_t topic_time
 * \brief The time the topic of the IRC Channel was last set.
 */
/**
 * \var time_t creation_time
 * \brief Time the IRC Channel was last created.
 */
typedef std::map<User*, Channel*> UList;
class CoreExport Channel : public Base
{
public:
  Channel(Network*, const Flux::string&, time_t ts = time(NULL));
  ~Channel();
  UList UserList;
  User *finduser(const Flux::string&);
  Flux::string name;
  Flux::string topic;
  Flux::string topic_setter;
  Network *n;
  Flux::insensitive_map<Bot*> bots;
  time_t topic_time;
  time_t creation_time;
  void AddUser(User*);
  void DelUser(User*);
  void SendJoin();
//   void SendPart();
  void SendPart(const Flux::string &msg = "");
  void SendPart(const char*, ...);
  void SendWho();
  void SendMessage(const Flux::string&);
  void SendMessage(const char*, ...);
  void SendAction(const Flux::string&);
  void SendAction(const char*, ...);
  void SendNotice(const Flux::string&);
  void SendNotice(const char*, ...);
};
#endif
