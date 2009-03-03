#ifndef __VIEW_H__
#define __VIEW_H__

void View_Init();
void View_Start();
void View_Poll();
void View_End();

void View_OnRosterTabDrop(struct Box_s *pbox, char *name, int x, int y);

void View_PopupLogin();
void View_CloseLogin();

void View_PopupRoster();
void View_CloseRoster();

void View_PopupSimpleDialog(char *dialogtype, void *userdata);

void View_PopupChatDialog(char *jid, char *nick, int isGroupChat);
void View_CloseChatDialog(struct Box_s *pbox);
void View_AddChatMessage(char *jid, char *nick, char *msg);

void View_PopupRemoveFriendDialog(char *name, char *group);

void View_CloseSpawnWindow(struct Box_s *pbox);

void View_SetUser(char *jid, char *nick);
void View_SetPresence(enum SStatus status, char *statusmsg);
void View_ResendPresence();
void View_RemoveAllFriends();
void View_AddFriend(char *jid, char *nickname, char *group, enum SStatus status, char *statusmsg, int inroster);
void View_RemoveFriend(char *jid, char *group, int inroster);
void View_SetFriendStatus(char *jid, char *nick, char *group,
	enum SStatus status, char *statusmsg, char *avatarhash, char *rating,
	struct namedlist_s *roles, struct namedlist_s *titles,
	char *gameid, struct gamesearchinfo_s *info, int stopped, int watching,
	struct adhoccommand_s *command);

void View_AddGroup(char *name);
void View_RemoveGroup(char *name);
void View_RenameGroup(char *oldname, char *newname);

void View_AddAwayStatus(char *name);
void View_AddAvailableStatus(char *name);
void View_SetAwayStatusList(struct StatusList_s *list);
void View_SetAvailableStatusList(struct StatusList_s *list);

void View_NubRoster();
void View_UnnubRoster();
void View_RosterAutoReconnect();
void View_RosterDisconnectError();
void View_SetRosterNormal();

void View_SetLoginNormal();
void View_SetLoginConnecting();
void View_SetLoginFailed();
void View_SetLoginNoUsername();
void View_SetLoginBadUsername();
void View_SetLoginNoPassword();

void View_SetStatusList(enum SStatus status, struct StatusList_s *list);
void View_SetChatParticipantStatus(char *jid, enum SStatus status, char *statusmsg, char *role, char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist, char *actorjid, char *reason, int notactivated, char *membertype);

void View_CloseAllChatDialogs();
void View_ReconnectAllChatDialogs();

void View_SetChatroomLoadStatusFinished();

void View_AddChatroom(char *group, char *jid, char *name, char *topic, int users);
void View_AddGroupChatMessage(char *jid, char *msg, char *timestamp);

void View_RoomNickConflict(char *jid);

void View_ClearChatrooms();

void View_RemoveChatroom(char *group, char *jid);

void View_PopupChessGame(char *gameid, char *roomjid, char *oldgameid);
void View_CloseChessGame(char *gameid);

void View_ParseGameMove(char *gameid, char *move, char *annotation, char *opponentjid, int correspondence, int numtakebacks, int ply);
void View_ResetGamePosition(char *gameid, int illegalmove);
void View_ParseGameState(char *gameid, char *initialstate, char *state);
void View_SyncClock(char *gameid, char *time, char *side, char *control, int tick);
void View_SetClockControl(char *gameid, char *side, int *controlarray);

void View_SetAvatar(char *filename);

void View_SetFriendsOfflineVisibility(BOOL visible);

void View_SetShowRosterTab(BOOL show, char *name);
void View_SetShowFriendAvatars(BOOL show);
void View_SetShowFriendGroups(BOOL show);
void View_ToggleShowChatParticipants(char *jid);

void View_OnChatTabDrop(char *jid, int isGroupChat, int x, int y);
void View_OnJIDDrop(char *jid, int xmouse, int ymouse);

void View_PopupGameCreateAdDialog(struct gamesearchinfo_s *info);
void View_CloseGameCreateAdDialog();

void View_PopupGameInvitationDialog(char *jid, struct gamesearchinfo_s *info, char *oldgameid);
void View_CloseGameInvitationDialog();

void View_PopupGameInviteFriendDialog(char *jid, struct gamesearchinfo_s *info, int autojoin);
void View_CloseGameInviteFriendDialog();

void View_PopupGameRespondAdDialog(char *from, struct gamesearchinfo_s *info);
void View_CloseGameRespondAdDialog();

void View_PopupReconveneDialog(char *opponentjid, char *gameid, struct gamesearchinfo_s *info, int inviting);
void View_CloseReconveneDialog();

void View_SetProfileInfo(char *jid, struct namedlist_s *profileinfo);
void View_PopupProfileDialog(char *jid, char *nick);

