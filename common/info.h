#ifndef __INFO_H__
#define __INFO_H__

#include <time.h>

struct playerinfo_s
{
	char *jid;
	char *rating;
	char *award;
	struct namedlist_s *titles;
	struct namedlist_s *roles;
	int notactivated;
	char *membertype;
};

struct searchlimit_s
{
	char *type;
	int low;
	int high;
};

struct timecontrol_s
{
	int correspondence;
	int delayinc;
	int *controlarray;
};

struct tcpair_s
{
	struct timecontrol_s *white;
	struct timecontrol_s *black;
};

struct gamesearchinfo_s
{
	char *node;
	char *gameid;
	struct playerinfo_s *adplayer;
	struct playerinfo_s *white;
	struct playerinfo_s *black;
	char *variant;
	struct timecontrol_s *timecontrol;
	struct timecontrol_s *blacktimecontrol;
	char *timecontrolrange;
	int rated;
	int computers;
	int titled;
	char *takebacks;
	int solitaire;
	struct searchlimit_s *limit;
	int relativerating;
	int filterunrated;
	char *keywords;
	char *comment;
	int colorpreference; /* 0 is none, 1 is white, 2 is black */
	int correspondence;
	char *category;
	float whiteclock;
	float blackclock;
	int movenum;
	int whitetomove;
	int blacktomove;
	char *lastmove;
	int offline;
	char *date;

	int tournament;
	char *manager;
	char *tournamentname;
	char *pairingtype;

	int filteropen;
	int quickapply;
	int hideown;
	int showonlymine;

	struct namedlist_s *groupids;
};

struct rating_s
{
	int rating;
	int rd;
	int best;
	int worst;
	int wins;
	int losses;
	int draws;
	int prevrating;
};

struct tournamentpairing_s
{
	char *gameid;
	char *white;
	char *black;
	char *bye;
	int gamestate;
	char *winner;
};

struct tournamentplayerinfo_s
{
	char *jid;
	float score;
	int wins;
	int draws;
	int losses;
	int rating;
};

struct tournamentinfo_s
{
	char *id;
	char *manager;
	char *name;
	char *variant;
	char *pairingtype;
	char *timecontrol;
	int currentround;
	int totalrounds;
	struct namedlist_s *players;
	struct namedlist_s *roundstarttimes;
	struct namedlist_s *roundpairings;
};

struct adhoccommand_s
{
	char *name;
	char *command;
};

struct profile_s
{
	struct namedlist_s *ratings;
	struct namedlist_s *roles;
	struct namedlist_s *titles;
	struct namedlist_s *playinggames;
	struct namedlist_s *watchinggames;
	struct namedlist_s *groups;
	char *nickname;
	char *avatarhash;
	char *description;
	char *membersince;
	char *lastonline;
	char *clientname;
	char *clientversion;
	char *clientvendor;
	char *clientos;
	char *membertype;
	char *countrycode;
	char *location;
	char *rank;
};

struct privacylistentry_s
{
	char *type;
	char *value;
	char *action;
	char *order;
};

struct groupinfo_s
{
	char *name;
	char *chat;
	char *forum;
	char *type;
	char *id;
	struct namedlist_s *roles;
	struct namedlist_s *titles;
	char *avatar;
	struct namedlist_s *permissions;
};


struct gamesearchinfo_s *Info_DupeGameSearchInfo(struct gamesearchinfo_s *src);
void Info_DestroyGameSearchInfo(struct gamesearchinfo_s *info);

