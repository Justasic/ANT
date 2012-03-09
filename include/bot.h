/* Arbitrary Navn Tool -- Prototype for Bot Class
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#ifndef _BOT_H
#define _BOT_H
#include "user.h"
#include "network.h"

class CoreExport Bot : public User
{
public:
  Bot(Network *net, const Flux::string &n, const Flux::string &i, const Flux::string &real = "ANT Bot (http://ANT.Flux-Net.net/)");
  ~Bot();
  /* The network we're on*/
  Network *network;
  /* The current bot number used as the nickname */
  int BotNumber;
  /* List of chans the bot is in */
  Flux::map<Channel*> channels;
  /* IRCProto class for the network */
  IRCProto *ircproto;
  /* Announce a commit that has been made */
//   void AnnounceCommit(CommitMessage&);
  /* Join a channel */
  void Join(Channel*);
  void Join(const Flux::string&);
  /* Part a channel */
  void Part(Channel*, const Flux::string &msg = "");
  /* Quit the IRC network */
  void Quit(const Flux::string &msg = "");
  /* Change a nickname */
  void SetNick(const Flux::string&);
  /* Send the user credentials for connecting */
  void SendUser();
  /* Check that we still have a valid nick, and fix our nick if it isnt */
  void CheckNickName(const Flux::string &nick = "");
  /* Set a mode to the bot, User class does not have this so we do it manually */
  void SetMode(const Flux::string&);
};

#endif