void View_AddMoveToMoveList(char *gameid, char *move, char *annotation);
void View_SetGameViewRotated(char *gameid, int rotated);
void View_NoSearchGameResults();
void View_SetRosterRating(int rating);

void View_SetWhitePlayer(char *gameid, char *jid, char *nick, int islocalplayer);
void View_SetBlackPlayer(char *gameid, char *jid, char *nick, int islocalplayer);
void View_AddSearchGame(char *itemid, char *node, char *jid, struct gamesearchinfo_s *info);
void View_AddGameChatMessage(char *gameid, char *nick, char *msg);
void View_RefreshGamesListPage();

void View_HandleAdjourn(char *gameid, char *white, char *black);
void View_HandleDraw(char *gameid, char *from, int whiteaccept,	int blackaccept, int correspondence);
void View_HandleRejectAdjourn(char *gameid);
void View_HandleRejectDraw(char *gameid, char *from, int correspondence);
void View_HandleGameOver(char *gameid, char *type, char *win, char *lose,
  char *reason, int correspondence, char *opponentjid, int localwin,
  int locallose, char *datestarted);
void View_FriendsOpenAllGroup();
void View_SetTotalGamesCount(int count);

void View_RequestMatchError(char *opponentjid, char *error);
void View_ReconveneError(char *gameid, char *error);

void View_SetComposing(char *jid, char *msgid);
void View_UnsetComposing(char *jid, char *msgid);

void View_ShowGameMessage(char *jid, char *nick, char *gameid, struct gamesearchinfo_s *info, int whitelocal, int blacklocal);

void ViewLink_CorGameOpen_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_CorGameResign_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_CorGameAcceptDraw_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_CorGameRejectDraw_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_GameOpen_OnClick(struct Box_s *pbox, struct gamesearchinfo_s *info);
void ViewLink_GameResign_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_RespondAd_OnClick(struct Box_s *pbox, struct gamesearchinfo_s *info);
void ViewLink_WatchGame_OnClick(struct Box_s *pbox, char *gameid);
void ViewLink_JoinChat_OnClick(struct Box_s *pbox, char *roomjid);
void ViewLink_AddContact_OnClick(struct Box_s *pbox, char *jid);
void ViewLink_ShowProfile(struct Box_s *pbox, char *jid);
void ViewLink_MatchLink_OnClick(struct Box_s *pbox, char *jid);
void ViewLink_HelpLink_OnClick(struct Box_s *pbox, char *helplink);
void ViewLink_ProfileLink_OnClick(struct Box_s *pbox, char *jid);
void ViewLink_OpenChat(struct Box_s *pbox, char *jid);

void View_PopupTourneyDialog(char *tourneyid, char *tourneychatjid);
void View_CloseTourneyDialog(char *tourneyid);
void View_AddTourneyChatMessage(char *tourneyid, char *nick, char *msg);
void View_SetTourneyChatParticipantStatus(char *jid, enum SStatus status,
  char *statusmsg, char *role, char *affiliation, char *realjid,
  char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason);
void View_TournamentAddPlayer(char *tourneyid, char *playerjid);
void View_TournamentRemovePlayer(char *tourneyid, char *playerjid);
void View_TournamentStartRound(char *tourneyid, int round, struct namedlist_s *pairinglist);
void View_TournamentEndRound(char *tourneyid, int round);
void View_TournamentEnd(char *tourneyid, char *winner);
void View_TournamentGamePlaying(char *tourneyid, char *gameid, int round, char *white, char *black);
void View_TournamentGameStoppedPlaying(char *tourneyid, char *gameid, int round, char *white, char *black, char *winner);
void View_TournamentUpdatePlayer(char *tourneyid, char *jid, struct tournamentplayerinfo_s *pinfo);

void View_GetWindowPosFromRegistry();
void View_SetUIFlags(enum uiflags_e uiflags);
void View_SetStatusOnAllChats(enum SStatus status, char *statusmsg);
void View_RefreshChatDialog(char *jid);

void View_SetGameChatParticipantStatus(char *jid, enum SStatus status,
  char *statusmsg, char *role, char *affiliation, char *realjid,
  char *nickchange, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, char *actorjid, char *reason,
  int notactivated, char *membertype);

void View_SetGroupChatTopic(char *jid, char *topic);
void View_ShowDisconnectOnChats();
void View_CloseAllGameRequestDialogs();
void View_ShowCorGameAccepted(char *gameid, char *from);
void View_ClearMoveList(char *gameid);
void View_SetGameInfo(char *gameid, struct gamesearchinfo_s *info, int whitelocal, int blacklocal, char *tourneyid);
void View_ClearGamesSearch(char *node);
void View_AddSearchTournament(struct tournamentinfo_s *info);
void View_FinishGameResults(int noresults);