struct timecontrol_s *Info_DupeTimeControl(struct timecontrol_s *src);
void Info_DestroyTimeControl(struct timecontrol_s *tc);
char *Info_TimeControlToText(struct timecontrol_s *tc);
char *Info_SecsToText1(float fsec);
void Info_SecsToText2(float fsec, char *showtime, int *msec, int longestonly);
void Info_SecsToText3(float fsec, char *showtime, int *msec, int longestonly, int offset);
char *Info_SecsToTextShort(char *buffer, int len, float fsec);
char *Info_SearchLimitToText(struct searchlimit_s *limit);
char *Info_TimeControlsToText(struct timecontrol_s *tc, struct timecontrol_s *blacktc);
char *Info_TimeControlToLongText(struct timecontrol_s *tc);
char *Info_TimeControlToStringList(struct timecontrol_s *src);
char *Info_TimeControlToCategory(struct timecontrol_s *tc);
int Info_TimeControlToEstimatedTime(struct timecontrol_s *tc);
struct timecontrol_s *Info_StringListToTimeControl(char *src);
char *Info_TimeControlsToShortText(struct timecontrol_s *wtc, struct timecontrol_s *btc);

char *Info_GetHighestRating(struct namedlist_s *ratinglist);

struct rating_s *Info_DupeRating(struct rating_s *src);

void Info_DestroyTournamentPairing(struct tournamentpairing_s *pairing);
void Info_DestroyTournamentPlayerInfo(struct tournamentplayerinfo_s *info);

struct adhoccommand_s *Info_DupeAdHocCommand(struct adhoccommand_s *src);
void Info_DestroyAdHocCommand(struct adhoccommand_s *cmd);

struct profile_s *Info_DupeProfile(struct profile_s *src);
void Info_DestroyProfile(struct profile_s *profile);

char *Info_ConvertTimeOfDayToTimestamp(
	int sec, int min, int hour,
	int day, int mon, int year);
void Info_ConvertTimestampToTimeOfDay(char *timestamp,
	int *dstsec, int *dstmin, int *dsthour,
	int *dstday, int *dstmon, int *dstyear);
void Info_GetTimeOfDay(int *dstsec, int *dstmin, int *dsthour,
	int *dstday, int *dstmon, int *dstyear);
time_t Info_ConvertTimestampToTimeT(char *timestamp);
time_t Info_ConvertTimeOfDayToTimeT(
	int sec, int min, int hour,
	int day, int mon, int year);
char *Info_GetCurrentTimestamp();
char *Info_GetCurrentTimestamp2();
char *Info_ConvertTimestampToLongDate(char *timestamp);
char *Info_ConvertTimestampToLongDateAndTime(char *timestamp);
char *Info_TimestampToLocalTime2(char *timestamp);
char *Info_GetLocalTime2();

void Info_24HourTo12Hour(int *hour, char *ampm);

char *Info_GetLocalTime();
char *Info_GetLocalTime2();
int Info_GetTimeDiffFromNow(time_t t1);
int Info_GetTimeDiffFromNowNeg(time_t t1);

void Info_DestroyTournamentPlayerInfo(struct tournamentplayerinfo_s *info);
struct tournamentplayerinfo_s *Info_DupeTournamentPlayerInfo(struct tournamentplayerinfo_s *src);

struct tournamentinfo_s *Info_DupeTournamentInfo(struct tournamentinfo_s *src);

void Info_DestroyTournamentPairing(struct tournamentpairing_s *pairing);
struct tournamentpairing_s *Info_DupeTournamentPairing(struct tournamentpairing_s *src);

struct namedlist_s *Info_DupeRatingList(struct namedlist_s *src);

char *Info_GetStandardRating(struct namedlist_s *ratinglist);
void Info_DestroyRating(struct rating_s *rating);

struct privacylistentry_s *Info_DupePrivacyListEntry(struct privacylistentry_s *src);
void Info_DestroyPrivacyListEntry(struct privacylistentry_s *entry);

void Info_DestroyTCPair(struct tcpair_s *tcp);

char *Info_GetLocalDate();

struct groupinfo_s *Info_DupeGroupInfo(struct groupinfo_s *src);
struct groupinfo_s *Info_DestroyGroupInfo(struct groupinfo_s *ginfo);

#endif