void View_SetRosterProfile(struct namedlist_s *ratinglist, struct namedlist_s *roleslist, struct namedlist_s *titleslist);
void View_ActivateChatDialog(char *jid, char *nick, int isGroupChat);
void View_RequestMatchSetID(char *opponentjid, char *gameid);
void View_RematchSetID(char *oldgameid, char *newgameid);

void View_GameRematchError(char *oldgameid, char *error, char *actor);
void View_GameRequestError(char *opponentjid, char *gameid, char *error, char *actor, char *adid);

void View_SetTournamentInfo(struct tournamentinfo_s *info);

void View_ShowGameLogoutMessage(char *jid, char *nick);
void View_ShowGameWatchInviteMessage(char *friendjid, char *chatjid);
void View_ShowChatInviteMessage(char *friendjid, char *chatjid);
void View_ShowClientUpgradeMessage(char *from, char *number, char *version, char *build, char *url);
void View_ShowLoginMessage();
void View_SetAdjournedCount(int adjourned);
void View_SetCorrespondenceCount(int correspondence);
void View_ShowAnnouncement(char *from, char *text, char *subject);

struct namedlist_s *View_GetOpenGames();
int View_LocalUserHasMucPower(char *jid);
void View_PopupInviteFriendToChessparkDialog(char *jid);

void View_PopupInviteToChatDialog(char *friendjid, char *chatjid);

void View_PopupRoundStartDialog(char *tourneyid);
void View_CloseGameRequestDialog(char *jid, char *gameid);

void View_CloseGamesWithSamePlayers(char *newgameid, char *player1, char *player2);
void View_SetSavedWindowPos2(char *id, int x, int y, int w, int h, int dw, int group);
struct namedlist_s **View_GetPtrToChat(char *jid, int isGroupChat);

void View_PopupNewGamesDialog();
void View_CloseNewGamesDialog();
void View_ToggleNewGamesDialog();

void View_SpawnRosterTabWindow(char *name, int x, int y, int w, int h);

void View_PopupOptionsDialog();
void View_CloseOptionsDialog();

void View_SetSavedWindowPos(char *id, int x, int y, int w, int h);

void View_PopupGameWaitingDialog(char *titletext, char *dialogtext);

void View_ShowRosterLoading();

void View_SetOptions();
void View_ReconnectChatDialogs(int chessparkchats);

void View_FinishRoster();

void View_ScrollFriendVisible(char *jid);

void View_RefreshChatIcon(char *jid);

int View_IsPlayingAGame();

void View_PopupOfflineWarning();
void View_PopupDisconnectWarning();

void View_SetDisconnectOnChats();

void View_SetGameMoveList(char *gameid, struct namedlist_s *textmovelist);
void View_SwitchGameClockToNonlocalPlayer(char *gameid, unsigned int lag);
void View_ShowGameMucError(char *gameid, int icode);
void View_GroupChatError(char *roomjid, int icode, char *error);
void View_SetGameFinderExpiryReminder(int expiry);
void View_ShowRatingUpdate(char *from, char *jid, struct rating_s *newrating);
void View_HandleAbort(char *gameid, char *white, char *black);
void View_HandleTakeback(char *gameid, char *white, char *black);
struct gamesearchinfo_s *View_GetGameInfo(char *gameid);
void View_HandleRejectAbort(char *gameid);
void View_HandleRejectTakeback(char *gameid);
void View_AddAdSuccess(char *id);
void View_ShowFriendPlayingPopup(char *from, char *gameid, struct gamesearchinfo_s *info);
void View_ShowMemberExpired();
void View_ShowMemberNotFound();
void View_ShowMemberLoginsExceeded();
void View_RefreshNewGamesDialog();
void View_ShowUpgradedMessage();
void View_PopupBanlist(char *jid, struct namedlist_s *banlist);
void View_GenericError(char *errortitle, char *error);
void View_AddPushGame(char *type, char *id, struct gamesearchinfo_s *info);
void View_RemovePushGame(char *type, char *id);
void View_PopupQuitWarning();
void View_PopupPingDialog();
void View_PopupSendDebugLogDialog();
void View_PopupIgnoreDialog(char *target);
void View_PopupBanDialog(char *muc, char *target);
void View_ResetLanguage(char *langcode);
void View_KillGameAds();
int View_IsInRoom(char *roomjid);
int View_GameToFront(char *gameid);
void View_PlayNow();
struct Box_s *View_PopupWaitingDialog(char *titlebartxt,
  char *topinfotext, char *bottominfotext);
void View_PopupReconveneReminderDialog(char *jid, struct gamesearchinfo_s *info);
struct Box_s *View_CreateEmptyChatBox(int x, int y);
void View_SetLastActiveChatBox(struct Box_s *chatbox);
void View_OnChatTabDropEmpty(struct Box_s *psrc, char *jid, int x, int y);
void View_CloseAllDialogs();

void View_PopupNotAProError(char *error);

#endif