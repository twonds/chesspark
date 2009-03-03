#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <mmsystem.h>

#include <strophe.h>

#include "constants.h"
#include "i18n.h"
#include "info.h"
#include "model.h"
#include "mp.h"
#include "log.h"
#include "options.h"
#include "namedlist.h"
#include "util.h"

#include "conn.h"
#include "leak.h"

void Conn_ParseTimeControl(xmpp_stanza_t *timecontrol, int *delayinc, int **controlarray);
xmpp_stanza_t *Conn_CreateTimeControl(int idelayinc, int *controlarray);
void Conn_GetTime(char *jid);

xmpp_ctx_t *ctx = NULL;
xmpp_conn_t *conn = NULL;

enum connstate_e ConnectState = CONN_NOTCONNECTED;
enum connerror_e ConnectError = CONNERROR_NONE;
unsigned int ConnCounter;
int keepalive = 0;
int intentionaldisconnect = 0;

void Conn_SetKeepAlive(int ka)
{
	keepalive = ka;
}
extern void Log_xmppLogCallback(void * const userdata,
	const xmpp_log_level_t level,
	const char * const area,
	const char * const msg);

static const xmpp_log_t CtrlLog = {&Log_xmppLogCallback, NULL};

const char Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

char *Int24ToBase64(int input)
{
	static char output[5];
	int i;

	for (i=0; i<4; i++)
	{
		int index = (input >> (6 * (3 - i))) & 0x3F;
		output[i] = Base64[index];
	}

	output[4] = '\0';

	return output;
}

void Conn_GenID(char *id, char *suffix)
{
	strcpy(id, Int24ToBase64((ConnCounter >> 24) & 0xFFFFFF));
	strcat(id, Int24ToBase64(ConnCounter & 0xFFFFFF));
	if (suffix)
	{
		strcat(id, ":");
		strcat(id, suffix);
	}
	ConnCounter += 1;
}

void RosterEntry_Destroy(struct rosterentry_s *entry)
{
	if (!entry)
	{
		return;
	}

	free(entry->jid);
	free(entry->name);
	NamedList_Destroy(&(entry->groups));
	free(entry);
};

void PresenceInfo_Destroy(struct presenceinfo_s *info)
{
	if (!info)
	{
		return;
	}

	free(info->jid);
	free(info->statusmsg);
	free(info->role);
	free(info->affiliation);
	free(info->realjid);
	free(info->vcardphotohash);
	free(info->nickchange);
	NamedList_Destroy2(info->roleslist);
	NamedList_Destroy2(info->titleslist);
	free(info->actorjid);
	free(info->reason);
	free(info->membertype);
	free(info);
}

void MessageInfo_Destroy(struct messageinfo_s *info)
{
	if (!info)
	{
		return;
	}

	free(info->jid);
	free(info->text);
	free(info->msgid);
	free(info->timestamp);
	free(info);
}
void Conn_Init()
{
	/* initialize libstrophe lib */
	xmpp_initialize();

	/* create a context */
	ctx = xmpp_ctx_new(NULL, &CtrlLog);

	ConnectState = CONN_NOTCONNECTED;

	/* initialize ConnCounter to a random number */
	ConnCounter = (rand() << 30) ^ (rand() << 15) ^ rand();
}


void Conn_Reset()
{
	keepalive = 0;

	if (conn) {
		xmpp_conn_release(conn);
		conn = NULL;
	}

	if (ctx) {
		xmpp_ctx_free(ctx);
		ctx = NULL;
	}

	/* create a context */
	ctx = xmpp_ctx_new(NULL, &CtrlLog);
	ConnectState = CONN_NOTCONNECTED;
}

void Conn_Disconnect()
{
	if (conn) {
		xmpp_disconnect(conn);
	}

	intentionaldisconnect = 1;

	ConnectState = CONN_DISCONNECTING;

	ConnCounter++;
}

xmpp_stanza_t *xmpp_stanza_get_next_by_name(xmpp_stanza_t * const stanza, const char * const name)
{
	xmpp_stanza_t *next = xmpp_stanza_get_next(stanza);

	while (next)
	{
		char *nextname = xmpp_stanza_get_name(next);

		if (nextname && strcmp(name, nextname) == 0)
		{
			return next;
		}

		next = xmpp_stanza_get_next(next);
	}

	return NULL;
}

xmpp_stanza_t *Conn_CreateTextStanza(char *name, char *text)
{
	xmpp_stanza_t *namestanza;

	namestanza = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(namestanza, name);

	if (text)
	{
		xmpp_stanza_t *textstanza = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(textstanza, text);

		xmpp_stanza_add_child(namestanza, textstanza);

		xmpp_stanza_release(textstanza);
	}

	return namestanza;
}

struct gamesearchinfo_s *Conn_ParseGameRequest(xmpp_stanza_t *gamerequest)
{
	xmpp_stanza_t *variant         = xmpp_stanza_get_child_by_name(gamerequest, "variant");
	xmpp_stanza_t *colorpreference = xmpp_stanza_get_child_by_name(gamerequest, "color-preference");
	xmpp_stanza_t *timecontrol     = xmpp_stanza_get_child_by_name(gamerequest, "time-control");
	xmpp_stanza_t *rated           = xmpp_stanza_get_child_by_name(gamerequest, "rated");
	xmpp_stanza_t *comment         = xmpp_stanza_get_child_by_name(gamerequest, "comment");
	xmpp_stanza_t *correspondence  = xmpp_stanza_get_child_by_name(gamerequest, "correspondence");
	xmpp_stanza_t *gamereconvene   = xmpp_stanza_get_child_by_name(gamerequest, "game-reconvene");
	xmpp_stanza_t *takebacks       = xmpp_stanza_get_child_by_name(gamerequest, "takebacks");

	char *gameid = xmpp_stanza_get_attribute(gamerequest, "id");
	
	struct gamesearchinfo_s *info = malloc(sizeof(*info));
				
	memset(info, 0, sizeof(*info));

	if (gameid)
	{
		info->gameid = strdup(gameid);
	}

/*
<game-reconvene>
 <current-time-clock side="white">868</current-time-clock>
 <current-time-clock side="black">884</current-time-clock>
 <current-moves>2</current-moves>
 <player side="white">jcanete@chesspark.com/cpc</player>
 <player side="black">test@chesspark.com/laptop</player>
 <current-moves>2</current-moves>
 <current-side>black</current-side></game-reconvene>
*/
	if (gamereconvene)
	{
		xmpp_stanza_t *currenttimeclock = xmpp_stanza_get_child_by_name(gamereconvene, "current-time-clock");
		xmpp_stanza_t *currentmoves     = xmpp_stanza_get_child_by_name(gamereconvene, "current-moves");
		xmpp_stanza_t *player           = xmpp_stanza_get_child_by_name(gamereconvene, "player");
		xmpp_stanza_t *currentside      = xmpp_stanza_get_child_by_name(gamereconvene, "currentside");

		if (currentside)
		{
			char *side = xmpp_stanza_get_text(currentside);

			if (stricmp(side, "white") == 0)
			{
				info->whitetomove = 1;
			}
			else if (stricmp(side, "black") == 0)
			{
				info->blacktomove = 1;
			}
		}

		while (currenttimeclock)
		{
			char *side = xmpp_stanza_get_attribute(currenttimeclock, "side");

			if (stricmp(side, "white") == 0)
			{
				sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->whiteclock));
			}
			else if (stricmp(side, "black") == 0)
			{
				sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->blackclock));
			}

			currenttimeclock = xmpp_stanza_get_next_by_name(currenttimeclock, "current-time-clock");
		}

		while (player)
		{
			char *side = xmpp_stanza_get_attribute(player, "side");

			struct playerinfo_s *pinfo = malloc(sizeof(*(pinfo)));
			memset(pinfo, 0, sizeof(*(pinfo)));
	
			pinfo->jid = strdup(xmpp_stanza_get_text(player));

			if (stricmp(side, "white") == 0)
			{
				info->white = pinfo;
			}
			else if (stricmp(side, "black") == 0)
			{
				info->black = pinfo;
			}

			player = xmpp_stanza_get_next_by_name(player, "player");
		}

		if (currentmoves)
		{
			sscanf(xmpp_stanza_get_text(currentmoves), "%d", &(info->movenum));
		}
	}

	if (colorpreference)
	{
		char *color = xmpp_stanza_get_attribute(colorpreference, "color");
		if (color && strcmp(color, "white") == 0)
		{
			info->colorpreference = 1;
		}
		else if (color && strcmp(color, "black") == 0)
		{
			info->colorpreference = 2;
		}
	}

	if (variant)
	{
		char *variantname = xmpp_stanza_get_attribute(variant, "name");

		if (!variantname || strlen(variantname) == 0)
		{
			variantname = xmpp_stanza_get_text(variant);
		}

		info->variant = strdup(variantname);
	}

	while (timecontrol)
	{
		char *side = xmpp_stanza_get_attribute(timecontrol, "side");
		struct timecontrol_s *tc;
		
		tc = malloc(sizeof(*tc));
		memset(tc, 0, sizeof(*tc));
		Conn_ParseTimeControl(timecontrol, &(tc->delayinc), &(tc->controlarray));					
		if (correspondence)
		{
			tc->correspondence = 1;
		}

		if (!side || (side && stricmp(side, "white") == 0) || (side && stricmp(side, "both") == 0))
		{
			info->timecontrol = tc;
		}
		else
		{
			info->blacktimecontrol = tc;
		}

		timecontrol = xmpp_stanza_get_next_by_name(timecontrol, "time-control");
	}

	if (comment)
	{
		info->comment = strdup(xmpp_stanza_get_text(comment));
	}

	if (rated)
	{
		info->rated = 1;
	}

	if (correspondence)
	{
		info->correspondence = 1;
	}

	if (takebacks)
	{
		info->takebacks = strdup(xmpp_stanza_get_text(takebacks));
	}

	return info;
}

void Conn_HandleGameRequestError(xmpp_stanza_t *stanza, char *opponentjid, char *gameid, char *adid)
{
	xmpp_stanza_t *gamerequest = xmpp_stanza_get_child_by_name(stanza, "game-request");
	xmpp_stanza_t *error       = xmpp_stanza_get_child_by_name(stanza, "error");
	xmpp_stanza_t *membernotonline      = xmpp_stanza_get_child_by_name(stanza, "member-not-online");
	xmpp_stanza_t *memberplaying        = xmpp_stanza_get_child_by_name(stanza, "member-playing");
	xmpp_stanza_t *memberalreadyplaying = xmpp_stanza_get_child_by_name(stanza, "member-already-playing");
	xmpp_stanza_t *text                 = xmpp_stanza_get_child_by_name(stanza, "text");
	char *jid = NULL;

	if (text)
	{
		jid = xmpp_stanza_get_text(text);
	}

	if (gamerequest)
	{
		xmpp_stanza_t *gamerejected    = xmpp_stanza_get_child_by_name(gamerequest, "game-rejected");

		gameid = xmpp_stanza_get_attribute(gamerequest, "id");

		if (membernotonline)
		{
			if (!jid)
			{
				jid = xmpp_stanza_get_text(membernotonline);
			}

			ModelConn_GameRequestError(opponentjid, gameid, "offline", jid, adid);

			return;
		}
		else if (gamerejected)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "reject", jid, adid);

			return;
		}
	}
/*
0[20070814T16:43:11.593]: (xmpp) RECV: 
<iq id="AADBLcHu:requestmatch" xmlns="jabber:client" type="error" to="jcanete@chesspark.com/cpc" from="match.chesspark.com">
<iq id="AADBLcHu:requestmatch" type="set" to="match.chesspark.com" from="jcanete@chesspark.com/cpc"><game-request xmlns="http://onlinegamegroup.com/xml/chesspark-01" to="use.less01@gmail.com"><time-control delayinc="-5"><control><time>1800</time></control></time-control></game-request></iq>
<error type="error">
<item-not-found xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/>
<membership-expired xmlns="http://onlinegamegroup.com/xml/chesspark-01"/><text xmlns="urn:ietf:params:xml:ns:xmpp-stanzas">use.less01@gmail.com</text></error></iq>
*/
	if (error)
	{
		xmpp_stanza_t *badrequest           = xmpp_stanza_get_child_by_name(error, "bad-request");
		xmpp_stanza_t *membernotonline      = xmpp_stanza_get_child_by_name(error, "member-not-online");
		xmpp_stanza_t *memberplaying        = xmpp_stanza_get_child_by_name(error, "member-playing");
		xmpp_stanza_t *memberalreadyplaying = xmpp_stanza_get_child_by_name(error, "member-already-playing");
		xmpp_stanza_t *memberexpired        = xmpp_stanza_get_child_by_name(error, "membership-expired");
		xmpp_stanza_t *internalserviceerror = xmpp_stanza_get_child_by_name(error, "internal-service-error");
		xmpp_stanza_t *notfound             = xmpp_stanza_get_child_by_name(error, "not-found");
		xmpp_stanza_t *text                 = xmpp_stanza_get_child_by_name(error, "text");
		xmpp_stanza_t *notamember           = xmpp_stanza_get_child_by_name(error, "not-a-member");
		xmpp_stanza_t *notapro              = xmpp_stanza_get_child_by_name(error, "not-a-pro");

		if (text)
		{
			jid = xmpp_stanza_get_text(text);
		}


		if (membernotonline)
		{
			if (!jid)
			{
				jid = xmpp_stanza_get_text(membernotonline);
			}
/*
			if (jid && strlen(jid) != 0)
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1 is offline."), Model_GetFriendNick(jid));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\nMember is offline."));
			}
*/
			ModelConn_GameRequestError(opponentjid, gameid, "offline", jid, adid);
		}
		else if (memberplaying)
		{
			if (!jid)
			{
				jid = xmpp_stanza_get_text(memberplaying);
			}
/*
			if (jid && strlen(jid) != 0)
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1 is playing a game."), Model_GetFriendNick(jid));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\nMember is playing a game."));
			}
*/
			ModelConn_GameRequestError(opponentjid, gameid, "playing", jid, adid);
		}
		else if (memberalreadyplaying)
		{
			if (!jid)
			{
				jid = xmpp_stanza_get_text(memberalreadyplaying);
			}
/*
			if (jid && strlen(jid) != 0)
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\n%1 is already playing a game."), Model_GetFriendNick(jid));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Game invite cancelled.\nMember is already playing a game."));
			}
*/

			ModelConn_GameRequestError(opponentjid, gameid, "playing", jid, adid);
		}
		else if (memberexpired)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "expired", jid, adid);
		}
		else if (notfound)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "notfound", jid, adid);
		}
		else if (notamember)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "notamember", jid, adid);
		}
		else if (notapro)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "notapro", jid, adid);
		}
		else if (badrequest)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "badrequest", NULL, adid);
		}
		else if (internalserviceerror)
		{
			ModelConn_GameRequestError(opponentjid, gameid, "internalservice", NULL, adid);
		}
		else
		{
			ModelConn_GameRequestError(opponentjid, gameid, xmpp_stanza_get_text(error), NULL, adid);
		}
	}
	else
	{
		ModelConn_GameRequestError(opponentjid, gameid, "unknown", NULL, adid);
	}
}

void Conn_HandleLoginMessage(xmpp_stanza_t *stanza)
{
        xmpp_stanza_t *game =         xmpp_stanza_get_child_by_name(stanza, "game");
        xmpp_stanza_t *login =        xmpp_stanza_get_child_by_name(stanza, "login");
	xmpp_stanza_t *upgrade =      xmpp_stanza_get_child_by_name(stanza, "upgrade");
	xmpp_stanza_t *error =        xmpp_stanza_get_child_by_name(stanza, "error");
	xmpp_stanza_t *memberlogout = xmpp_stanza_get_child_by_name(stanza, "member-logout");

	if (upgrade)
	{
		char *number   = xmpp_stanza_get_attribute(upgrade, "number");
		char *url      = xmpp_stanza_get_attribute(upgrade, "url");
		char *build    = xmpp_stanza_get_attribute(upgrade, "build");
		char *version  = xmpp_stanza_get_attribute(upgrade, "version");
		Model_HandleClientUpgrade(number, version, build, url);
	}

	if (memberlogout)
	{
		ModelConn_ShowGameLogoutMessage("login.chesspark.com");
	}

	if (error)
	{
		xmpp_stanza_t *memberexpired =        xmpp_stanza_get_child_by_name(error, "membership-expired");
		xmpp_stanza_t *membernotfound =       xmpp_stanza_get_child_by_name(error, "member-not-found");
		xmpp_stanza_t *notamember =           xmpp_stanza_get_child_by_name(error, "not-a-member");
		xmpp_stanza_t *memberloginsexceeded = xmpp_stanza_get_child_by_name(error, "member-logins-exceeded");
		xmpp_stanza_t *notloggedin =          xmpp_stanza_get_child_by_name(error, "not-logged-in");
		xmpp_stanza_t *membercancelled =      xmpp_stanza_get_child_by_name(error, "membership-canceled");

		Log_Write(0, "membercancelled %d\n", membercancelled);

		if (memberexpired)
		{
			ModelConn_ShowMemberExpired();
		}

		if (membernotfound || notamember)
		{
			ModelConn_ShowMemberNotFound();
		}

		if (memberloginsexceeded)
		{
			ModelConn_ShowMemberLoginsExceeded();
		}

		if (membercancelled)
		{
			ModelConn_ShowMemberCancelled();
		}
	}

	while (game)
	{
                char *gametype = xmpp_stanza_get_attribute(game, "type");
		char *count = xmpp_stanza_get_attribute(game, "count");

		if (gametype && count)
		{
			if (stricmp(gametype, "adjourned") == 0)
			{
				int adjourned;

				sscanf(count, "%d", &adjourned);

				ModelConn_SetAdjournedCount(adjourned);
			}
			else if (stricmp(gametype, "correspondence") == 0)
			{
				int correspondence;

				sscanf(count, "%d", &correspondence);

				ModelConn_SetCorrespondenceCount(correspondence);
			}
			else if (stricmp(gametype, "playing") == 0)
			{
				int playing;

				sscanf(count, "%d", &playing);

				ModelConn_HandleTotalGamesCount(playing);
			}
		}
		game = xmpp_stanza_get_next_by_name(game, "game");
	}

	if (login)
	{
		xmpp_stanza_t *permissions =  xmpp_stanza_get_child_by_name(login, "permissions");
		xmpp_stanza_t *expire =       xmpp_stanza_get_child_by_name(login, "expire");
	        xmpp_stanza_t *rooms =        xmpp_stanza_get_child_by_name(login, "rooms");
		xmpp_stanza_t *membertype =   xmpp_stanza_get_child_by_name(login, "member-type");
		xmpp_stanza_t *notactivated = xmpp_stanza_get_child_by_name(login, "not-activated");
		struct namedlist_s *permissionlist = NULL, *roomslist = NULL;
		char *membertypetext = NULL;

		if (permissions)
		{
			xmpp_stanza_t *feature = xmpp_stanza_get_child_by_name(permissions, "feature");
			while (feature)
			{
				char *featurename = xmpp_stanza_get_attribute(feature, "type");

				NamedList_AddString(&permissionlist, featurename, NULL);

				feature = xmpp_stanza_get_next_by_name(feature, "feature");
			}
		}

		if (rooms)
		{
			xmpp_stanza_t *room = xmpp_stanza_get_child_by_name(rooms, "room");
			while (room)
			{
				char *roomname = xmpp_stanza_get_text(room);

				NamedList_AddString(&roomslist, roomname, NULL);
				room = xmpp_stanza_get_next_by_name(room, "room");
			}
		}

		if (expire)
		{
			char *expiredate = xmpp_stanza_get_attribute(expire, "expire");
			char *expiretype = xmpp_stanza_get_attribute(expire, "type");

			/*Model_SetChessparkExpiry(expiretype, expiredate);*/
		}

		if (membertype)
		{
			membertypetext = xmpp_stanza_get_text(membertype);
		}

		ModelConn_OnChessparkLogin(membertypetext, permissionlist, roomslist, notactivated != NULL);
	}
}


struct gamesearchinfo_s *Conn_ParseGameSearchInfo(xmpp_stanza_t *game)
{
	xmpp_stanza_t *variant          = xmpp_stanza_get_child_by_name(game, "variant");
	xmpp_stanza_t *timecontrol      = xmpp_stanza_get_child_by_name(game, "time-control");
	xmpp_stanza_t *rated            = xmpp_stanza_get_child_by_name(game, "rated");
	xmpp_stanza_t *timecontrolrange = xmpp_stanza_get_child_by_name(game, "time-control-range");
	xmpp_stanza_t *computer         = xmpp_stanza_get_child_by_name(game, "computer");
	xmpp_stanza_t *limit            = xmpp_stanza_get_child_by_name(game, "limit");
	xmpp_stanza_t *colorpreference  = xmpp_stanza_get_child_by_name(game, "color-preference");
	xmpp_stanza_t *state            = xmpp_stanza_get_child_by_name(game, "state");
	xmpp_stanza_t *player           = xmpp_stanza_get_child_by_name(game, "player");
	xmpp_stanza_t *comment          = xmpp_stanza_get_child_by_name(game, "comment");
	xmpp_stanza_t *currentmove      = xmpp_stanza_get_child_by_name(game, "current-move");
	xmpp_stanza_t *currenttimeclock = xmpp_stanza_get_child_by_name(game, "current-time-clock");
	xmpp_stanza_t *movenumber       = xmpp_stanza_get_child_by_name(game, "move-number");
	xmpp_stanza_t *currentside      = xmpp_stanza_get_child_by_name(game, "current_side");
	xmpp_stanza_t *correspondence   = xmpp_stanza_get_child_by_name(game, "correspondence");
	xmpp_stanza_t *offline          = xmpp_stanza_get_child_by_name(game, "offline");
	xmpp_stanza_t *rating           = xmpp_stanza_get_child_by_name(game, "rating");
	xmpp_stanza_t *relativerating   = xmpp_stanza_get_child_by_name(game, "relative-rating");
	xmpp_stanza_t *filterunrated    = xmpp_stanza_get_child_by_name(game, "filter-unrated");
	xmpp_stanza_t *hideown          = xmpp_stanza_get_child_by_name(game, "hide-own");
	xmpp_stanza_t *showonlymine     = xmpp_stanza_get_child_by_name(game, "show-only-mine");
	xmpp_stanza_t *takebacks        = xmpp_stanza_get_child_by_name(game, "takebacks");
	xmpp_stanza_t *groups           = xmpp_stanza_get_child_by_name(game, "groups");
	char *date = xmpp_stanza_get_attribute(game, "date");
	char *filteropen = xmpp_stanza_get_attribute(game, "open");
	char *quickapply = xmpp_stanza_get_attribute(game, "apply");
	struct gamesearchinfo_s *info = malloc(sizeof(*info));

	memset(info, 0, sizeof(*info));

	if (date && date[0] != '\0')
	{
		info->date = strdup(date);
	}

	if (rating)
	{
		info->timecontrolrange = strdup(xmpp_stanza_get_text(rating));
	}

	if (currentmove)
	{
		info->lastmove = strdup(xmpp_stanza_get_text(currentmove));
	}

	if (movenumber)
	{
		sscanf(xmpp_stanza_get_text(movenumber), "%d", &(info->movenum));
	}

	if (currentside)
	{
		char *side = xmpp_stanza_get_text(currentside);

		if (stricmp(side, "white") == 0)
		{
			info->whitetomove = 1;
		}
		else if (stricmp(side, "black") == 0)
		{
			info->blacktomove = 1;
		}
	}

	while (currenttimeclock)
	{
		char *side = xmpp_stanza_get_attribute(currenttimeclock, "side");

		if (stricmp(side, "white") == 0)
		{
			sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->whiteclock));
		}
		else if (stricmp(side, "black") == 0)
		{
			sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->blackclock));
		}

		currenttimeclock = xmpp_stanza_get_next_by_name(currenttimeclock, "current-time-clock");
	}

	while (player)
	{
		xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(player, "rating");
		xmpp_stanza_t *titles = xmpp_stanza_get_child_by_name(player, "titles");
		xmpp_stanza_t *roles  = xmpp_stanza_get_child_by_name(player, "roles");
		xmpp_stanza_t *notactivated = xmpp_stanza_get_child_by_name(player, "not-activated");
		xmpp_stanza_t *membertype   = xmpp_stanza_get_child_by_name(player, "member-type");

		char *side = xmpp_stanza_get_attribute(player, "side");

		struct playerinfo_s *pinfo = malloc(sizeof(*(pinfo)));
		memset(pinfo, 0, sizeof(*(pinfo)));

		pinfo->jid = strdup(xmpp_stanza_get_attribute(player, "jid"));

		if (rating)
		{
			pinfo->rating = strdup(xmpp_stanza_get_text(rating));
		}

		if (roles)
		{
			xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(roles, "role");

			while (role)
			{
				NamedList_AddString(&(pinfo->roles), xmpp_stanza_get_text(role), NULL);
				role = xmpp_stanza_get_next_by_name(role, "role");
			}
		}

		if (titles)
		{
			xmpp_stanza_t *title = xmpp_stanza_get_child_by_name(titles, "title");

			while (title)
			{
				NamedList_AddString(&(pinfo->titles), xmpp_stanza_get_text(title), NULL);
				title = xmpp_stanza_get_next_by_name(title, "title");
			}
		}

		if (notactivated)
		{
			pinfo->notactivated = 1;
		}

		if (membertype)
		{
			pinfo->membertype = strdup(xmpp_stanza_get_text(membertype));
		}

		if (side && stricmp(side, "white") == 0)
		{
			info->white = pinfo;
		}
		else if (side && stricmp(side, "black") == 0)
		{
			info->black = pinfo;
		}
		else
		{
			info->adplayer = pinfo;
		}

		player = xmpp_stanza_get_next_by_name(player, "player");
	}

	if (variant)
	{
		char *variantname = xmpp_stanza_get_attribute(variant, "name");

		if (!variantname || strlen(variantname) == 0)
		{
			variantname = xmpp_stanza_get_text(variant);
		}

		if (strlen(variantname) == 0)
		{
			variantname = NULL;
		}

		info->variant = strdup(variantname);
	}

	while (timecontrol)
	{
		struct timecontrol_s *tc;
		char *side = xmpp_stanza_get_attribute(timecontrol, "side");

		tc = malloc(sizeof(*tc));
		memset(tc, 0, sizeof(*tc));

		Conn_ParseTimeControl(timecontrol, &(tc->delayinc), &(tc->controlarray));

		if (!side || stricmp(side, "white") == 0 || stricmp(side, "both") == 0)
		{
			info->timecontrol = tc;
		}
		else if (stricmp(side, "black") == 0)
		{
			info->blacktimecontrol = tc;
		}

		timecontrol = xmpp_stanza_get_next_by_name(timecontrol, "time-control");
	}

	if (timecontrolrange)
	{
		char *timecontrolrangename = xmpp_stanza_get_attribute(timecontrolrange, "name");

		if (!timecontrolrangename || strlen(timecontrolrangename) == 0)
		{
			timecontrolrangename = xmpp_stanza_get_text(timecontrolrange);
		}

		if (strlen(timecontrolrangename) == 0)
		{
			timecontrolrangename = NULL;
		}

		info->timecontrolrange = strdup(timecontrolrangename);
	}

	if (rated)
	{
		info->rated = TRUE;
	}

	if (computer)
	{
		info->computers = TRUE;
	}

	if (limit)
	{
		char *low = xmpp_stanza_get_attribute(limit, "low");
		char *high = xmpp_stanza_get_attribute(limit, "high");
		info->limit = malloc(sizeof(*(info->limit)));

		info->limit->type = strdup(xmpp_stanza_get_attribute(limit, "type"));
		info->limit->low = -1;
		info->limit->high = -1;

		if (low)
		{
			sscanf(low, "%d", &(info->limit->low));
		}

		if (high)
		{
			sscanf(high, "%d", &(info->limit->high));		
		}

		if ((low || high) & !info->limit->type)
		{
			info->limit->type = strdup("rating");
		}
	}

	if (relativerating)
	{
		sscanf(xmpp_stanza_get_text(relativerating), "%d", &(info->relativerating));
	}

	if (filterunrated)
	{
		info->filterunrated = 1;
	}

	if (colorpreference)
	{
		char *color = xmpp_stanza_get_attribute(colorpreference, "color");

                if (color && stricmp(color, "white") == 0)				
		{	
			info->colorpreference = 1;				
		}
		else if (color && stricmp(color, "black") == 0)
		{
			info->colorpreference = 2;
		}
	}

	if (state)
	{
		xmpp_stanza_t *whiteclock = xmpp_stanza_get_child_by_name(state, "white-clock");
		xmpp_stanza_t *blackclock = xmpp_stanza_get_child_by_name(state, "black-clock");
						
		if (whiteclock)
		{
			sscanf(xmpp_stanza_get_text(whiteclock), "%f", &(info->whiteclock));
		}

		if (blackclock)
		{
			sscanf(xmpp_stanza_get_text(blackclock), "%f", &(info->blackclock));
		}
	}

	if (comment)
	{
		info->comment = strdup(xmpp_stanza_get_text(comment));
	}

	if (correspondence)
	{
		if (info->timecontrol)
		{
			info->timecontrol->correspondence = 1;
		}
		info->correspondence = 1;
	}

	if (offline)
	{
		info->offline = 1;
	}

	if (filteropen && stricmp(filteropen, "yes") == 0)
	{
		info->filteropen = 1;
	}

	if (quickapply && stricmp(quickapply, "yes") == 0)
	{
		info->quickapply = 1;
	}

	if (hideown)
	{
		info->hideown = 1;
	}

	if (showonlymine)
	{
		info->showonlymine = 1;
	}

	if (takebacks)
	{
		info->takebacks = strdup(xmpp_stanza_get_text(takebacks));
	}

	if (groups)
	{
		xmpp_stanza_t *group = xmpp_stanza_get_child_by_name(groups, "group");

		while (group)
		{
			char *groupid = xmpp_stanza_get_text(group);
			NamedList_AddString(&(info->groupids), groupid, NULL);

			group = xmpp_stanza_get_next_by_name(groups, "group");
		}
	}

	return info;
}

void Conn_AddGameSearchInfo(xmpp_stanza_t *filter, struct gamesearchinfo_s *info)
{
	if (info->colorpreference)
	{
		xmpp_stanza_t *colorpreference = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(colorpreference, "color-preference");
		if (info->colorpreference == 1)
		{
			xmpp_stanza_set_attribute(colorpreference, "color", "white");
		}
		else if (info->colorpreference == 2)
		{
			xmpp_stanza_set_attribute(colorpreference, "color", "black");
		}

		xmpp_stanza_add_child(filter, colorpreference);

		xmpp_stanza_release(colorpreference);

	}

	if (info->variant)
	{
		xmpp_stanza_t *variant;

		variant = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(variant, "variant");
		xmpp_stanza_set_attribute(variant, "name", info->variant);
		
		xmpp_stanza_add_child(filter, variant);

		xmpp_stanza_release(variant);

	}

	if (info->comment)
	{
		xmpp_stanza_t *comment = xmpp_stanza_new(ctx);
		xmpp_stanza_t *commenttext = xmpp_stanza_new(ctx);
		char *text = EscapeParsedText(info->comment);

		xmpp_stanza_set_name(comment, "comment");
		xmpp_stanza_set_text(commenttext, text);

		xmpp_stanza_add_child(comment, commenttext);

		xmpp_stanza_release(commenttext);

		xmpp_stanza_add_child(filter, comment);

		xmpp_stanza_release(comment);
		free(text);
	}

	if (info->keywords)
	{
		xmpp_stanza_t *keywords, *keywordstext;
		char *text = EscapeParsedText(info->keywords);

		keywords = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(keywords, "keywords");

		keywordstext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(keywordstext, text);

		xmpp_stanza_add_child(keywords, keywordstext);
		
		xmpp_stanza_release(keywordstext);
		
		xmpp_stanza_add_child(filter, keywords);

		xmpp_stanza_release(keywords);

		free(text);
	}

	if (info->rated)
	{
		xmpp_stanza_t *rated;

		rated = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(rated, "rated");

		xmpp_stanza_add_child(filter, rated);

		xmpp_stanza_release(rated);
	}		

	if (info->computers)
	{
		xmpp_stanza_t *computer;

		computer = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(computer, "computer");

		xmpp_stanza_add_child(filter, computer);

		xmpp_stanza_release(computer);
	}

	if (info->timecontrol)
	{
		if (info->blacktimecontrol)
		{
			xmpp_stanza_t *timecontrol;

			timecontrol = Conn_CreateTimeControl(info->timecontrol->delayinc, info->timecontrol->controlarray);

			xmpp_stanza_set_attribute(timecontrol, "side", "white");

			xmpp_stanza_add_child(filter, timecontrol);

			xmpp_stanza_release(timecontrol);

			timecontrol = Conn_CreateTimeControl(info->blacktimecontrol->delayinc, info->blacktimecontrol->controlarray);

			xmpp_stanza_set_attribute(timecontrol, "side", "black");

			xmpp_stanza_add_child(filter, timecontrol);

			xmpp_stanza_release(timecontrol);
		}
		else
		{
			xmpp_stanza_t *timecontrol = Conn_CreateTimeControl(info->timecontrol->delayinc, info->timecontrol->controlarray);

			xmpp_stanza_add_child(filter, timecontrol);

			xmpp_stanza_release(timecontrol);
		}
	}

	if (info->timecontrolrange)
	{
		xmpp_stanza_t *timecontrolrange;

		timecontrolrange = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(timecontrolrange, "time-control-range");

		xmpp_stanza_set_attribute(timecontrolrange, "name", info->timecontrolrange);

		xmpp_stanza_add_child(filter, timecontrolrange);

		xmpp_stanza_release(timecontrolrange);

	}

	if (info->titled)
	{
		xmpp_stanza_t *titled;

		titled = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(titled, "titled");

		xmpp_stanza_add_child(filter, titled);

		xmpp_stanza_release(titled);
	}

	if (info->limit)
	{
		xmpp_stanza_t *limit;

		limit = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(limit, "limit");

		xmpp_stanza_set_attribute(limit, "type", info->limit->type);

		if (info->limit->low != -1)
		{
			char txt[120];

			sprintf(txt, "%d", info->limit->low);
			xmpp_stanza_set_attribute(limit, "low", txt);
		}

		if (info->limit->high != -1)
		{
			char txt[120];

			sprintf(txt, "%d", info->limit->high);
			xmpp_stanza_set_attribute(limit, "high", txt);
		}

		xmpp_stanza_add_child(filter, limit);

		xmpp_stanza_release(limit);
	}

	if (info->relativerating)
	{
		xmpp_stanza_t *relativerating;
		char txt[20];

		sprintf(txt, "%d", info->relativerating);

		relativerating = Conn_CreateTextStanza("relative-rating", txt);

		xmpp_stanza_add_child(filter, relativerating);
		xmpp_stanza_release(relativerating);
	}

	if (info->filterunrated)
	{
		xmpp_stanza_t *filterunrated;

		filterunrated = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(filterunrated, "filter-unrated");

		xmpp_stanza_add_child(filter, filterunrated);
		xmpp_stanza_release(filterunrated);
	}

	if (info->pairingtype)
	{
		xmpp_stanza_t *pairing, *pairingtext;

		pairing = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pairing, "pairing");

		pairingtext = xmpp_stanza_new(ctx);

		if (stricmp(info->pairingtype, "Swiss Pairing") == 0)
		{
                        xmpp_stanza_set_text(pairingtext, "s");
		}
		else if (stricmp(info->pairingtype, "Round Robin") == 0)
		{
                        xmpp_stanza_set_text(pairingtext, "r");
		}
		else if (stricmp(info->pairingtype, "Elimination") == 0)
		{
                        xmpp_stanza_set_text(pairingtext, "e");
		}

		xmpp_stanza_add_child(pairing, pairingtext);
		
		xmpp_stanza_release(pairingtext);
		
		xmpp_stanza_add_child(filter, pairing);

		xmpp_stanza_release(pairing);
	}

	/*
	if (info->correspondence)
	{
		xmpp_stanza_t *correspondence;

		correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");

		xmpp_stanza_add_child(filter, correspondence);

		xmpp_stanza_release(correspondence);
	}
	*/

	if (info->takebacks)
	{
		xmpp_stanza_t *takebacks = Conn_CreateTextStanza("takebacks", info->takebacks);

		xmpp_stanza_add_child(filter, takebacks);
		xmpp_stanza_release(takebacks);
	}

	if (info->groupids)
	{
		xmpp_stanza_t *groups = xmpp_stanza_new(ctx);
		struct namedlist_s *entry;

		xmpp_stanza_set_name(groups, "groups");

		entry = info->groupids;
		while (entry)
		{
			xmpp_stanza_t *group = Conn_CreateTextStanza("group", entry->name);

			xmpp_stanza_add_child(groups, group);
			xmpp_stanza_release(group);

			entry = entry->next;
		}

		xmpp_stanza_add_child(filter, groups);
		xmpp_stanza_release(groups);
	}
}

struct gamesearchinfo_s *Conn_ParseGameReconvene(xmpp_stanza_t *game)
{
	xmpp_stanza_t *variant          = xmpp_stanza_get_child_by_name(game, "variant");
	xmpp_stanza_t *timecontrol      = xmpp_stanza_get_child_by_name(game, "time-control");
	xmpp_stanza_t *rated            = xmpp_stanza_get_child_by_name(game, "rated");
	xmpp_stanza_t *timecontrolrange = xmpp_stanza_get_child_by_name(game, "time-control-range");
	xmpp_stanza_t *computer         = xmpp_stanza_get_child_by_name(game, "computer");
	xmpp_stanza_t *limit            = xmpp_stanza_get_child_by_name(game, "limit");
	xmpp_stanza_t *colorpreference  = xmpp_stanza_get_child_by_name(game, "color-preference");
	xmpp_stanza_t *state            = xmpp_stanza_get_child_by_name(game, "state");
	xmpp_stanza_t *player           = xmpp_stanza_get_child_by_name(game, "player");
	xmpp_stanza_t *comment          = xmpp_stanza_get_child_by_name(game, "comment");
	xmpp_stanza_t *currentmove      = xmpp_stanza_get_child_by_name(game, "current-move");
	xmpp_stanza_t *currenttimeclock = xmpp_stanza_get_child_by_name(game, "current-time-clock");
	xmpp_stanza_t *movenumber       = xmpp_stanza_get_child_by_name(game, "move-number");
	xmpp_stanza_t *currentside      = xmpp_stanza_get_child_by_name(game, "current_side");
	xmpp_stanza_t *correspondence   = xmpp_stanza_get_child_by_name(game, "correspondence");
	xmpp_stanza_t *offline          = xmpp_stanza_get_child_by_name(game, "offline");
	xmpp_stanza_t *rating           = xmpp_stanza_get_child_by_name(game, "rating");
	struct gamesearchinfo_s *info = malloc(sizeof(*info));

	memset(info, 0, sizeof(*info));

	if (rating)
	{
		info->timecontrolrange = strdup(xmpp_stanza_get_text(rating));
	}

	if (currentmove)
	{
		info->lastmove = strdup(xmpp_stanza_get_text(currentmove));
	}

	if (movenumber)
	{
		sscanf(xmpp_stanza_get_text(movenumber), "%d", &(info->movenum));
	}

	if (currentside)
	{
		char *side = xmpp_stanza_get_text(currentside);

		if (stricmp(side, "white") == 0)
		{
			info->whitetomove = 1;
		}
		else if (stricmp(side, "black") == 0)
		{
			info->blacktomove = 1;
		}
	}

	while (currenttimeclock)
	{
		char *side = xmpp_stanza_get_attribute(currenttimeclock, "side");

		if (stricmp(side, "white") == 0)
		{
			sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->whiteclock));
		}
		else if (stricmp(side, "black") == 0)
		{
			sscanf(xmpp_stanza_get_text(currenttimeclock), "%f", &(info->blackclock));
		}

		currenttimeclock = xmpp_stanza_get_next_by_name(currenttimeclock, "current-time-clock");
	}

	while (player)
	{
		xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(player, "rating");
		xmpp_stanza_t *titles = xmpp_stanza_get_child_by_name(player, "titles");
		xmpp_stanza_t *roles  = xmpp_stanza_get_child_by_name(player, "roles");
		char *side = xmpp_stanza_get_attribute(player, "side");

		struct playerinfo_s *pinfo = malloc(sizeof(*(pinfo)));
		memset(pinfo, 0, sizeof(*(pinfo)));

		pinfo->jid = strdup(xmpp_stanza_get_attribute(player, "jid"));

		if (rating)
		{
			pinfo->rating = strdup(xmpp_stanza_get_text(rating));
		}

		if (roles)
		{
			xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(roles, "role");

			while (role)
			{
				NamedList_AddString(&(pinfo->roles), xmpp_stanza_get_text(role), NULL);
				role = xmpp_stanza_get_next_by_name(role, "role");
			}
		}

		if (titles)
		{
			xmpp_stanza_t *title = xmpp_stanza_get_child_by_name(titles, "title");

			while (title)
			{
				NamedList_AddString(&(pinfo->titles), xmpp_stanza_get_text(title), NULL);
				title = xmpp_stanza_get_next_by_name(title, "title");
			}
		}

		if (side && stricmp(side, "white") == 0)
		{
			info->white = pinfo;
		}
		else if (side && stricmp(side, "black") == 0)
		{
			info->black = pinfo;
		}
		else
		{
			info->adplayer = pinfo;
		}

		player = xmpp_stanza_get_next_by_name(player, "player");
	}

	if (variant)
	{
		char *variantname = xmpp_stanza_get_attribute(variant, "name");

		if (!variantname || strlen(variantname) == 0)
		{
			variantname = xmpp_stanza_get_text(variant);
		}

		info->variant = strdup(variantname);
	}

	while (timecontrol)
	{
		struct timecontrol_s *tc;
		char *side = xmpp_stanza_get_attribute(timecontrol, "side");

		tc = malloc(sizeof(*tc));
		memset(tc, 0, sizeof(*tc));
		Conn_ParseTimeControl(timecontrol, &(tc->delayinc), &(tc->controlarray));

		if (!side || stricmp(side, "white") == 0)
		{
			info->timecontrol = tc;
		}
		else if (stricmp(side, "black") == 0)
		{
			info->blacktimecontrol = tc;
		}

		timecontrol = xmpp_stanza_get_next_by_name(timecontrol, "time-control");
	}

	if (timecontrolrange)
	{
		info->timecontrolrange = strdup(xmpp_stanza_get_attribute(timecontrolrange, "name"));
	}

	if (rated)
	{
		info->rated = TRUE;
	}

	if (computer)
	{
		info->computers = TRUE;
	}

	if (limit)
	{
		char *low = xmpp_stanza_get_attribute(limit, "low");
		char *high = xmpp_stanza_get_attribute(limit, "high");
		info->limit = malloc(sizeof(*(info->limit)));

		info->limit->type = strdup(xmpp_stanza_get_attribute(limit, "type"));
		info->limit->low = -1;
		info->limit->high = -1;

		if (low)
		{
			sscanf(low, "%d", &(info->limit->low));
		}

		if (high)
		{
			sscanf(high, "%d", &(info->limit->high));		
		}

		if ((low || high) & !info->limit->type)
		{
			info->limit->type = strdup("rating");
		}
	}

	if (colorpreference)
	{
		char *color = xmpp_stanza_get_attribute(colorpreference, "color");

                if (color && stricmp(color, "white") == 0)				
		{	
			info->colorpreference = 1;				
		}
		else if (color && stricmp(color, "black") == 0)
		{
			info->colorpreference = 2;
		}
	}

	if (state)
	{
		xmpp_stanza_t *whiteclock = xmpp_stanza_get_child_by_name(state, "white-clock");
		xmpp_stanza_t *blackclock = xmpp_stanza_get_child_by_name(state, "black-clock");
						
		if (whiteclock)
		{
			sscanf(xmpp_stanza_get_text(whiteclock), "%f", &(info->whiteclock));
		}

		if (blackclock)
		{
			sscanf(xmpp_stanza_get_text(blackclock), "%f", &(info->blackclock));
		}
	}

	if (comment)
	{
		info->comment = strdup(xmpp_stanza_get_text(comment));
	}

	if (correspondence)
	{
		info->correspondence = 1;
	}

	if (offline)
	{
		info->offline = 1;
	}

	return info;
}


struct rating_s *Conn_ParseRatingStanza(xmpp_stanza_t *rating)
{
	xmpp_stanza_t *rating2 = xmpp_stanza_get_child_by_name(rating, "rating");
	xmpp_stanza_t *rd      = xmpp_stanza_get_child_by_name(rating, "rd");
	xmpp_stanza_t *best    = xmpp_stanza_get_child_by_name(rating, "best");
	xmpp_stanza_t *worst   = xmpp_stanza_get_child_by_name(rating, "worst");
	xmpp_stanza_t *wins    = xmpp_stanza_get_child_by_name(rating, "wins");
	xmpp_stanza_t *losses  = xmpp_stanza_get_child_by_name(rating, "losses");
	xmpp_stanza_t *draws   = xmpp_stanza_get_child_by_name(rating, "draws");
	xmpp_stanza_t *prevrat = xmpp_stanza_get_child_by_name(rating, "previous-rating");

	char *parsetxt;

	struct rating_s *ratinginfo = malloc(sizeof(*ratinginfo));
	memset (ratinginfo, 0, sizeof(*ratinginfo));

	if (rating2 && (parsetxt = xmpp_stanza_get_text(rating2)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->rating));
	}

	if (rd && (parsetxt = xmpp_stanza_get_text(rd)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->rd));
	}

	if (best && (parsetxt = xmpp_stanza_get_text(best)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->best));
	}

	if (worst && (parsetxt = xmpp_stanza_get_text(worst)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->worst));
	}

	if (wins && (parsetxt = xmpp_stanza_get_text(wins)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->wins));
	}

	if (losses && (parsetxt = xmpp_stanza_get_text(losses)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->losses));
	}

	if (draws && (parsetxt = xmpp_stanza_get_text(draws)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->draws));
	}

	if (prevrat && (parsetxt = xmpp_stanza_get_text(prevrat)) && *parsetxt != '\0')
	{
		sscanf(parsetxt, "%d", &(ratinginfo->prevrating));
	}

	return ratinginfo;
}

int Conn_HandleIq(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleIq(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "result") == 0)
	{
		/*
		if (id && strcmp(id, "roster_set") == 0)
		{
		}
		if (id && strcmp(id, "roster_remove") == 0)
		{
		}
		if (id && strcmp(id, "vCard_get") == 0)
		{
		}
		*/
	}

/*

<iq id="arbiter_3" xmlns="jabber:client" type="set" to="jcanete@chesspark.com/cpc" from="arbiter.chesspark.com">
  <game-join id="430" xmlns="http://onlinegamegroup.com/xml/chesspark-01">
   <room>430@games.chesspark.com</room>
   <role>white</role>
  </game-join>
 </iq>

*/
	if (type && strcmp(type, "set") == 0)
	{
		xmpp_stanza_t *gamerequest = xmpp_stanza_get_child_by_name(stanza, "game-request");
		xmpp_stanza_t *gamejoin = xmpp_stanza_get_child_by_name(stanza, "game-join");
		xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

		if (gamerequest)
		{
			char *gameid = xmpp_stanza_get_attribute(gamerequest, "id");
			xmpp_stanza_t *gameaccepted = xmpp_stanza_get_child_by_name(gamerequest, "game-accepted");
			xmpp_stanza_t *gamerejected = xmpp_stanza_get_child_by_name(gamerequest, "game-rejected");

			xmpp_stanza_t *rematch = xmpp_stanza_get_child_by_name(gamerequest, "rematch");
			char *oldgameid = NULL;

			if (rematch)
			{
				oldgameid = xmpp_stanza_get_attribute(rematch, "oldgameid");
			}

			if (gameaccepted)
			{
				xmpp_stanza_t *iq;

				iq = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(iq, "iq");
				xmpp_stanza_set_type(iq, "result");
				xmpp_stanza_set_id(iq, id);
				xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
				xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));

				xmpp_send(conn, iq);

				xmpp_stanza_release(iq);
				
			}
			else if (gamerejected)
			{
				xmpp_stanza_t *iq;
				char *from = xmpp_stanza_get_attribute(gamerequest, "from");

				iq = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(iq, "iq");
				xmpp_stanza_set_type(iq, "result");
				xmpp_stanza_set_id(iq, id);
				xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
				xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));

				xmpp_send(conn, iq);

				xmpp_stanza_release(iq);

				Conn_HandleGameRequestError(stanza, from, NULL, NULL);

				/*ModelConn_GameRequestError(from, gameid, "Game request cancelled.");*/
			}
			else
			{
				xmpp_stanza_t *gamereconvene = xmpp_stanza_get_child_by_name(gamerequest, "game-reconvene");
				char *from = xmpp_stanza_get_attribute(gamerequest, "from");
				struct gamesearchinfo_s *info = Conn_ParseGameRequest(gamerequest);
				info->gameid = strdup(gameid);

				if (gamereconvene)
				{
					ModelConn_RequestReconvene(from, gameid, info);
				}
				else
				{
					ModelConn_RequestMatch(from, info, oldgameid);
				}

				Info_DestroyGameSearchInfo(info);
			}
		}
		else if (gamejoin)
		{
			xmpp_stanza_t *rematch = xmpp_stanza_get_child_by_name(gamejoin, "rematch");
			char *oldgameid = NULL;

			if (rematch)
			{
				oldgameid = xmpp_stanza_get_attribute(rematch, "oldgameid");
			}

			{
				xmpp_stanza_t *iq;

				iq = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(iq, "iq");
				xmpp_stanza_set_type(iq, "result");
				xmpp_stanza_set_id(iq, id);
				xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
				xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));

				xmpp_send(conn, iq);

				xmpp_stanza_release(iq);
			}
			{
				xmpp_stanza_t *room = xmpp_stanza_get_child_by_name(gamejoin, "room");
				xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(gamejoin, "role");
				char *gameid = xmpp_stanza_get_attribute(gamejoin, "id");
				char *roomtext = xmpp_stanza_get_text(room);
				char *roletext = xmpp_stanza_get_text(role);

				ModelConn_JoinGame(gameid, roomtext, roletext, oldgameid);
			}
		}
		else if (query)
		{
			char *ns = xmpp_stanza_get_ns(query);

			if (strcmp(ns, "jabber:iq:roster") == 0)
			{
				xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(query, "item");

				while (item)
				{
					char *jid = xmpp_stanza_get_attribute(item, "jid");
					char *ask = xmpp_stanza_get_attribute(item, "ask");
					char *subscription = xmpp_stanza_get_attribute(item, "subscription");

					if (subscription && (strcmp(subscription, "to") == 0) || (strcmp(subscription, "both") == 0))
					{
						Model_QueueMessage(MPMessage_Create(MODELMSG_SETSUBSCRIBED, strdup(jid), NULL, NamedList_FreeString, NULL, 0), 1);
					}

					if (subscription && (strcmp(subscription, "from") == 0) || (strcmp(subscription, "none") == 0))
					{
						Model_QueueMessage(MPMessage_Create(MODELMSG_SETUNSUBSCRIBED, strdup(jid), NULL, NamedList_FreeString, NULL, 0), 1);
					}

/*
					if (ask && strcmp(ask, "subscribe") == 0)
					{
						ModelConn_AskApprove(jid);
					}
*/
					item = xmpp_stanza_get_next_by_name(item, "item");
				}
			}

		}

	}

/*
<iq id="AAAAAAAE:sendmove" xmlns="jabber:client" type="modify" to="jcanete@chesspark.com/cpc" from="arbiter.chesspark.com">
 <error xmlns="http://onlinegamegroup.com/xml/chesspark-01" type="modify">
  <bad-request xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/>
  <move-error xmlns="http://onlinegamegroup.com/xml/chesspark-01"/>
 </error>
</iq>
*/

	if (type && strcmp(type, "get") == 0)
	{
/*
<iq type='get'
    from='romeo@montague.net/orchard'
    to='plays.shakespeare.lit'
    id='info1'>
  <query xmlns='http://jabber.org/protocol/disco#info'/>
</iq>
*/
		char *from = xmpp_stanza_get_attribute(stanza, "from");
		xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

		if (query)
		{
			char *ns = xmpp_stanza_get_ns(query);

			if (ns && strcmp(ns, "http://jabber.org/protocol/disco#info") == 0)
			{
/*
<iq type='result'
    from='plays.shakespeare.lit'
    to='romeo@montague.net/orchard'
    id='info1'>
  <query xmlns='http://jabber.org/protocol/disco#info'>
    <feature var='http://jabber.org/protocol/disco#info'/>
	<feature var='http://onlinegamegroup.com/xml/chesspark-01'/>
  </query>
</iq>
*/
				xmpp_stanza_t *iq, *query, *feature;
				
				iq = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(iq, "iq");
				xmpp_stanza_set_type(iq, "result");
				xmpp_stanza_set_id(iq, id);
				xmpp_stanza_set_attribute(iq, "to", from);

				query = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(query, "query");
				xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");

				feature = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(feature, "feature");
				xmpp_stanza_set_attribute(feature, "var", "http://jabber.org/protocol/disco#info");

				xmpp_stanza_add_child(query, feature);
				
				xmpp_stanza_release(feature);

				feature = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(feature, "feature");
				xmpp_stanza_set_attribute(feature, "var", "http://onlinegamegroup.com/xml/chesspark-01");

				xmpp_stanza_add_child(query, feature);

				xmpp_stanza_release(feature);

				xmpp_stanza_add_child(iq, query);

				xmpp_stanza_release(query);

				xmpp_send(conn, iq);

				xmpp_stanza_release(iq);	
			}
			else if (ns && strcmp(ns, "jabber:iq:version") == 0)
			{
				xmpp_stanza_t *iq, *query, *name, *version, *os;
				char versiontxt[120];
				
				iq = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(iq, "iq");
				xmpp_stanza_set_type(iq, "result");
				if (id)
				{
					xmpp_stanza_set_id(iq, id);
				}
				xmpp_stanza_set_attribute(iq, "to", from);

				query = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(query, "query");
				xmpp_stanza_set_ns(query, "jabber:iq:version");

				name = Conn_CreateTextStanza("name", "ChessparkClient");

				xmpp_stanza_add_child(query, name);

				xmpp_stanza_release(name);

				sprintf(versiontxt, "%s build %s", CHESSPARK_VERSION, CHESSPARK_BUILD);

				version = Conn_CreateTextStanza("version", versiontxt);

				xmpp_stanza_add_child(query, version);

				xmpp_stanza_release(version);

				os = Conn_CreateTextStanza("os", Util_WinVerText());

				xmpp_stanza_add_child(query, os);

				xmpp_stanza_release(os);

				xmpp_stanza_add_child(iq, query);

				xmpp_stanza_release(query);

				xmpp_send(conn, iq);

				xmpp_stanza_release(iq);
			}
		}
	}
/*
<iq id="match_181" xmlns="jabber:client" to="tofu@chesspark.com/cpc" type="error" from="match.chesspark.com">
 <error code="404" type="wait"><recipient-unavailable xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/>
 </error>
 <game-request id="2877" xmlns="http://onlinegamegroup.com/xml/chesspark-01" from="tofu@chesspark.com/cpc">
  <variant name="standard"/><game-accepted/>
 </game-request>
</iq>
*/

/*
<iq id="H_308" xmlns="jabber:client" type="error" to="dandee@chesspark.com/cpc" from="match.chesspark.com">
 <game-request id="2425" xmlns="http://onlinegamegroup.com/xml/chesspark-01"><game-rejected/>
 </game-request>
 <member-playing>jeff23@chesspark.com/cpwc</member-playing></iq>
*/
	if (type && strcmp(type, "error") == 0)
	{
		/*Conn_HandleGameRequestError(stanza, NULL, NULL);*/
	}

	return 1;
}

int Conn_HandleRoster(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
		
	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleRoster(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (!type || !(strcmp(type, "result") == 0))
	{
		return 0;
	}

	if (query)
	{
		xmpp_stanza_t *item;
		struct namedlist_s *rosterlist = NULL;
		int hasgroups = FALSE;

		Model_ShowLoginTime("gotroster");

		/*ModelConn_PurgeRoster();*/
		Model_QueueMessage(MPMessage_CreateSimple(MODELMSG_STARTROSTER), 1);

		for (item = xmpp_stanza_get_child_by_name(query, "item"); item; item = xmpp_stanza_get_next_by_name(item, "item"))
		{
			char *jid = xmpp_stanza_get_attribute(item, "jid");
			char *subscription = xmpp_stanza_get_attribute(item, "subscription");
			char *name = xmpp_stanza_get_attribute(item, "name");
			char *ask = xmpp_stanza_get_attribute(item, "ask");
			int subscribedTo = FALSE, subscribedFrom = FALSE, pending = FALSE;
			xmpp_stanza_t *group;
			struct namedlist_s *grouplist = NULL;
			struct rosterentry_s *entry;
				
			if (stricmp(subscription, "to") == 0
				|| stricmp(subscription, "to + pending in") == 0
				|| stricmp(subscription, "both") == 0)
			{
				subscribedTo = TRUE;
			}
					
			if (stricmp(subscription, "from") == 0
				|| stricmp(subscription, "from + pending out") == 0
				|| stricmp(subscription, "both") == 0)
			{
				subscribedFrom = TRUE;
			}

			if (ask && stricmp(ask, "subscribe") == 0)
			{
				pending = TRUE;
			}

			group = xmpp_stanza_get_child_by_name(item, "group");

			while (group)
			{
				if (strcmp(xmpp_stanza_get_name(group), "group") == 0)
				{
					NamedList_AddString(&grouplist, xmpp_stanza_get_text(group), NULL);
					hasgroups = TRUE;
				}
				group = xmpp_stanza_get_next_by_name(group, "group");
			}

			entry = malloc(sizeof(*entry));
			memset(entry, 0, sizeof(*entry));

			entry->jid = strdup(jid);
			entry->name = strdup(name);
			entry->groups = grouplist;
			entry->subscribedto = subscribedTo;
			entry->subscribedfrom = subscribedFrom;
			entry->pending = pending;

			Model_QueueMessage(MPMessage_Create(MODELMSG_ADDROSTERENTRY, entry, NULL, RosterEntry_Destroy, NULL, 0), 1);

			/*NamedList_AddToTop(&rosterlist, NULL, entry, RosterEntry_Destroy);*/
/*
			ModelConn_SetFriend(jid, name, grouplist, subscribedTo, subscribedFrom, 1);

			NamedList_Destroy(&grouplist);
*/
		}

		/*ModelConn_HandleRoster(rosterlist);*/

		/*NamedList_Destroy(&rosterlist);*/

		/*
		if (!hasgroups)
		{
			ModelConn_OpenAllGroup();
		}
		*/

		Model_QueueMessage(MPMessage_CreateSimple(MODELMSG_FINISHROSTER), 1);

		/*ModelConn_FinishRoster();*/
	}

	return 0;
}


int Conn_HandleRoomsList(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
		
	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleRoomsList(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (!type || !(strcmp(type, "result") == 0))
	{
		return 0;
	}

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		if (ns && strcmp(ns, "http://jabber.org/protocol/disco#items") == 0)
		{
			xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(query, "item");
			struct namedlist_s *roomnames = NULL;
			while (item)
			{
				char *jid = xmpp_stanza_get_attribute(item, "jid");
				char *name = xmpp_stanza_get_attribute(item, "name");

				NamedList_AddString(&roomnames, jid, NULL);
				/*Conn_GetRoomInfo(jid);*/

				item = xmpp_stanza_get_next_by_name(item, "item");
			}
			ModelConn_ParseRoomNames(roomnames);

			NamedList_Destroy(&roomnames);
		}
	}

	return 0;
}

int Conn_HandleRoomInfo(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
			
	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleRoomInfo(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "error") == 0)
	{
		ModelConn_AddChatroomError(id + 22);
	}

	if (!type || !(strcmp(type, "result") == 0))
	{
		return 0;
	}

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		if (ns && strcmp(ns, "http://jabber.org/protocol/disco#info") == 0)
		{
			xmpp_stanza_t *identity = xmpp_stanza_get_child_by_name(query, "identity");
			xmpp_stanza_t *feature = xmpp_stanza_get_child_by_name(query, "feature");
			xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(query, "x");
			char *jid = xmpp_stanza_get_attribute(stanza, "from");
			char *name = NULL;
			char *topic = NULL;
			int users = 0;

			if (identity)
			{
				name = xmpp_stanza_get_attribute(identity, "name");
			}

			while (feature)
			{
				feature = xmpp_stanza_get_next_by_name(feature, "feature");
			}

			while (x)
			{
				char *ns = xmpp_stanza_get_ns(x);

				if (ns && strcmp(ns, "jabber:x:data") == 0)
				{
					xmpp_stanza_t *field = xmpp_stanza_get_child_by_name(x, "field");

					while (field)
					{
						char *var = xmpp_stanza_get_attribute(field, "var");
						xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(field, "value");

						if (var && value)
						{
							if (strcmp(var, "muc#roominfo_subject") == 0)
							{
								topic = xmpp_stanza_get_text(value);
							}
							else if (strcmp(var, "muc#roominfo_occupants") == 0)
							{
								sscanf(xmpp_stanza_get_text(value), "%d", &users);
							}
						}
						
						field = xmpp_stanza_get_next_by_name(field, "field");
					}
				}
				x = xmpp_stanza_get_next_by_name(x, "x");
			}

			if (!topic)
			{
				topic = "";
			}
			ModelConn_AddChatroom(jid, name, topic, users);
		}
	}
	return 0;
}

xmpp_stanza_t *Conn_CreateTimeControl2(int idelayinc, int *controlarray, int prefs)
{
	xmpp_stanza_t *timecontrol;
	char txt[80];
	int numcontrols;

	if (!controlarray)
	{
		return NULL;
	}

	timecontrol = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(timecontrol, "time-control");
	xmpp_stanza_set_ns(timecontrol, "http://onlinegamegroup.com/xml/chesspark-01");

	sprintf(txt, "%d", idelayinc);
	
	xmpp_stanza_set_attribute(timecontrol, "delayinc", txt);
	
	numcontrols = *controlarray++;

	while (numcontrols)
	{
		xmpp_stanza_t *control;
		int imoves, itime;

		control = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(control, "control");

		imoves = *controlarray++;
		itime = *controlarray++;

		if (itime != -1)
		{
			if (prefs)
			{
				sprintf(txt, "%d", itime);
				xmpp_stanza_set_attribute(control, "time", txt);
			}
			else
			{
				xmpp_stanza_t *time, *timetxt;

				time = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(time, "time");

				sprintf(txt, "%d", itime);

				timetxt = xmpp_stanza_new(ctx);
				xmpp_stanza_set_text(timetxt, txt);

				xmpp_stanza_add_child(time, timetxt);

				xmpp_stanza_release(timetxt);

				xmpp_stanza_add_child(control, time);

				xmpp_stanza_release(time);
			}
		}

		if (imoves != -1)
		{
			if (prefs)
			{
				sprintf(txt, "%d", imoves);
				xmpp_stanza_set_attribute(control, "moves", txt);
			}
			else
			{
				xmpp_stanza_t *moves, *movestxt;

				moves = xmpp_stanza_new(ctx);
				xmpp_stanza_set_name(moves, "moves");

				sprintf(txt, "%d", imoves);

				movestxt = xmpp_stanza_new(ctx);
				xmpp_stanza_set_text(movestxt, txt);

				xmpp_stanza_add_child(moves, movestxt);

				xmpp_stanza_release(movestxt);

				xmpp_stanza_add_child(control, moves);

				xmpp_stanza_release(moves);
			}
		}

		xmpp_stanza_add_child(timecontrol, control);

		xmpp_stanza_release(control);

		numcontrols--;
	}

	return timecontrol;
}

xmpp_stanza_t *Conn_CreateTimeControl(int idelayinc, int *controlarray)
{
	return Conn_CreateTimeControl2(idelayinc, controlarray, 0);
}

void Conn_ParseTimeControl(xmpp_stanza_t *timecontrol, int *delayinc, int **controlarray)
{
	xmpp_stanza_t *control = xmpp_stanza_get_child_by_name(timecontrol, "control");

	if (control)
	{
		int numcontrols, *p;
		char *cdelayinc = xmpp_stanza_get_attribute(timecontrol, "delayinc");

		if (cdelayinc)
		{
			sscanf(cdelayinc, "%d", delayinc);
		}
		else
		{
			*delayinc = 0;
		}

		numcontrols = 0;
		
		while (control)
		{
			numcontrols++;

			control = xmpp_stanza_get_next_by_name(control, "control");
		}

		*controlarray = malloc(sizeof(**controlarray) * (numcontrols * 2 + 1));
		p = *controlarray;
		*p++ = numcontrols;

		control = xmpp_stanza_get_child_by_name(timecontrol, "control");

		while (control)
		{
			xmpp_stanza_t *moves = xmpp_stanza_get_child_by_name(control, "moves");
			xmpp_stanza_t *time = xmpp_stanza_get_child_by_name(control, "time");
			char *cmoves = xmpp_stanza_get_attribute(control, "moves");
			char *ctime  = xmpp_stanza_get_attribute(control, "time");
			int itime = -1 , imoves = -1;

			if (moves)
			{
				sscanf(xmpp_stanza_get_text(moves), "%d", &imoves);
			}
			else if (cmoves)
			{
				sscanf(cmoves, "%d", &imoves);
			}

			if (time)
			{
				sscanf(xmpp_stanza_get_text(time), "%d", &itime);
			}
			else if (ctime)
			{
				sscanf(ctime, "%d", &itime);
			}

			*p++ = imoves;
			*p++ = itime;

			control = xmpp_stanza_get_next_by_name(control, "control");
		}
	}
	else
	{
		*controlarray = malloc(sizeof(int));
		**controlarray = 0;
	}
}

void Conn_HandleGame(xmpp_stanza_t *game)
{
	xmpp_stanza_t *move             = xmpp_stanza_get_child_by_name(game, "move");
	xmpp_stanza_t *state            = xmpp_stanza_get_child_by_name(game, "state");
	xmpp_stanza_t *initialstate     = xmpp_stanza_get_child_by_name(game, "initial-state");
	xmpp_stanza_t *timecontrol      = xmpp_stanza_get_child_by_name(game, "time-control");
	xmpp_stanza_t *currenttimeclock = xmpp_stanza_get_child_by_name(game, "current-time-clock");
	xmpp_stanza_t *correspondence   = xmpp_stanza_get_child_by_name(game, "correspondence");
	xmpp_stanza_t *gameover         = xmpp_stanza_get_child_by_name(game, "gameover");
	xmpp_stanza_t *draw             = xmpp_stanza_get_child_by_name(game, "draw");
	xmpp_stanza_t *rejectdraw       = xmpp_stanza_get_child_by_name(game, "rejectdraw");
	xmpp_stanza_t *abort            = xmpp_stanza_get_child_by_name(game, "abort");
	xmpp_stanza_t *rejectabort      = xmpp_stanza_get_child_by_name(game, "rejectabort");
	xmpp_stanza_t *takeback	        = xmpp_stanza_get_child_by_name(game, "takeback");
	xmpp_stanza_t *rejecttakeback   = xmpp_stanza_get_child_by_name(game, "rejecttakeback");
	xmpp_stanza_t *rating           = xmpp_stanza_get_child_by_name(game, "rating");
	xmpp_stanza_t *rated            = xmpp_stanza_get_child_by_name(game, "rated");
	xmpp_stanza_t *variant          = xmpp_stanza_get_child_by_name(game, "variant");
	xmpp_stanza_t *tournament       = xmpp_stanza_get_child_by_name(game, "tournament");
	xmpp_stanza_t *time             = xmpp_stanza_get_child_by_name(game, "time");
	xmpp_stanza_t *resign           = xmpp_stanza_get_child_by_name(game, "resign");
	xmpp_stanza_t *adjourn          = xmpp_stanza_get_child_by_name(game, "adjourn");
	xmpp_stanza_t *rejectadjourn    = xmpp_stanza_get_child_by_name(game, "rejectadjourn");
	xmpp_stanza_t *sdatestarted     = xmpp_stanza_get_child_by_name(game, "date-started");
			
	char *white  = xmpp_stanza_get_attribute(game, "white");
	char *black  = xmpp_stanza_get_attribute(game, "black");
	char *gameid = xmpp_stanza_get_attribute(game, "id");
	char *blackcurrentclock = NULL;
	char *whitecurrentclock = NULL;
	int tick = (move == NULL);
	struct namedlist_s *textmovelist = NULL;
	char *datestarted = NULL;

	if (move)
	{
		char *stamp = xmpp_stanza_get_attribute(move, "stamp");

		if (stamp)
		{
			ModelConn_ClearMoveList(gameid);
		}
	}
	else if (takeback)
	{
		char *white = xmpp_stanza_get_attribute(takeback, "white");
		char *black = xmpp_stanza_get_attribute(takeback, "black");
		char *side = xmpp_stanza_get_attribute(takeback, "side");
		char *player = xmpp_stanza_get_attribute(takeback, "player");
		if (!white && !black && side && player)
		{
			if (stricmp(side, "white") == 0)
			{
				white = player;
			}
			if (stricmp(side, "black") == 0)
			{
				black = player;
			}
		}

		ModelConn_HandleTakeback(gameid, white, black);
	}

	if (rejecttakeback)
	{
		char *from = xmpp_stanza_get_attribute(game, "from");
		ModelConn_HandleRejectTakeback(gameid);
	}

	if (state)
	{
		struct gamesearchinfo_s *info;
		char *categorytxt = NULL;
		char *tourneyid = NULL;
		char *statetext = NULL;
		char *initialstatetext = NULL;
		statetext = xmpp_stanza_get_text(state);

		info = malloc(sizeof(*info));
		memset(info, 0, sizeof(*info));

		info->gameid = strdup(gameid);

		if (initialstate)
		{
			initialstatetext = xmpp_stanza_get_text(initialstate);
		}

		if (white)
		{
			info->white = malloc(sizeof(*(info->white)));
			memset(info->white, 0, sizeof(*(info->white)));
			info->white->jid = strdup(white);
		}

		if (black)
		{
			info->black = malloc(sizeof(*(info->black)));
			memset(info->black, 0, sizeof(*(info->black)));
			info->black->jid = strdup(black);
		}

		while (timecontrol)
		{
			char *side = xmpp_stanza_get_attribute(timecontrol, "side");
			int delayinc = 0;
			int *controlarray = NULL;

			Conn_ParseTimeControl(timecontrol, &delayinc, &controlarray);

			if (controlarray)
			{
				if (!side || strcmp(side, "white") == 0)
				{
					info->timecontrol = malloc(sizeof(*(info->timecontrol)));
					memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));

					info->timecontrol->controlarray = controlarray;
					info->timecontrol->delayinc = delayinc;
				}
				else if (strcmp(side, "black") == 0)
				{
					info->blacktimecontrol = malloc(sizeof(*(info->blacktimecontrol)));
					memset(info->blacktimecontrol, 0, sizeof(*(info->blacktimecontrol)));

					info->blacktimecontrol->controlarray = controlarray;
					info->blacktimecontrol->delayinc = delayinc;
				}
			}

			timecontrol = xmpp_stanza_get_next_by_name(timecontrol, "time-control");
		}

		while (currenttimeclock)
		{
			char *side = xmpp_stanza_get_attribute(currenttimeclock, "side");
			char *timetext = xmpp_stanza_get_text(currenttimeclock);

			if (strcmp(side, "white") == 0)
			{
				whitecurrentclock = strdup(timetext);
			}
			else if (strcmp(side, "black") == 0)
			{
				blackcurrentclock = strdup(timetext);
			}
						
			currenttimeclock = xmpp_stanza_get_next_by_name(currenttimeclock, "current-time-clock");
		}

		if (rating)
		{
			xmpp_stanza_t *category = xmpp_stanza_get_child_by_name(rating, "category");

			if (category)
			{
				info->category = strdup(xmpp_stanza_get_text(category));
			}
		}

		if (rated)
		{
			info->rated = 1;
		}

		if (variant)
		{
			char *variantname = xmpp_stanza_get_attribute(variant, "name");

			if (!variantname || strlen(variantname) == 0)
			{
				variantname = xmpp_stanza_get_text(variant);
			}

			info->variant = strdup(variantname);
		}

		if (tournament)
		{
			tourneyid = xmpp_stanza_get_attribute(tournament, "id");
		}

		ModelConn_SetGameState(gameid, initialstatetext, statetext, info, whitecurrentclock, blackcurrentclock, tourneyid);

		free(whitecurrentclock);
		free(blackcurrentclock);
	}

	while (time)
	{
		char *control = xmpp_stanza_get_attribute(time, "control");
		char *side = xmpp_stanza_get_attribute(time, "side");
		char *timetext = xmpp_stanza_get_text(time);

		ModelConn_SyncClock(gameid, timetext, side, control, tick);

		time = xmpp_stanza_get_next_by_name(time, "time");
	}

	while (move)
	{
		char *movetext = xmpp_stanza_get_text(move);
		char *stamp = xmpp_stanza_get_attribute(move, "stamp");
		char *annotation  = xmpp_stanza_get_attribute(move, "annotation");
		char *cply = xmpp_stanza_get_attribute(move, "ply");

		if (movetext && strlen(movetext) > 0)
		{
			if (stamp)
			{
				NamedList_AddString(&textmovelist, movetext, annotation);
				/*ModelConn_AddMoveToMoveList(gameid, movetext, annotation);*/
			}
			else
			{
				int numtakebacks = 0;
				int ply = 0;

				while (takeback)
				{
					numtakebacks++;
					takeback = xmpp_stanza_get_next_by_name(takeback, "takeback");
				}

				if (cply)
				{
					sscanf(cply, "%d", &ply);
				}

				ModelConn_ParseGameMove(gameid, movetext, annotation, white, black, correspondence != NULL, numtakebacks, ply);
			}
		}

		move = xmpp_stanza_get_next_by_name(move, "move");
	}

	if (textmovelist)
	{
		ModelConn_SetGameMoveList(gameid, textmovelist);
	}

	NamedList_Destroy(&textmovelist);

	if (sdatestarted)
	{
		datestarted = xmpp_stanza_get_text(sdatestarted);
	}

	if (gameover)
	{
		char *type = xmpp_stanza_get_attribute(gameover, "type");
		char *win = xmpp_stanza_get_attribute(gameover, "win");
		char *lose = xmpp_stanza_get_attribute(gameover, "lose");
		char *reason = xmpp_stanza_get_attribute(gameover, "reason");
		ModelConn_HandleGameOver(gameid, white, black, type, win, lose, reason, correspondence != NULL, datestarted);
	}

	if (resign)
	{
		ModelConn_HandleResign(gameid);
	}

	if (draw)
	{
		char *from = xmpp_stanza_get_attribute(game, "from");
		char *whitedraw = xmpp_stanza_get_attribute(draw, "white");
		char *blackdraw = xmpp_stanza_get_attribute(draw, "black");
		ModelConn_HandleDraw(gameid, from, whitedraw != NULL && strlen(whitedraw) != 0, blackdraw != NULL && strlen(blackdraw) != 0, correspondence != NULL);
	}

	if (rejectdraw)
	{
		char *from = xmpp_stanza_get_attribute(game, "from");
		ModelConn_HandleRejectDraw(gameid, from, correspondence != NULL);
	}

	if (adjourn)
	{
		char *white = xmpp_stanza_get_attribute(adjourn, "white");
		char *black = xmpp_stanza_get_attribute(adjourn, "black");
		ModelConn_HandleAdjourn(gameid, white, black);
	}

	if (rejectadjourn)
	{
		ModelConn_HandleRejectAdjourn(gameid);
	}

	if (abort)
	{
		char *white = xmpp_stanza_get_attribute(abort, "white");
		char *black = xmpp_stanza_get_attribute(abort, "black");
		ModelConn_HandleAbort(gameid, white, black);
	}

	if (rejectabort)
	{
		char *from = xmpp_stanza_get_attribute(game, "from");
		ModelConn_HandleRejectAbort(gameid);
	}
}


void Conn_HandleProfilePlayingFields(char *jid, xmpp_stanza_t *items, int update)
{
	xmpp_stanza_t *field = xmpp_stanza_get_child_by_name(items, "field");

	while (field)
	{
		char *var = xmpp_stanza_get_attribute(field, "var");

		if (var && (strcmp(var, "playing") == 0 || strcmp(var, "watching") == 0 || strcmp(var, "stopped-playing") == 0 || strcmp(var, "stopped-watching") == 0))
		{
			xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(field, "value");

			while (value)
			{
				xmpp_stanza_t *game = xmpp_stanza_get_child_by_name(value, "game");

				while (game)
				{
					xmpp_stanza_t *playing         = xmpp_stanza_get_child_by_name(game, "playing");
					xmpp_stanza_t *stoppedplaying  = xmpp_stanza_get_child_by_name(game, "stopped-playing");
					xmpp_stanza_t *watching        = xmpp_stanza_get_child_by_name(game, "watching");
					xmpp_stanza_t *stoppedwatching = xmpp_stanza_get_child_by_name(game, "stopped-watching");
					xmpp_stanza_t *timecontrol     = xmpp_stanza_get_child_by_name(game, "time-control");
					xmpp_stanza_t *variant         = xmpp_stanza_get_child_by_name(game, "variant");
					xmpp_stanza_t *rating          = xmpp_stanza_get_child_by_name(game, "rating");
					xmpp_stanza_t *rated           = xmpp_stanza_get_child_by_name(game, "rated");
							
					char *gameid = xmpp_stanza_get_attribute(game, "id");
					struct gamesearchinfo_s *info = malloc(sizeof(*info));

					memset(info, 0, sizeof(*info));

					info->white = malloc(sizeof(*(info->white)));
					info->black = malloc(sizeof(*(info->black)));

					memset(info->white, 0, sizeof(*(info->white)));
					memset(info->black, 0, sizeof(*(info->black)));

					info->white->jid = strdup(xmpp_stanza_get_attribute(game, "white"));
					info->black->jid = strdup(xmpp_stanza_get_attribute(game, "black"));

					if (timecontrol)
					{
						info->timecontrol = malloc(sizeof(*(info->timecontrol)));
						memset(info->timecontrol, 0, sizeof(*(info->timecontrol)));
							
						Conn_ParseTimeControl(timecontrol, &(info->timecontrol->delayinc), &(info->timecontrol->controlarray));
					}

					if (variant)
					{
						char *variantname = xmpp_stanza_get_attribute(variant, "name");

						if (!variantname || strlen(variantname) == 0)
						{
							variantname = xmpp_stanza_get_text(variant);
						}

						info->variant = strdup(variantname);
					}

					if (rating)
					{
						xmpp_stanza_t *category = xmpp_stanza_get_child_by_name(rating, "category");

						if (category)
						{
							info->category = strdup(xmpp_stanza_get_text(category));
						}
					}

					if (rated)
					{
						info->rated = 1;
					}

					ModelConn_SetGameStatus(jid, gameid, info, stoppedplaying != NULL || stoppedwatching != NULL, watching != NULL || stoppedwatching != NULL, update, 0); 

					game = xmpp_stanza_get_next_by_name(game, "game");
				}

				value = xmpp_stanza_get_next_by_name(value, "value");
			}
		}
		else if (var && strcmp(var, "rating") == 0)
		{
		}

		field = xmpp_stanza_get_next_by_name(field, "field");
	}
}


struct groupinfo_s *Conn_ParseGroupStanza(xmpp_stanza_t *group)
{
	xmpp_stanza_t *roles = xmpp_stanza_get_child_by_name(group, "roles");
	xmpp_stanza_t *avatar = xmpp_stanza_get_child_by_name(group, "avatar");
	xmpp_stanza_t *titles = xmpp_stanza_get_child_by_name(group, "titles");
	xmpp_stanza_t *permissions = xmpp_stanza_get_child_by_name(group, "permissions");
	struct groupinfo_s *ginfo;

	ginfo = malloc(sizeof(*ginfo));
	memset(ginfo, 0, sizeof(*ginfo));

	ginfo->name  = strdup(xmpp_stanza_get_attribute(group, "name"));
	ginfo->chat  = strdup(xmpp_stanza_get_attribute(group, "chat"));
	ginfo->type  = strdup(xmpp_stanza_get_attribute(group, "type"));
	ginfo->forum = strdup(xmpp_stanza_get_attribute(group, "forum"));
	ginfo->id    = strdup(xmpp_stanza_get_attribute(group, "id"));

	if (avatar)
	{
		ginfo->avatar = strdup(xmpp_stanza_get_text(avatar));
	}

	if (roles)
	{
		xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(roles, "role");

		while (role)
		{
			NamedList_AddString(&(ginfo->roles), xmpp_stanza_get_text(role), xmpp_stanza_get_text(role));
			role = xmpp_stanza_get_next_by_name(role, "role");
		}
	}

	if (titles)
	{
		xmpp_stanza_t *title = xmpp_stanza_get_child_by_name(titles, "title");

		while (title)
		{
			NamedList_AddString(&(ginfo->titles), xmpp_stanza_get_text(title), xmpp_stanza_get_text(title));
			title = xmpp_stanza_get_next_by_name(title, "title");
		}
	}
	
	if (permissions)
	{
		xmpp_stanza_t *feature = xmpp_stanza_get_child_by_name(permissions, "feature");
		while (feature)
		{
			char *featurename = xmpp_stanza_get_attribute(feature, "type");

			NamedList_AddString(&(ginfo->permissions), featurename, NULL);

			feature = xmpp_stanza_get_next_by_name(feature, "feature");
		}
	}

	return ginfo;
}

int Conn_HandleMessage(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type = xmpp_stanza_get_type(stanza);
	char *id   = xmpp_stanza_get_id(stanza);
	char *name = xmpp_stanza_get_name(stanza);
	char *from = xmpp_stanza_get_attribute(stanza, "from");

	xmpp_stanza_t *body         = xmpp_stanza_get_child_by_name(stanza, "body");
	xmpp_stanza_t *subject      = xmpp_stanza_get_child_by_name(stanza, "subject");
	xmpp_stanza_t *game         = xmpp_stanza_get_child_by_name(stanza, "game");
	xmpp_stanza_t *event        = xmpp_stanza_get_child_by_name(stanza, "event");
	xmpp_stanza_t *x            = xmpp_stanza_get_child_by_name(stanza, "x");
	xmpp_stanza_t *gamerequest  = xmpp_stanza_get_child_by_name(stanza, "game-request");
	xmpp_stanza_t *tournament   = xmpp_stanza_get_child_by_name(stanza, "tournament");
	xmpp_stanza_t *search       = xmpp_stanza_get_child_by_name(stanza, "search");
	xmpp_stanza_t *gamestart    = xmpp_stanza_get_child_by_name(stanza, "game-start");
	xmpp_stanza_t *groups       = xmpp_stanza_get_child_by_name(stanza, "groups");
	int supressnormal = 0;

	char *msgid = NULL;
	xmpp_stanza_t *composing = NULL;
	char *timestamp = NULL;

	Log_Write(0, "Conn_HandleMessage(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

/*
<message
    from='romeo@montague.net'
    to='juliet@capulet.com/balcony'>
  <x xmlns='jabber:x:event'>
    <composing/>
    <id>message22</id>
  </x>
</message>
*/

	while (x)
	{
		char *ns = xmpp_stanza_get_ns(x);

		if (ns && strcmp(ns, "jabber:x:event") == 0)
		{
			xmpp_stanza_t *id = xmpp_stanza_get_child_by_name(x, "id");

			composing = xmpp_stanza_get_child_by_name(x, "composing");

			if (id)
			{
				msgid = xmpp_stanza_get_text(id);
			}

			if (body || event)
			{
			}
			else
			{
				if (composing)
				{
					ModelConn_SetComposing(from, msgid);
				}
				else
				{
					ModelConn_UnsetComposing(from, msgid);
				}
			}
		}
		else if (ns && strcmp(ns, "jabber:x:delay") == 0)
		{
			timestamp = xmpp_stanza_get_attribute(x, "stamp");
		}
		else if (ns && strcmp(ns, "http://onlinegamegroup.com/xml/chesspark-01") == 0)
		{
			xmpp_stanza_t *roster = xmpp_stanza_get_child_by_name(x, "roster");
#if 0 /* disabled initial roster */
			if (roster)
			{
				xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(roster, "item");

				while (item)
				{
					char *jid = xmpp_stanza_get_attribute(item, "jid");
					char *nick = xmpp_stanza_get_attribute(item, "name");
					char *group = xmpp_stanza_get_attribute(item, "group");
					struct namedlist_s *grouplist = NULL;

					if (!Model_HasFriend(jid))
					{
						char *barejid = Jid_Strip(jid);

						if (group)
						{
							NamedList_AddString(&grouplist, group, NULL);
						}

						/*ModelConn_SetFriend(barejid, nick, grouplist, 0, 1, 0);
						Conn_SetFriend(barejid, nick, grouplist);*/
						Conn_ApproveFriend(barejid);
						/*Conn_RequestFriend(barejid);*/

						free(barejid);
					}

					NamedList_Destroy(&grouplist);

					item = xmpp_stanza_get_next_by_name(item, "item");
				}
			}
#endif
		}
		else if (ns && strcmp(ns, "http://jabber.org/protocol/muc#user") == 0)
		{
			xmpp_stanza_t *invite = xmpp_stanza_get_child_by_name(x, "invite");

			if (invite && type && strcmp(type, "error") != 0)
			{
				char *invitefrom = xmpp_stanza_get_attribute(invite, "from");

				ModelConn_ShowChatInviteMessage(invitefrom, from);

				supressnormal = 1;
			}
		}
/*
<message to='user@chesspark.com' from='login.chesspark.com'>
	 <x xmlns='http://onlinegamegroup.com/xml/chesspark-01'>
	    <roster>
		<item jid='test@chesspark.com/cpc' name='Tester, The'/>
	    </roster>
         </x>
</message>
*/
		x = xmpp_stanza_get_next_by_name(x, "x");
	}

	if (type && strcmp(type, "chat") == 0)
	{
		char *from = xmpp_stanza_get_attribute(stanza, "from");

		if (from && strstr(from, "chesspark.com") && !strchr(from, '@'))
		{
			/* system message, ignore */
		}
		else if (body)
		{
			char *bodytext = xmpp_stanza_get_text(body);
			struct messageinfo_s *info;

			info = malloc(sizeof(*info));
			memset(info, 0, sizeof(*info));

			info->jid          = strdup(from);
			info->text         = strdup(bodytext);
			info->msgid        = strdup(msgid);
			info->reqcomposing = composing != NULL;
			info->ismuc        = 0;

			Model_QueueMessage(MPMessage_Create(MODELMSG_HANDLEMESSAGE, info, NULL, MessageInfo_Destroy, NULL, 0), 2);

			/*ModelConn_AddChatMessage(from, bodytext, msgid, composing != NULL);*/
		}
	}
	else if (type && strcmp(type, "groupchat") == 0)
	{
		if (body)
		{
			char *bodytext = xmpp_stanza_get_text(body);
			struct messageinfo_s *info;

			info = malloc(sizeof(*info));
			memset(info, 0, sizeof(*info));

			info->jid          = strdup(from);
			info->text         = strdup(bodytext);
			info->timestamp    = strdup(timestamp);
			info->ismuc        = 1;

			Model_QueueMessage(MPMessage_Create(MODELMSG_HANDLEMESSAGE, info, NULL, MessageInfo_Destroy, NULL, 0), 1);

			/*ModelConn_AddGroupChatMessage(from, bodytext, timestamp);*/
		}
		if (subject)
		{
			char *subjecttext = xmpp_stanza_get_text(subject);
			ModelConn_SetGroupChatTopic(from, subjecttext);
		}
	}
	else if (!supressnormal && (!type || (type && strcmp(type, "normal") == 0)) && body) /* login.chesspark.com messages */
	{
		char *bodytext = xmpp_stanza_get_text(body);
		char *subjecttext = NULL;

		if (subject)
		{
			subjecttext = xmpp_stanza_get_text(subject);
		}

		if (from)
		{
			if (strstr(from, "chesspark.com") && !strchr(from, '@'))
			{
				ModelConn_ShowAnnouncement(from, bodytext, subjecttext);
			}
			else
			{
				struct messageinfo_s *info;

				info = malloc(sizeof(*info));
				memset(info, 0, sizeof(*info));

				info->jid          = strdup(from);
				info->text         = strdup(bodytext);
				info->msgid        = strdup(msgid);
				info->reqcomposing = composing != NULL;
				info->ismuc        = 0;

				Model_QueueMessage(MPMessage_Create(MODELMSG_HANDLEMESSAGE, info, NULL, MessageInfo_Destroy, NULL, 0), 2);

				/*ModelConn_AddChatMessage(from, bodytext, msgid, composing != NULL);*/
			}
		}
	}
	else if (type && stricmp(type, "error") == 0)
	{
		if (from)
		{
			xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");
			xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, "x");
			char *code = NULL;
			int handled = 0;

			if (error)
			{
				code = xmpp_stanza_get_attribute(error, "code");
			}

			if (x)
			{
				char *ns = xmpp_stanza_get_ns(x);

				if (ns && stricmp(ns, "http://jabber.org/protocol/muc#user") == 0)
				{
					xmpp_stanza_t *invite = xmpp_stanza_get_child_by_name(x, "invite");

					if (invite)
					{
						ModelConn_InviteUserError(from);
						handled = 1;
					}
				}
			}
			
			if (!handled)
			{
				ModelConn_MucError(from, code);
			}
		}
	}
/*
<message xmlns="jabber:client" to="jcanete@chesspark.com" from="profile.chesspark.com">
 <event xmlns="http://jabber.org/protocol/pubsub#event">
  <items jid="test@chesspark.com" node="http://jabber.org/protocol/profile">
   <field var="stoped-watching">
    <value>
	 <game id="2266" black="test@chesspark.com/cpc" white="robopawn@chesspark.com/RoboPawn" xmlns="http://onlinegamegroup.com/xml/chesspark-01">
	  <stoped-watching/>
	  <time-control side="white" delayinc="-5"><control><time>1800</time></control></time-control>
	  <time-control side="black" delayinc="-5"><control><time>1800</time></control></time-control>
	  <variant name="standard"/>
	  <rating><category>Long</category></rating>
	 </game>
	</value>
   </field>
  </items>
 </event>
</message>
*/

/*
<message xmlns="jabber:client" type="chat" to="jcanete@chesspark.com/cpc" from="login.chesspark.com">
 <upgrade number="1.0" xmlns="http://onlinegamegroup.com/xml/chesspark-01" url="http://alpha.chesspark.com" build="110" version="alpha"/>
 <body>You seem to have an older version of chesspark, please upgrade. http://alpha.chesspark.com/</body>
</message>
*/

/*
<message xmlns="jabber:client" to="jcanete@chesspark.com/cpc" from="ratings.chesspark.com">
 <event xmlns="http://jabber.org/protocol/pubsub#event">
  <items jid="test@chesspark.com" node="ratings">
   <rating jid="test@chesspark.com">
    <rating>1337</rating>
	<category>Blitz</category>
   </rating>
  </items>
 </event>
</message>
*/

	if (event)
	{
		xmpp_stanza_t *items = xmpp_stanza_get_child_by_name(event, "items");

		while (items)
		{
			char *jid = xmpp_stanza_get_attribute(items, "jid");
			char *node = xmpp_stanza_get_attribute(items, "node");
			xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(items, "rating");

			if (rating)
			{
				char *jid = xmpp_stanza_get_attribute(rating, "jid");
				xmpp_stanza_t *category = xmpp_stanza_get_child_by_name(rating, "category");
				/*xmpp_stanza_t *variant     = xmpp_stanza_get_child_by_name(rating, "variant");*/
					
				struct rating_s *ratinginfo;

				ratinginfo = Conn_ParseRatingStanza(rating);

				if (category)
				{
					ModelConn_SetRating(jid, ratinginfo, xmpp_stanza_get_text(category), from);
				}
				/*
				else if (variant)
				{
					ModelConn_SetRating(jid, ratinginfo, xmpp_stanza_get_text(variant));
				}
				else
				{
					ModelConn_SetRating(jid, ratinginfo, NULL);
				}
				*/

				Info_DestroyRating(ratinginfo);
			}

			if (node && stricmp(node, "http://jabber.org/protocol/profile") == 0)
			{
				xmpp_stanza_t *field = xmpp_stanza_get_child_by_name(items, "field");

				while (field)
				{
					char *var = xmpp_stanza_get_attribute(field, "var");
					xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(field, "value");

					if (var && stricmp(var, "description") == 0)
					{
						ModelConn_SetProfileDescription(jid, xmpp_stanza_get_text(value));
					}

					field = xmpp_stanza_get_next_by_name(field, "field");
				}
			}

			Conn_HandleProfilePlayingFields(jid, items, 1);

			items = xmpp_stanza_get_next_by_name(items, "items");
		}
	}
	
	if (gamerequest)
	{
		struct gamesearchinfo_s *info = Conn_ParseGameRequest(gamerequest);
		xmpp_stanza_t *gameaccepted = xmpp_stanza_get_child_by_name(gamerequest, "game-accepted");
		xmpp_stanza_t *gamerejected = xmpp_stanza_get_child_by_name(gamerequest, "game-rejected");
		char *from = xmpp_stanza_get_attribute(gamerequest, "from");
		char *gameid = xmpp_stanza_get_attribute(gamerequest, "id");

		if (gameaccepted)
		{
			/*
			xmpp_stanza_t *message, *gamerequest2;

			message = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(message, "message");
			xmpp_stanza_set_id(message, id);
			xmpp_stanza_set_attribute(message, "to", xmpp_stanza_get_attribute(stanza, "from"));

			gamerequest2 = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(gamerequest2, "game-request");
			xmpp_stanza_set_ns(gamerequest2, "http://onlinegamegroup.com/xml/chesspark-01");
			xmpp_stanza_set_id(gamerequest2, xmpp_stanza_get_attribute(gamerequest, "id"));
			xmpp_stanza_set_attribute(gamerequest2, "from", xmpp_stanza_get_attribute(stanza, "to"));
			xmpp_stanza_set_type(gamerequest2, "result");

			xmpp_stanza_add_child(message, gamerequest2);
			xmpp_stanza_release(gamerequest2);
			
			xmpp_send(conn, message);

			xmpp_stanza_release(message);
			*/
			/*ModelConn_JoinCorGame(gameid);*/
			ModelConn_ShowCorGameAccepted(gameid, from);
		}
		else if (gamerejected)
		{
			ModelConn_GameRequestError(from, gameid, "reject", NULL, NULL);
		}
		else if (from)
		{
			ModelConn_RequestMatch(from, info, NULL);
		}

		Info_DestroyGameSearchInfo(info);
	}

	if (gamestart)
	{
		char *gameid = xmpp_stanza_get_attribute(gamestart, "id");

		/*ModelConn_ShowGameStart(gameid);*/
	}

	if (from && (strcmp(from, "login.chesspark.com") == 0))
	{
		Conn_HandleLoginMessage(stanza);
	}

	while (game)
	{
		char *gameid = xmpp_stanza_get_attribute(game, "id");

		char *count  = xmpp_stanza_get_attribute(game, "count");

		char arbitername[120];

		arbitername[0] = '\0';

		if (gameid)
		{
			strcat(arbitername, gameid);
			strcat(arbitername, "@games.chesspark.com/Arbiter");
		}

		/* check for chat and if it's arbiter giving us this information */
		if (type && (strcmp(type, "chat") == 0) && from && (strcmp(from, arbitername) == 0 || strcmp(from, "arbiter.chesspark.com") == 0))
		{
			Conn_HandleGame(game);
		}

		/* check for groupchat and if it's truly the arbiter giving us this information */
		if (type && (strcmp(type, "groupchat") == 0) && from && (strcmp(from, arbitername) == 0))
		{
			Conn_HandleGame(game);
		}
			
/* check for chat and if it's login giving us this information */
/*
<message xmlns="jabber:client" to="jcanete@chesspark.com/build46" from="login.chesspark.com">
 <body>You have correspondence to play.</body>
 <game id="1499" black="test@chesspark.com/cpc" white="jcanete@chesspark.com/build46" xmlns="http://onlinegamegroup.com/xml/chesspark-01" type="correspondence">
  <state>rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e5 0 2</state>
  <last-move side="black" stamp="20060615030256" move="2" player="test@chesspark.com/cpc">e7e5</last-move>
  <variant xmlns="http://onlinegamegroup.com/xml/chesspark-01" name="standard"/>
 </game>
 </message>
*/
		/* no type */
/*
		if (count)
		{
			int icount;
			sscanf(count, "%d", &icount);
			ModelConn_HandleTotalGamesCount(icount);
		}
		*/
		game = xmpp_stanza_get_next_by_name(game, "game");
	}

	while (tournament)
	{
		xmpp_stanza_t *joined          = xmpp_stanza_get_child_by_name(tournament, "joined");
		xmpp_stanza_t *forfeit         = xmpp_stanza_get_child_by_name(tournament, "forfeit");
		xmpp_stanza_t *roundstarted    = xmpp_stanza_get_child_by_name(tournament, "round-started");
		xmpp_stanza_t *roundended      = xmpp_stanza_get_child_by_name(tournament, "round-ended");
		xmpp_stanza_t *pairings        = xmpp_stanza_get_child_by_name(tournament, "pairings");
		xmpp_stanza_t *games           = xmpp_stanza_get_child_by_name(tournament, "games");
		xmpp_stanza_t *game            = xmpp_stanza_get_child_by_name(tournament, "game");
		xmpp_stanza_t *players         = xmpp_stanza_get_child_by_name(tournament, "players");
		xmpp_stanza_t *currentround    = xmpp_stanza_get_child_by_name(tournament, "current-round");
		xmpp_stanza_t *tournamentended = xmpp_stanza_get_child_by_name(tournament, "tournament-ended");

		char *tourneyid = xmpp_stanza_get_attribute(tournament, "id");
		int roundnum = 0;

		if (currentround)
		{
			sscanf(xmpp_stanza_get_text(currentround), "%d", &roundnum);
		}

		if (tourneyid)
		{
			while (joined)
			{
				xmpp_stanza_t *player = xmpp_stanza_get_child_by_name(tournament, "player");

				if (player)
				{
					char *jid = Jid_Strip(xmpp_stanza_get_attribute(player, "jid"));

					if (jid)
					{
						xmpp_stanza_t *score  = xmpp_stanza_get_child_by_name(player, "score");
						xmpp_stanza_t *wins   = xmpp_stanza_get_child_by_name(player, "wins");
						xmpp_stanza_t *draws  = xmpp_stanza_get_child_by_name(player, "draws");
						xmpp_stanza_t *losses = xmpp_stanza_get_child_by_name(player, "losses");
						xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(player, "rating");
						struct tournamentplayerinfo_s *pinfo;

						pinfo = malloc(sizeof(*pinfo));

						memset(pinfo, 0, sizeof(*pinfo));

						pinfo->jid = strdup(jid);

						if (score)
						{
							char *txt = xmpp_stanza_get_text(score);

							if (txt)
							{
								sscanf(txt, "%f", &(pinfo->score));
							}
						}

						if (wins)
						{
							char *txt = xmpp_stanza_get_text(wins);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->wins));
							}
						}

						if (draws)
						{
							char *txt = xmpp_stanza_get_text(draws);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->draws));
							}
						}

						if (losses)
						{
							char *txt = xmpp_stanza_get_text(losses);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->losses));
							}
						}

						if (rating)
						{
							char *txt = xmpp_stanza_get_text(rating);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->rating));
							}
						}

						ModelConn_TournamentAddPlayer(tourneyid, jid);
						ModelConn_TournamentUpdatePlayer(tourneyid, jid, pinfo);
					}
				}

				joined = xmpp_stanza_get_next_by_name(joined, "joined");
			}

			while (forfeit)
			{
				ModelConn_TournamentRemovePlayer(tourneyid, xmpp_stanza_get_text(forfeit));
				forfeit = xmpp_stanza_get_next_by_name(forfeit, "forfeit");
			}

			if (roundstarted)
			{
				struct namedlist_s *pairinglist = NULL;
				int round;
				
				sscanf(xmpp_stanza_get_text(roundstarted), "%d", &round);

				if (pairings)
				{
					xmpp_stanza_t *pairing = xmpp_stanza_get_child_by_name(pairings, "pairing");

					while (pairing)
					{
						char *white = xmpp_stanza_get_attribute(pairing, "white");
						char *black = xmpp_stanza_get_attribute(pairing, "black");
						char *bye   = xmpp_stanza_get_attribute(pairing, "bye");

						struct tournamentpairing_s *pairinfo;

						pairinfo = malloc(sizeof(*pairinfo));

						memset(pairinfo, 0, sizeof(*pairinfo));

						pairinfo->white = strdup(white);
						pairinfo->black = strdup(black);
						pairinfo->bye   = strdup(bye);

						NamedList_Add(&pairinglist, NULL, pairinfo, Info_DestroyTournamentPairing);

						pairing = xmpp_stanza_get_next_by_name(pairing, "pairing");
					}
				}

				if (games)
				{
					xmpp_stanza_t *game = xmpp_stanza_get_child_by_name(games, "game");
					
					while (game)
					{
						xmpp_stanza_t *result = xmpp_stanza_get_child_by_name(game, "result");

						char *white = Jid_Strip(xmpp_stanza_get_attribute(game, "white"));
						char *black = Jid_Strip(xmpp_stanza_get_attribute(game, "black"));
						char *gameid = xmpp_stanza_get_attribute(game, "id");

						struct namedlist_s *entry = pairinglist;

						while (entry)
						{
							struct tournamentpairing_s *pairinfo = entry->data;
							if ( (pairinfo->white && stricmp(pairinfo->white, white) == 0)
							  && (pairinfo->black && stricmp(pairinfo->black, black) == 0))
							{
								if (result)
								{
									char *side = xmpp_stanza_get_attribute(result, "side");
									char *resulttxt = xmpp_stanza_get_text(result);

									pairinfo->gamestate = 2;

									if (side && resulttxt && strcmp(resulttxt, "win") == 0)
									{
										pairinfo->winner = strdup(side);
									}
									else
									{
										pairinfo->winner = strdup("draw");
									}
								}
							}

							entry = entry->next;
						}

						free(white);
						free(black);

						game = xmpp_stanza_get_next_by_name(game, "game");
					}
				}

				ModelConn_TournamentStartRound(tourneyid, round, pairinglist);
			}

			if (roundended)
			{
				int round;
				
				sscanf(xmpp_stanza_get_text(roundended), "%d", &round);

				ModelConn_TournamentEndRound(tourneyid, round);
			}

			if (tournamentended)
			{
				xmpp_stanza_t *winner = xmpp_stanza_get_child_by_name(tournamentended, "winner");
				char *winnerjid = NULL;

				if (winner)
				{
					winnerjid = xmpp_stanza_get_text(winner);
				}

				ModelConn_TournamentEnd(tourneyid, winnerjid);
			}

			while (game)
			{
				xmpp_stanza_t *round          = xmpp_stanza_get_child_by_name(game, "round");
				xmpp_stanza_t *playing        = xmpp_stanza_get_child_by_name(game, "playing");
				xmpp_stanza_t *stoppedplaying = xmpp_stanza_get_child_by_name(game, "stopped-playing");
				xmpp_stanza_t *win            = xmpp_stanza_get_child_by_name(game, "win");
				char *gameid = xmpp_stanza_get_attribute(game, "id");
				char *white = xmpp_stanza_get_attribute(game, "white");
				char *black = xmpp_stanza_get_attribute(game, "black");
				
				/*
				int roundnum = 0;

				if (round)
				{
					sscanf(xmpp_stanza_get_text(round), "%d", &roundnum);			
				}
				*/

				if (playing)
				{
					ModelConn_TournamentGamePlaying(tourneyid, gameid, roundnum, white, black);
				}
				else if (stoppedplaying)
				{
					char *winner = "draw";

					if (win)
					{
						winner = xmpp_stanza_get_text(win);
					}

					ModelConn_TournamentGameStoppedPlaying(tourneyid, gameid, roundnum, white, black, winner);
				}

				game = xmpp_stanza_get_next_by_name(game, "game");
			}

			if (players)
			{
				xmpp_stanza_t *player = xmpp_stanza_get_child_by_name(players, "player");
				
				while (player)
				{
					char *jid = xmpp_stanza_get_attribute(player, "jid");

					if (jid)
					{
						xmpp_stanza_t *score  = xmpp_stanza_get_child_by_name(player, "score");
						xmpp_stanza_t *wins   = xmpp_stanza_get_child_by_name(player, "wins");
						xmpp_stanza_t *draws  = xmpp_stanza_get_child_by_name(player, "draws");
						xmpp_stanza_t *losses = xmpp_stanza_get_child_by_name(player, "losses");
						xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(player, "rating");
						struct tournamentplayerinfo_s *pinfo;

						pinfo = malloc(sizeof(*pinfo));

						memset(pinfo, 0, sizeof(*pinfo));

						pinfo->jid = strdup(jid);

						if (score)
						{
							char *txt = xmpp_stanza_get_text(score);

							if (txt)
							{
								sscanf(txt, "%f", &(pinfo->score));
							}
						}

						if (wins)
						{
							char *txt = xmpp_stanza_get_text(wins);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->wins));
							}
						}

						if (draws)
						{
							char *txt = xmpp_stanza_get_text(draws);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->draws));
							}
						}

						if (losses)
						{
							char *txt = xmpp_stanza_get_text(losses);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->losses));
							}
						}

						if (rating)
						{
							char *txt = xmpp_stanza_get_text(rating);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->rating));
							}
						}

						ModelConn_TournamentUpdatePlayer(tourneyid, jid, pinfo);
					}

					player = xmpp_stanza_get_next_by_name(player, "player");
				}
			}

		}

		tournament = xmpp_stanza_get_next_by_name(tournament, "tournament");
	}

/*
<message from='search.chesspark.com'>
	<search xmlns='http://onlinegamegroup.com/xml/chesspark-01'
	        type='watch'>
		<item id='1234' jid='1234@games.chesspark.com' node='watch'>
			<game white='white@blah.blah" black='black@blah.blah'
				white-rating='blah' black-rating='blah'
				white-award='blah' black-award='blah'
				white-title='blah' black-title='blah'
				id='1234' room='1234@blah.blah'>
				<variant name="blah"/>
				<time-control delayinc="delay">
					<control><moves>40</moves><time>7200</time></control>
					<control><time>1800</time></control>
				</time-control>
				<time-control-range name='standard|speed|blitz|bullet'/>
				<rated/>
				<computers/>
				<limit/>
			</game>
		</item>
	</search>
</message>
*/
	if (search && from && stricmp(from, "search.chesspark.com") == 0)
	{
		xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(search, "item");
		char *type = xmpp_stanza_get_attribute(search, "type");

		while (item)
		{
			xmpp_stanza_t *game    = xmpp_stanza_get_child_by_name(item, "game");
			xmpp_stanza_t *removed = xmpp_stanza_get_child_by_name(item, "removed");
			char *id = xmpp_stanza_get_attribute(item, "id");
			char *node = xmpp_stanza_get_attribute(item, "node");
			struct gamesearchinfo_s *info = NULL;

			if (removed)
			{
				ModelConn_RemovePushGame(node, id);
			}
			else if (game)
			{
				info = Conn_ParseGameSearchInfo(game);
				ModelConn_AddPushGame(node, id, info);
			}

			item = xmpp_stanza_get_next_by_name(item, "item");
		}
	}
/*
   <groups xmlns='cpns'>
         <group name='test' chat='test' type='public' forum='test' >
            <roles/> <!-- similar to chesspark main roles -->
            <avatar>sha2 digest data of the groups avatar</avatar>
            <titles/> <!-- similar to chesspark main titles -->
            <permissions>
            <!-- similar to other chesspark permissions   -->
            </premissions>
         </group>
   </groups>
*/
	if (groups)
	{
		xmpp_stanza_t *group = xmpp_stanza_get_child_by_name(groups, "group");
		struct namedlist_s *groupslist = NULL;

		while (group)
		{
			struct groupinfo_s *ginfo = Conn_ParseGroupStanza(group);

			NamedList_Add(&groupslist, ginfo->id, ginfo, Info_DestroyGroupInfo);

			group = xmpp_stanza_get_next_by_name(group, "group");
		}

		ModelConn_HandleGroups(groupslist);
	}
	return 1;
}

/*
<presence xmlns="jabber:client" to="loadtester573@dev.chesspark.com/cpc" from="centralpark@chat.chesspark.com/loadtester573@dev.chesspark.com">
 <x xmlns="http://jabber.org/protocol/muc"/>
 <x xmlns="http://jabber.org/protocol/muc#user">
  <item role="participant" jid="loadtester573@dev.chesspark.com/cpc" affiliation="none"/>
 </x>
 <x xmlns="http://jabber.org/protocol/muc"/>
 <x jid="loadtester573@dev.chesspark.com/cpc" xmlns="http://onlinegamegroup.com/xml/chesspark-01">
  <not-activated/><member-type>pro</member-type><roles/><titles/>
 </x></presence>
*/
int Conn_HandlePresence(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name, *from;
	xmpp_stanza_t *error, *x;
	int ismuc = 0;
	char *role = NULL;
	char *affiliation = NULL;
	char *realjid = NULL;
	char *vcardphotohash = NULL;
	char *nickchange = NULL;
	char *actorjid = NULL;
	char *reasontxt = NULL;
	int statuscode = 0;
	struct namedlist_s *roleslist = NULL;
	struct namedlist_s *titleslist = NULL;
	int notactivated = 0;
	char *membertype = NULL;


	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	from = xmpp_stanza_get_attribute(stanza, "from");
	error = xmpp_stanza_get_child_by_name(stanza, "error");
	Log_Write(0, "Conn_HandlePresence(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	x = xmpp_stanza_get_child_by_name(stanza, "x");
	
	while (x)
	{
		char *ns = xmpp_stanza_get_ns(x);
				
		if (ns)
		{
			if (strcmp(ns, "http://jabber.org/protocol/muc") == 0)
			{
				ismuc = 1;
			} 
		}

		x = xmpp_stanza_get_next_by_name(x, "x");
	}

	if (error)
	{
		char *code = xmpp_stanza_get_attribute(error, "code");

		if (ismuc)
		{
			ModelConn_MucError(from, code);
		}

		if (from && stricmp(from, "login.chesspark.com") == 0)
		{
			Conn_HandleLoginMessage(stanza);
		}

		return 1;
	}

	x = xmpp_stanza_get_child_by_name(stanza, "x");

	while (x)
	{
		char *ns = xmpp_stanza_get_ns(x);
				
		if (ns)
		{
			if (strcmp(ns, "http://jabber.org/protocol/muc#user") == 0)
			{
				xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, "item");
				xmpp_stanza_t *statusstanza = xmpp_stanza_get_child_by_name(x, "status");

				ismuc = 1;

				if (item)
				{
					xmpp_stanza_t *actor = xmpp_stanza_get_child_by_name(item, "actor");
					xmpp_stanza_t *reason = xmpp_stanza_get_child_by_name(item, "reason");

					if (actor)
					{
						actorjid = xmpp_stanza_get_attribute(actor, "jid");
					}

					if (reason)
					{
						reasontxt = xmpp_stanza_get_text(reason);
					}

					role = xmpp_stanza_get_attribute(item, "role");
					affiliation = xmpp_stanza_get_attribute(item, "affiliation");
					realjid = xmpp_stanza_get_attribute(item, "jid");
					nickchange = xmpp_stanza_get_attribute(item, "nick");
				}

				if (statusstanza)
				{
					statuscode = atoi(xmpp_stanza_get_attribute(statusstanza, "code"));
				}
			}
			else if (strcmp(ns, "vcard-temp:x:update") == 0)
			{
				xmpp_stanza_t *photo = xmpp_stanza_get_child_by_name(x, "photo");
				if (photo)
				{
					vcardphotohash = xmpp_stanza_get_text(photo);
				}
			}
			else if (strcmp(ns, "http://onlinegamegroup.com/xml/chesspark-01") == 0)
			{
				xmpp_stanza_t *roles         = xmpp_stanza_get_child_by_name(x, "roles");
				xmpp_stanza_t *titles        = xmpp_stanza_get_child_by_name(x, "titles");
				xmpp_stanza_t *snotactivated = xmpp_stanza_get_child_by_name(x, "not-activated");
				xmpp_stanza_t *smembertype   = xmpp_stanza_get_child_by_name(x, "member-type");
				char *realjid2 = xmpp_stanza_get_attribute(x, "jid");

				if (realjid2 && realjid2[0])
				{
					realjid = realjid2;
				}

				if (roles)
				{
					xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(roles, "role");

					while (role)
					{
						NamedList_AddString(&roleslist, xmpp_stanza_get_text(role), xmpp_stanza_get_text(role));
						role = xmpp_stanza_get_next_by_name(role, "role");
					}
				}

				if (titles)
				{
					xmpp_stanza_t *title = xmpp_stanza_get_child_by_name(titles, "title");

					while (title)
					{
						NamedList_AddString(&titleslist, xmpp_stanza_get_text(title), xmpp_stanza_get_text(title));
						title = xmpp_stanza_get_next_by_name(title, "title");
					}
				}

				if (snotactivated)
				{
					notactivated = 1;
				}

				if (smembertype)
				{
					membertype = xmpp_stanza_get_text(smembertype);
				}
			}
		}

		x = xmpp_stanza_get_next_by_name(x, "x");
	}

	if (type && strcmp(type, "unsubscribe") == 0)
	{
		char *from = xmpp_stanza_get_attribute(stanza, "from");
		if (from)
		{
			Model_QueueMessage(MPMessage_Create(MODELMSG_ASKUNSUBSCRIBE, strdup(from), NULL, NamedList_FreeString, NULL, 0), 1);
			/*ModelConn_AskUnsubscribe(from);*/
		}
	}
	else if (type && strcmp(type, "unsubscribed") == 0)
	{
		char *from = xmpp_stanza_get_attribute(stanza, "from");
		if (from)
		{
			Model_QueueMessage(MPMessage_Create(MODELMSG_SETUNSUBSCRIBED, strdup(from), NULL, NamedList_FreeString, NULL, 0), 1);
			/*ModelConn_SetUnsubscribed(from);*/
		}
	}
	else if (type && strcmp(type, "subscribed") == 0)
	{
		char *from = xmpp_stanza_get_attribute(stanza, "from");
		if (from)
		{
			Model_QueueMessage(MPMessage_Create(MODELMSG_SETSUBSCRIBED, strdup(from), NULL, NamedList_FreeString, NULL, 0), 1);
			/*ModelConn_SetSubscribed(from);*/
		}
	}
	else if (type && stricmp(type, "subscribe") == 0)
	{
		if (from)
		{
			Model_QueueMessage(MPMessage_Create(MODELMSG_ASKSUBSCRIBE, strdup(from), NULL, NamedList_FreeString, NULL, 0), 1);
			/*ModelConn_AskApprove(from);*/
		}
	}
	else
	{
		if (from)
		{
			enum SStatus sstatus = SSTAT_AVAILABLE;
			char *statusmsg = NULL;
			xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, "show");
			xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, "status");
			xmpp_stanza_t *priority = xmpp_stanza_get_child_by_name(stanza, "priority");
			char *type = xmpp_stanza_get_attribute(stanza, "type");
			int prioritynum = 0;

			if (priority)
			{
				sscanf(xmpp_stanza_get_text(priority), "%d", &prioritynum);
			}
			if (show)
			{
				char *showtext = xmpp_stanza_get_text(show);

				if (stricmp(showtext, "chat") == 0)
				{
					sstatus = SSTAT_AVAILABLE;
				}
				else if (stricmp(showtext, "away") == 0)
				{
					sstatus = SSTAT_AWAY;
				}
				else if (stricmp(showtext, "xa") == 0)
				{
					sstatus = SSTAT_AWAY;
				}
				else if (stricmp(showtext, "dnd") == 0)
				{
					sstatus = SSTAT_AWAY;
				}
			}

			if (status)
			{
				statusmsg = xmpp_stanza_get_text(status);
			}

			if (type && strcmp(type, "unavailable") == 0)
			{
				sstatus = SSTAT_OFFLINE;
			}

			if (statuscode != 303)
			{
				nickchange = NULL;
			}

			{
				struct presenceinfo_s *info;

				info = malloc(sizeof(*info));
				memset(info, 0, sizeof(*info));

				info->jid            = strdup(from);
				info->ismuc          = ismuc;
				info->status         = sstatus;
				info->statusmsg      = strdup(statusmsg);
				info->role           = strdup(role);
				info->affiliation    = strdup(affiliation);
				info->realjid        = strdup(realjid);
				info->statuscode     = statuscode;
				info->vcardphotohash = strdup(vcardphotohash);
				info->nickchange     = strdup(nickchange);
				info->roleslist      = roleslist;
				info->titleslist     = titleslist;
				info->prioritynum    = prioritynum;
				info->actorjid       = strdup(actorjid);
				info->reason         = strdup(reasontxt);
				info->notactivated   = notactivated;
				info->membertype     = strdup(membertype);

				Model_QueueMessage(MPMessage_Create(MODELMSG_HANDLEPRESENCE, info, NULL, PresenceInfo_Destroy, NULL, 0), 1);
			}
/*
			ModelConn_SetFriendPresence(from, ismuc, sstatus,
			  statusmsg, role, affiliation, realjid, statuscode,
			  vcardphotohash, nickchange, roleslist, titleslist,
			  prioritynum, actorjid, reasontxt);
*/
		}
	}

	return 1;
}


void Conn_Poll()
{
	if (!ctx && ConnectState != CONN_NOTCONNECTED)
	{
		ConnectState = CONN_NOTCONNECTED;
	}

	if (ConnectState == CONN_CONNECTING || ConnectState == CONN_CONNECTED || ConnectState == CONN_DISCONNECTING)
	{
		int tickcount = GetTickCount();
		static int oldtickcount = 0;

		if (keepalive && (!oldtickcount || tickcount - 60 * 1000 > oldtickcount))
		{
			xmpp_stanza_t *space = xmpp_stanza_new(ctx);
			oldtickcount = tickcount;

			xmpp_stanza_set_text(space, " ");

			xmpp_send(conn, space);

			xmpp_stanza_release(space);
		}

		xmpp_run_once(ctx, 1);

		if (ConnectState == CONN_DISCONNECTED)
		{
			Conn_Reset();
			ModelConn_LostConnection(ConnectError);
		}
	}
}


void Conn_Handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
		  const int error, xmpp_stream_error_t * const stream_error,
		  void * const userdata)
{
	if (status == XMPP_CONN_CONNECT) {
		Log_Write(0, "Conn_Handler(): status changed to XMPP_CONN_CONNECT\n");
		
		/* Setup message handler */
		xmpp_handler_add(conn, Conn_HandleMessage, NULL, "message", NULL, NULL);

		/* Setup presence handler */
		xmpp_handler_add(conn, Conn_HandlePresence, NULL, "presence", NULL, NULL);

		/* Setup Iq handler */
		xmpp_handler_add(conn, Conn_HandleIq, NULL, "iq", NULL, NULL);

		ModelConn_Login(userdata);
		ConnectState = CONN_CONNECTED;
	} else if (status == XMPP_CONN_DISCONNECT) {
		Log_Write(0, "Conn_Handler(): status changed to XMPP_CONN_DISCONNECT\n");
		ConnectState = CONN_DISCONNECTED;
	} else if (status == XMPP_CONN_FAIL) {
		Log_Write(0, "Conn_Handler(): status changed to XMPP_CONN_FAIL\n");
		ConnectState = CONN_DISCONNECTED;
	}

	Log_Write(0, "Connect error %d\n", error);

	if (ConnectState == CONN_DISCONNECTED)
	{
		if (error == WSAETIMEDOUT)
		{
			ConnectError = CONNERROR_CANTCONNECT;
		} 
		else if (intentionaldisconnect)
		{
			intentionaldisconnect = 0;
			ConnectError = CONNERROR_NONE;
		}
		else
		{
			ConnectError = CONNERROR_LOSTCONNECTION;
		}
 
		if (stream_error)
		{
			Log_Write(0, "Stream error type %d\n", stream_error->type);

			switch (stream_error->type)
			{
				case XMPP_SE_CONN_TIMEOUT:
				case XMPP_SE_HOST_GONE:
				case XMPP_SE_HOST_UNKNOWN:
				case XMPP_SE_IMPROPER_ADDR:
				case XMPP_SE_INTERNAL_SERVER_ERROR:
				case XMPP_SE_REMOTE_CONN_FAILED:
				case XMPP_SE_SEE_OTHER_HOST:
					ConnectError = CONNERROR_CANTCONNECT;
					break;
				case XMPP_SE_CONFLICT:
					ConnectError = CONNERROR_CONFLICT;
					break;
				case XMPP_SE_NOT_AUTHORIZED:
					ConnectError = CONNERROR_BADPASSWORD;
					break;
				default:
					ConnectError = CONNERROR_UNKNOWN;
					break;
			}
		}
	
	}
}


void Conn_Login(char *jid, char *pass)
{
	char *url, *p;
	char *altdomain = NULL;

	url = strdup(strrchr(jid, '@') + 1);

	p = strchr(url, '/');

	if (p)
	{
		*p = '\0';
	}

	/* create a connection */
	conn = xmpp_conn_new(ctx);

	/* setup authentication information */
	xmpp_conn_set_jid(conn, jid);
	xmpp_conn_set_pass(conn, pass);

	/* initiate connection */
	if (stricmp(url, "chesspark.com") == 0)
	{
		altdomain = "xmpp.chesspark.com";
	}

	Model_SetLoginStartTime();

	if (xmpp_connect_client(conn, url, altdomain, 5222, Conn_Handler, strdup(jid)))
	{
		Conn_Reset();
		ModelConn_LostConnection(CONNERROR_CANTCONNECT);
	}
	else
	{
                ConnectState = CONN_CONNECTING;
	}

	intentionaldisconnect = 0;

	free(url);
}


xmpp_stanza_t *Conn_CreateIQStanza(char *type, char *id, char *to)
{
	xmpp_stanza_t *iq = xmpp_stanza_new(ctx);

	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_type(iq, type);
	xmpp_stanza_set_id(iq, id);
	if (Ctrl_GetFullLoginJid())
	{
		xmpp_stanza_set_attribute(iq, "from", Ctrl_GetFullLoginJid());
	}
	
	if (to)
	{
		xmpp_stanza_set_attribute(iq, "to", to);
	}

	return iq;
}


void Conn_GetRoster()
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "roster_get");

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("get", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

	xmpp_stanza_add_child(iq, query);

	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, Conn_HandleRoster, id, NULL);

	Model_ShowLoginTime("getroster");

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
}

void Conn_GetRooms()
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "rooms_get");

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("get", id, "chat.chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#items");

	xmpp_stanza_add_child(iq, query);

	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, Conn_HandleRoomsList, id, NULL);

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
}

void Conn_GetRoomInfo(char *jid)
{
	xmpp_stanza_t *iq, *query;

	char id[256];

	Conn_GenID(id, "roominfo_get:");
	strncat(id, jid, 255);

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");

	xmpp_stanza_add_child(iq, query);

	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, Conn_HandleRoomInfo, id, NULL);

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
}

void Conn_GetInfo()
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "info_get");

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("get", id, "chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");

	xmpp_stanza_add_child(iq, query);

	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, Conn_HandleRoomsList, id, NULL);

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);	
}

void Conn_SetFriend(char *jid, char *name, struct namedlist_s *grouplist)
{
	xmpp_stanza_t *iq, *query, *item, *group, *grouptext;
	char id[256];

	Conn_GenID(id, "roster_set");

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

	item = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "jid", jid);

	if (name)
	{
		xmpp_stanza_set_attribute(item, "name", name);
	}

	while (grouplist)
	{
		char *text = EscapeParsedText(grouplist->name);
		group = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(group, "group");

		grouptext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(grouptext, text);

		xmpp_stanza_add_child(group, grouptext);
		xmpp_stanza_add_child(item, group);
	
		xmpp_stanza_release(grouptext);
		xmpp_stanza_release(group);

		grouplist = grouplist->next;
		free(text);
	}
	
	xmpp_stanza_add_child(query, item);
	xmpp_stanza_add_child(iq, query);
	
	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(item);
	xmpp_stanza_release(query);

	/* set up reply handler */
	/*xmpp_id_handler_add(conn, Conn_HandleIq, id, NULL);*/
	
	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
}

void Conn_RemoveFriend(char *jid)
{
	xmpp_stanza_t *iq, *query, *item;
	char id[256];

	Conn_GenID(id, "roster_remove");

	/* create iq stanza for request */
	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

	item = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "jid", jid);
	xmpp_stanza_set_attribute(item, "subscription", "remove");

	xmpp_stanza_add_child(query, item);
	xmpp_stanza_add_child(iq, query);
	
	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(item);
	xmpp_stanza_release(query);

	/* set up reply handler */
	/*xmpp_id_handler_add(conn, Conn_HandleIq, id, NULL);*/

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
}

void Conn_SendMessage(char *tojid, char *text, int isGroupChat)
{
	xmpp_stanza_t *message, *body, *bodytext, *x, *composing;
	char id[256];

	Conn_GenID(id, "message_send");

	/* create message stanza */
	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	if (isGroupChat)
	{
		xmpp_stanza_set_type(message, "groupchat");
	}
	else
	{
		xmpp_stanza_set_type(message, "chat");
	}
	xmpp_stanza_set_id(message, id);
	xmpp_stanza_set_attribute(message, "to", tojid);

	body = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(body, "body");

	bodytext = xmpp_stanza_new(ctx);
	text = EscapeParsedText(text);
	xmpp_stanza_set_text(bodytext, text);

	xmpp_stanza_add_child(body, bodytext);
	xmpp_stanza_add_child(message, body);
	
	xmpp_stanza_release(bodytext);
	xmpp_stanza_release(body);

	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:event");

	if (!isGroupChat)
	{
		composing = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(composing, "composing");

		xmpp_stanza_add_child(x, composing);
		xmpp_stanza_release(composing);
	}

	xmpp_stanza_add_child(message, x);
	xmpp_stanza_release(x);
	
	/* send out the stanza */
	xmpp_send(conn, message);

	/* release the stanza */
	xmpp_stanza_release(message);
}

void Conn_SendPresence(enum SStatus fstatus, char *statusmsg, char *tojid, int ismuc, char *avatarhash, int prioritynum, int inclpriority)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");

	if (tojid)
	{
		xmpp_stanza_set_attribute(presence, "to", tojid);
	}

	if (ismuc)
	{
		xmpp_stanza_t *x;

		x = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(x, "x");
		xmpp_stanza_set_ns(x, "http://jabber.org/protocol/muc");

		xmpp_stanza_add_child(presence, x);

		xmpp_stanza_release(x);
	}

	if (fstatus == SSTAT_OFFLINE)
	{
		xmpp_stanza_set_type(presence, "unavailable");
	}
	else if (fstatus != SSTAT_AVAILABLE)
	{
		xmpp_stanza_t *show, *showtext;

		show = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(show, "show");

		showtext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(showtext, "away");

		xmpp_stanza_add_child(show, showtext);
		xmpp_stanza_add_child(presence, show);

		xmpp_stanza_release(showtext);
		xmpp_stanza_release(show);
	}

	if (statusmsg)
	{
		xmpp_stanza_t *status, *statustext;
		char *text = EscapeParsedText(statusmsg);

		status = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(status, "status");

		statustext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(statustext, text);

		xmpp_stanza_add_child(status, statustext);
		xmpp_stanza_add_child(presence, status);

		xmpp_stanza_release(statustext);
		xmpp_stanza_release(status);

		free(text);

	}

	if (avatarhash)
	{
		xmpp_stanza_t *x, *photo, *phototext;
 
		x = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(x, "x");
		xmpp_stanza_set_ns(x, "vcard-temp:x:update");

		photo = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(photo, "photo");


		phototext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(phototext, avatarhash);

		xmpp_stanza_add_child(photo, phototext);
		xmpp_stanza_add_child(x, photo);
		xmpp_stanza_add_child(presence, x);

		xmpp_stanza_release(phototext);
		xmpp_stanza_release(photo);
		xmpp_stanza_release(x);
	}

	if (inclpriority)
	{
		xmpp_stanza_t *priority, *prioritytext;
		char txt[256];

		sprintf(txt, "%d", prioritynum);

		priority = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(priority, "priority");

		prioritytext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(prioritytext, txt);

		xmpp_stanza_add_child(priority, prioritytext);
		xmpp_stanza_release(prioritytext);

		xmpp_stanza_add_child(presence, priority);
		xmpp_stanza_release(priority);
	}


	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

/*
<presence to="login.chesspark.com"/>
  <client-info xmlns='cpns'>
    <vender>Chesspark.com</vender>
    <name>windows cool version</vender>
    <version>0.1a</version>
    <os>WIndows XP Build 04123</os>
  </client-info>
</presence>
*/

void Conn_SendChessparkLogin()
{
	char versiontxt[256];
	xmpp_stanza_t *presence, *clientinfo, *vendor, *name, *version, *os;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");

	xmpp_stanza_set_attribute(presence, "to", "login.chesspark.com");

	clientinfo = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(clientinfo, "client-info");
	xmpp_stanza_set_ns(clientinfo, "http://onlinegamegroup.com/xml/chesspark-01");

	vendor = Conn_CreateTextStanza("vender", "Chesspark.com");
	xmpp_stanza_add_child(clientinfo, vendor);
	xmpp_stanza_release(vendor);

	name = Conn_CreateTextStanza("name", "ChessparkClient");
	xmpp_stanza_add_child(clientinfo, name);
	xmpp_stanza_release(name);

	sprintf(versiontxt, "%s build %s", CHESSPARK_VERSION, CHESSPARK_BUILD);
	version = Conn_CreateTextStanza("version", versiontxt);
	xmpp_stanza_add_child(clientinfo, version);
	xmpp_stanza_release(version);

	os = Conn_CreateTextStanza("os", Util_WinVerText());
	xmpp_stanza_add_child(clientinfo, os);
	xmpp_stanza_release(os);

	xmpp_stanza_add_child(presence, clientinfo);
	xmpp_stanza_release(clientinfo);

	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

void Conn_SendProbe(char *tojid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");

	if (tojid)
	{
		xmpp_stanza_set_attribute(presence, "to", tojid);
	}

	xmpp_stanza_set_type(presence, "probe");

	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

void Conn_RequestAvatar(char *jid)
{
	xmpp_stanza_t *iq, *query;

	char id[256];

	Conn_GenID(id, "avatar_get");

	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:avatar");

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_send(conn, iq);
	xmpp_stanza_release(iq);
}

int Conn_HandleRequestvCardReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name, *from;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	from = xmpp_stanza_get_attribute(stanza, "from");
	Log_Write(0, "Conn_HandleRequestvCardReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (!from)
	{
		from = userdata;
	}

	if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *vcard = xmpp_stanza_get_child_by_name(stanza, "vCard");

		if (vcard)
		{
			xmpp_stanza_t *photo = xmpp_stanza_get_child_by_name(vcard, "PHOTO");

			if (photo)
			{
				xmpp_stanza_t *phototype = xmpp_stanza_get_child_by_name(photo, "TYPE");
				xmpp_stanza_t *binval = xmpp_stanza_get_child_by_name(photo, "BINVAL");
				char *typetext = NULL;
				char *binvaltext = NULL;
				char *binvaldata = NULL;

				if (phototype)
				{
					typetext = xmpp_stanza_get_text(phototype);
				}

				if (binval)
				{
					binvaltext = xmpp_stanza_get_text(binval);

					if (binvaltext)
					{
						int len = 0;
						unsigned char *working, *p, *q;

						p = binvaltext;
						while(*p)
						{
							if (strchr(Base64, *p))
							{
								len++;
							}
							p++;
						}
						
						working = malloc(len + 1);
						p = binvaltext;
						q = working;
						while(*p)
						{
							if (strchr(Base64, *p))
							{
								*q++ = *p;
							}
							p++;
						}
						*q = 0;

						len = util_base64_decoded_len(working, (int)strlen(working));
						binvaldata = util_base64_decode(working, (int)strlen(working));

						if (binvaldata)
						{
							ModelConn_HandlevCardPhoto(from, typetext, binvaldata, len);
						}
					}
				}
			}
		}
	}
	return 0;
}

void Conn_RequestvCard(char *jid)
{
	xmpp_stanza_t *iq, *vCard;
	char id[256];

	Conn_GenID(id, "vCard_get");

	iq = Conn_CreateIQStanza("get", id, jid);

	vCard = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(vCard, "vCard");
	xmpp_stanza_set_ns(vCard, "vcard-temp");

	xmpp_stanza_add_child(iq, vCard);
	xmpp_stanza_release(vCard);

	xmpp_id_handler_add(conn, Conn_HandleRequestvCardReturn, id, strdup(jid));

	xmpp_send(conn, iq);
	xmpp_stanza_release(iq);
}

void Conn_PublishvCard(unsigned char *avatardata, int avatarlen, char *avatartype)
{
	xmpp_stanza_t *iq, *vCard, *photo, *type, *typetext, *binval, *binvaltext;
	char id[256];

	Conn_GenID(id, "vCard_set");

	iq = Conn_CreateIQStanza("set", id, NULL);

	vCard = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(vCard, "vCard");
	xmpp_stanza_set_ns(vCard, "vcard-temp");

	photo = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(photo, "PHOTO");

	type = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(type, "TYPE");

	typetext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(typetext, avatartype);

	xmpp_stanza_add_child(type, typetext);
	xmpp_stanza_release(typetext);

	xmpp_stanza_add_child(photo, type);
	xmpp_stanza_release(type);

	binval = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(binval, "BINVAL");

	binvaltext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(binvaltext, util_base64_encode(avatardata, avatarlen));

	xmpp_stanza_add_child(binval, binvaltext);
	xmpp_stanza_release(binvaltext);

	xmpp_stanza_add_child(photo, binval);
	xmpp_stanza_release(binval);

	xmpp_stanza_add_child(vCard, photo);
	xmpp_stanza_release(photo);

	xmpp_stanza_add_child(iq, vCard);
	xmpp_stanza_release(vCard);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_RequestFriend(char *jid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_type(presence, "subscribe");
	xmpp_stanza_set_attribute(presence, "to", jid);
	
	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}


void Conn_ApproveFriend(char *jid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_type(presence, "subscribed");
	xmpp_stanza_set_attribute(presence, "to", jid);
	
	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}


void Conn_UnsubscribeToFriend(char *jid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_type(presence, "unsubscribed");
	xmpp_stanza_set_attribute(presence, "to", jid);
	
	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

void Conn_UnsubscribeFromFriend(char *jid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_type(presence, "unsubscribe");
	xmpp_stanza_set_attribute(presence, "to", jid);
	
	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

void Conn_RejectFriend(char *jid)
{
	xmpp_stanza_t *presence;

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_type(presence, "unsubscribed");
	xmpp_stanza_set_attribute(presence, "to", jid);
	
	xmpp_send(conn, presence);
	xmpp_stanza_release(presence);
}

enum connstate_e Conn_GetConnState()
{
	return ConnectState;
}

int Conn_HandleSetTopicReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name, *from;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	from = xmpp_stanza_get_attribute(stanza, "from");
	Log_Write(0, "Conn_HandleSetTopicReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "error") == 0)
	{
		char *room = userdata;
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			char *code = xmpp_stanza_get_attribute(error, "code");

			if (code)
			{
				if (stricmp(code, "403") == 0)
				{
                                        ModelConn_SetTopicError(room);
				}
				else
				{
					ModelConn_MucError(room, NULL);
				}
			}
			else
			{
				ModelConn_MucError(room, NULL);
			}
		}
		else
		{
			ModelConn_MucError(room, NULL);
		}
	}

	return 1;
}

void Conn_SetTopic(char *tojid, char *topic)
{
	xmpp_stanza_t *message, *subject, *subjecttext;
	char id[256];
	char *text;

	Conn_GenID(id, "topic_set");

	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_type(message, "groupchat");
	xmpp_stanza_set_id(message, id);
	xmpp_stanza_set_attribute(message, "to", tojid);

	subject = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(subject, "subject");

	if (topic)
	{
		subjecttext = xmpp_stanza_new(ctx);
		text = EscapeParsedText(topic);
		xmpp_stanza_set_text(subjecttext, text);
		xmpp_stanza_add_child(subject, subjecttext);
		xmpp_stanza_release(subjecttext);
	}

	xmpp_stanza_add_child(message, subject);
	xmpp_stanza_release(subject);

	xmpp_id_handler_add(conn, Conn_HandleSetTopicReturn, id, strdup(tojid));

	/* send out the stanza */
	xmpp_send(conn, message);

	/* release the stanza */
	xmpp_stanza_release(message);
}

int Conn_HandleModerationReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *opponentjid = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleModerationReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (stricmp(type, "error") == 0))
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");
		char *room = userdata;

		if (error)
		{
			char *code = xmpp_stanza_get_attribute(error, "code");

			if (code)
			{
				if (stricmp(code, "403") == 0)
				{
					ModelConn_MucModerationError(room);
				}
				else
				{
					ModelConn_MucError(room, code);
				}
			}
			else
			{
				ModelConn_MucError(room, NULL);
			}
		}
		else
		{
			ModelConn_MucError(room, NULL);
		}
	}

	return 1;
}

void Conn_SetRole(char *roomjid, char *nick, char *role, char *comment, char *idtag)
{
	xmpp_stanza_t *iq, *query, *item;
	char id[256];

	if (!nick)
		return;

	Conn_GenID(id, idtag);

	iq = Conn_CreateIQStanza("set", id, roomjid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#admin");

	item = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "nick", nick);
	xmpp_stanza_set_attribute(item, "role", role);

	if (comment)
	{
		xmpp_stanza_t *reason, *reasontext;
		char *text = EscapeParsedText(comment);

		reason = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(reason, "reason");

		reasontext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(reasontext, text);

		xmpp_stanza_add_child(reason, reasontext);
		xmpp_stanza_add_child(item, reason);

		xmpp_stanza_release(reasontext);
		xmpp_stanza_release(reason);

		free(text);
	}

	xmpp_stanza_add_child(query, item);
	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(item);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleModerationReturn, id, strdup(roomjid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


void Conn_GiveMod(char *roomjid, char *nick, char *comment)
{
	Conn_SetRole(roomjid, nick, "moderator", comment, "mod");
}


void Conn_RevokeMod(char *roomjid, char *nick, char *comment)
{
	Conn_SetRole(roomjid, nick, "participant", comment, "unmod");
}


void Conn_GiveVoice(char *roomjid, char *nick, char *comment)
{
	Conn_SetRole(roomjid, nick, "participant", comment, "voice");
}


void Conn_RevokeVoice(char *roomjid, char *nick, char *comment)
{
	Conn_SetRole(roomjid, nick, "visitor", comment, "unvoice");
}


void Conn_KickUser(char *roomjid, char *nick, char *comment)
{
	Conn_SetRole(roomjid, nick, "none", comment, "kick");
}


void Conn_SetAffiliation(char *roomjid, char *userjid, char *affiliation, char *comment, char *idtag)
{
	xmpp_stanza_t *iq, *query, *item;
	char id[256];

	if (!userjid)
		return;

	Conn_GenID(id, idtag);

	iq = Conn_CreateIQStanza("set", id, roomjid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#admin");

	item = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "affiliation", affiliation);
	xmpp_stanza_set_attribute(item, "jid", userjid);

	if (comment)
	{
		xmpp_stanza_t *reason, *reasontext;
		char *text = EscapeParsedText(comment);

		reason = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(reason, "reason");

		reasontext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(reasontext, text);

		xmpp_stanza_add_child(reason, reasontext);
		xmpp_stanza_add_child(item, reason);

		xmpp_stanza_release(reasontext);
		xmpp_stanza_release(reason);

		free(text);
	}

	xmpp_stanza_add_child(query, item);
	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(item);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleModerationReturn, id, strdup(roomjid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_GiveOwner(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "owner", comment, "owned");
}


void Conn_RevokeOwner(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "admin", comment, "unowned");
}

void Conn_GiveAdmin(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "admin", comment, "admin");
}


void Conn_RevokeAdmin(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "none", comment, "unadmin");
}


void Conn_BanUser(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "outcast", comment, "ban");
}


void Conn_UnbanUser(char *roomjid, char *userjid, char *comment)
{
	Conn_SetAffiliation(roomjid, userjid, "none", comment, "unban");
}

int Conn_HandleClearChatHistoryReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *room = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleModerationReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (stricmp(type, "error") == 0))
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			char *code = xmpp_stanza_get_attribute(error, "code");

			if (code)
			{
				if (stricmp(code, "403") == 0)
				{
					ModelConn_MucModerationError(room);
				}
				else
				{
					ModelConn_MucError(room, code);
				}
			}
			else
			{
				ModelConn_MucError(room, NULL);
			}
		}
		else
		{
			ModelConn_MucError(room, NULL);
		}
	}
	else if (type && (stricmp(type, "result") == 0))
	{
		ModelConn_HandleClearChatHistorySuccess(room);
	}
	return 1;
}

/*
<iq type='set' to='room@server' id='exec1'>
           <command xmlns='http://jabber.org/protocol/commands'
                   node='clearhistory'
                   action='execute'/>
        </iq>
*/

void Conn_ClearChatHistory(char *roomjid)
{
	xmpp_stanza_t *iq, *command;
	char id[256];

	Conn_GenID(id, "clearhistory");

	iq = Conn_CreateIQStanza("set", id, roomjid);

	command = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(command, "command");
	xmpp_stanza_set_ns(command, "http://jabber.org/protocol/commands");
	xmpp_stanza_set_attribute(command, "node", "clearhistory");
	xmpp_stanza_set_attribute(command, "action", "execute");

	xmpp_stanza_add_child(iq, command);

	xmpp_stanza_release(command);

	xmpp_id_handler_add(conn, Conn_HandleClearChatHistoryReturn, id, strdup(roomjid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


/*
<iq from='kinghenryv@shakespeare.lit/throne'
    id='ban2'
    to='southampton@henryv.shakespeare.lit'
    type='get'>
  <query xmlns='http://jabber.org/protocol/muc#admin'>
    <item affiliation='outcast'/>
  </query>
</iq>
*/

/*
void Conn_GetBanList(char *roomjid)
{
	xmpp_stanza_t *iq, *query, *item;
	char id[256];

	Conn_GenID(id, "requestinstantroom");

	iq = Conn_CreateIQStanza("get", id, roomjid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "
}
*/

/*
<iq from='southampton@henryv.shakespeare.lit'
    id='ban2'
    to='kinghenryv@shakespeare.lit/throne'
    type='result'>
  <query xmlns='http://jabber.org/protocol/muc#admin'>
    <item affiliation='outcast'
          jid='earlofcambridge@shakespeare.lit'>
      <reason>Treason</reason>
    </item>
  </query>
</iq>
  */  

void Conn_InviteUser(char *roomjid, char *userjid, char *comment)
{
	xmpp_stanza_t *message, *x, *invite;

	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_attribute(message, "to", roomjid);
	
	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "http://jabber.org/protocol/muc#user");

	invite = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(invite, "invite");
	xmpp_stanza_set_attribute(invite, "to", userjid);

	if (comment)
	{
		xmpp_stanza_t *reason, *reasontext;
		char *text = EscapeParsedText(comment);

		reason = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(reason, "reason");

		reasontext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(reasontext, text);

		xmpp_stanza_add_child(reason, reasontext);
		xmpp_stanza_add_child(invite, reason);

		xmpp_stanza_release(reasontext);
		xmpp_stanza_release(reason);

		free(text);
	}

	xmpp_stanza_add_child(x, invite);
	xmpp_stanza_add_child(message, x);

	xmpp_stanza_release(invite);
	xmpp_stanza_release(x);

	xmpp_send(conn, message);

	xmpp_stanza_release(message);
}

void Conn_SetNick(char *roomjid, char *nick)
{
	char fulljid[256];
	xmpp_stanza_t *presence;

	strcpy(fulljid, roomjid);
	strcat(fulljid, "/");
	strcat(fulljid, nick);

	presence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(presence, "presence");
	xmpp_stanza_set_attribute(presence, "to", EscapeParsedText(fulljid));

	xmpp_send(conn, presence);

	xmpp_stanza_release(presence);
}

void Conn_RequestInstantRoom(char *roomjid)
{
	xmpp_stanza_t *iq, *query, *x;
	char id[256];

	Conn_GenID(id, "requestinstantroom");

	iq = Conn_CreateIQStanza("set", id, roomjid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#owner");

	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:data");
	xmpp_stanza_set_type(x, "submit");

	if (strstr(roomjid, "@chat.chesspark.com"))
	{
		xmpp_stanza_t *field, *value, *valuetext;

		field = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(field, "field");
		xmpp_stanza_set_attribute(field, "var", "muc#roomconfig_whois");

		value = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(value, "value");

		valuetext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(valuetext, "anyone");

		xmpp_stanza_add_child(value, valuetext);
		xmpp_stanza_release(valuetext);

		xmpp_stanza_add_child(field, value);
		xmpp_stanza_release(value);

		xmpp_stanza_add_child(x, field);
		xmpp_stanza_release(field);
	}

	xmpp_stanza_add_child(query, x);
	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(x);
	xmpp_stanza_release(query);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


void Conn_RequestChatConfigForm(char *roomjid)
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "requestchatconfig");

	iq = Conn_CreateIQStanza("get", id, roomjid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#owner");

	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(query);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


int Conn_HandleRequestMatchReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *opponentjid = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleRequestMatchReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (strcmp(type, "error") == 0 || strcmp(type, "auth") == 0 || strcmp(type, "modify") == 0 || strcmp(type, "cancel") == 0))
	{
		Conn_HandleGameRequestError(stanza, opponentjid, NULL, NULL);
	}
	else if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *gamerequest = xmpp_stanza_get_child_by_name(stanza, "game-request");

		if (gamerequest)
		{
			char *gameid = xmpp_stanza_get_attribute(gamerequest, "id");

			ModelConn_RequestMatchSetID(opponentjid, gameid);
		}
	}

	free(userdata);
	return 0;
}


int Conn_HandleRequestRematchReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *oldgameid = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleRequestMatchReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (strcmp(type, "error") == 0 || strcmp(type, "auth") == 0 || strcmp(type, "modify") == 0 || strcmp(type, "cancel") == 0))
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			xmpp_stanza_t *badrequest           = xmpp_stanza_get_child_by_name(error, "bad-request");
			xmpp_stanza_t *membernotonline      = xmpp_stanza_get_child_by_name(error, "member-not-online");
			xmpp_stanza_t *memberplaying        = xmpp_stanza_get_child_by_name(error, "member-playing");
			xmpp_stanza_t *memberalreadyplaying = xmpp_stanza_get_child_by_name(error, "member-already-playing");
			xmpp_stanza_t *memberexpired        = xmpp_stanza_get_child_by_name(error, "membership-expired");
			xmpp_stanza_t *internalserviceerror = xmpp_stanza_get_child_by_name(error, "internal-service-error");
			xmpp_stanza_t *notfound             = xmpp_stanza_get_child_by_name(error, "not-found");
			xmpp_stanza_t *notamember           = xmpp_stanza_get_child_by_name(error, "not-a-member");
			xmpp_stanza_t *notapro              = xmpp_stanza_get_child_by_name(error, "not-a-pro");

			char *jid = NULL;

			if (membernotonline)
			{
				if (!jid)
				{
					jid = xmpp_stanza_get_text(membernotonline);
				}

				ModelConn_GameRematchError(oldgameid, "offline", jid);
			}
			else if (memberplaying)
			{
				if (!jid)
				{
					jid = xmpp_stanza_get_text(memberplaying);
				}

				ModelConn_GameRematchError(oldgameid, "playing", jid);
			}
			else if (memberalreadyplaying)
			{
				if (!jid)
				{
					jid = xmpp_stanza_get_text(memberalreadyplaying);
				}

				ModelConn_GameRematchError(oldgameid, "playing", jid);
			}
			else if (memberexpired)
			{
				ModelConn_GameRematchError(oldgameid, "expired", jid);
			}
			else if (notfound)
			{
				ModelConn_GameRematchError(oldgameid, "notfound", jid);
			}
			else if (notamember)
			{
				ModelConn_GameRematchError(oldgameid, "notamember", jid);
			}
			else if (notapro)
			{
				ModelConn_GameRematchError(oldgameid, "notapro", jid);
			}
			else if (badrequest)
			{
				ModelConn_GameRematchError(oldgameid, "badrequest", NULL);
			}
			else if (internalserviceerror)
			{
				ModelConn_GameRematchError(oldgameid, "internalservice", NULL);
			}
			else
			{
				ModelConn_GameRematchError(oldgameid, xmpp_stanza_get_text(error), NULL);
			}
		}
		else
		{
			ModelConn_GameRematchError(oldgameid, "", NULL);
		}
	}
	else if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *gamerequest = xmpp_stanza_get_child_by_name(stanza, "game-request");

		if (gamerequest)
		{
			char *gameid = xmpp_stanza_get_attribute(gamerequest, "id");

			ModelConn_RematchSetID(oldgameid, gameid);
		}
	}

	free(userdata);
	return 0;
}

xmpp_stanza_t *Conn_CreateRequestMatchStanza(char *opponentjid, struct gamesearchinfo_s *info)
{
	xmpp_stanza_t *gamerequest;

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(gamerequest, "to", opponentjid);

	if (info && info->gameid)
	{
		xmpp_stanza_set_attribute(gamerequest, "id", info->gameid);
	}

	if (info && info->variant)
	{
		xmpp_stanza_t *variant = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(variant, "variant");
		xmpp_stanza_set_attribute(variant, "name", info->variant);

		xmpp_stanza_add_child(gamerequest, variant);

		xmpp_stanza_release(variant);
	}

	if (info && info->colorpreference)
	{
		xmpp_stanza_t *colorpreference = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(colorpreference, "color-preference");
		if (info->colorpreference == 1)
		{
			xmpp_stanza_set_attribute(colorpreference, "color", "white");
		}
		else
		{
			xmpp_stanza_set_attribute(colorpreference, "color", "black");
		}

		xmpp_stanza_add_child(gamerequest, colorpreference);

		xmpp_stanza_release(colorpreference);

	}

	if (info && info->timecontrol)
	{
		if (info->blacktimecontrol)
		{
			xmpp_stanza_t *timecontrol;

			timecontrol = Conn_CreateTimeControl(info->timecontrol->delayinc, info->timecontrol->controlarray);

			xmpp_stanza_set_attribute(timecontrol, "side", "white");

			xmpp_stanza_add_child(gamerequest, timecontrol);

			xmpp_stanza_release(timecontrol);

			timecontrol = Conn_CreateTimeControl(info->blacktimecontrol->delayinc, info->blacktimecontrol->controlarray);

			xmpp_stanza_set_attribute(timecontrol, "side", "black");

			xmpp_stanza_add_child(gamerequest, timecontrol);

			xmpp_stanza_release(timecontrol);
		}
		else
		{
			xmpp_stanza_t *timecontrol = Conn_CreateTimeControl(info->timecontrol->delayinc, info->timecontrol->controlarray);

			xmpp_stanza_add_child(gamerequest, timecontrol);

			xmpp_stanza_release(timecontrol);
		}
	}

	if (info && info->rated)
	{
		xmpp_stanza_t *rated = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(rated, "rated");

		xmpp_stanza_add_child(gamerequest, rated);
		xmpp_stanza_release(rated);
	}

	if (info && info->comment)
	{
		xmpp_stanza_t *comment = xmpp_stanza_new(ctx);
		xmpp_stanza_t *commenttext = xmpp_stanza_new(ctx);
		char *text = EscapeParsedText(info->comment);

		xmpp_stanza_set_name(comment, "comment");
		xmpp_stanza_set_text(commenttext, text);

		xmpp_stanza_add_child(comment, commenttext);

		xmpp_stanza_release(commenttext);

		xmpp_stanza_add_child(gamerequest, comment);

		xmpp_stanza_release(comment);
		free(text);
	}

	/*
	if (info && info->correspondence)
	{
		xmpp_stanza_t *correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");

		xmpp_stanza_add_child(gamerequest, correspondence);
		xmpp_stanza_release(correspondence);
	}
	*/

	if (info && info->takebacks)
	{
		xmpp_stanza_t *takebacks = Conn_CreateTextStanza("takebacks", info->takebacks);

		xmpp_stanza_add_child(gamerequest, takebacks);
		xmpp_stanza_release(takebacks);
	}

	if (info && info->solitaire)
	{
		xmpp_stanza_t *solitaire = Conn_CreateTextStanza("solitaire", NULL);

		xmpp_stanza_add_child(gamerequest, solitaire);
		xmpp_stanza_release(solitaire);
	}

	return gamerequest;
}

void Conn_RequestMatch(char *opponentjid, struct gamesearchinfo_s *info)
{
	/*if (!info->correspondence)*/
	{
		xmpp_stanza_t *iq, *gamerequest;
		char id[256];

		Conn_GenID(id, "requestmatch");

		iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

		gamerequest = Conn_CreateRequestMatchStanza(opponentjid, info);

		xmpp_stanza_add_child(iq, gamerequest);

		xmpp_stanza_release(gamerequest);
	
		xmpp_id_handler_add(conn, Conn_HandleRequestMatchReturn, id, strdup(opponentjid));

		xmpp_send(conn, iq);

		xmpp_stanza_release(iq);
	}
	/*else
	{
		xmpp_stanza_t *message, *gamerequest;
		char id[256];
  
		Conn_GenID(id, "requestcormatch");

		message = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(message, "message");
		xmpp_stanza_set_id(message, id);
		xmpp_stanza_set_attribute(message, "to", "match.chesspark.com");

		gamerequest = Conn_CreateRequestMatchStanza(opponentjid, info);
		xmpp_stanza_set_attribute(gamerequest, "type", "set");

		xmpp_stanza_add_child(message, gamerequest);

		xmpp_stanza_release(gamerequest);
	
		xmpp_id_handler_add(conn, Conn_HandleRequestMatchReturn, id, strdup(opponentjid));

		xmpp_send(conn, message);

		xmpp_stanza_release(message);
	}*/
}


void Conn_RespondAd(char *opponentjid, struct gamesearchinfo_s *info)
{
	Conn_RequestMatch(opponentjid, info);
}



void Conn_RequestRematch(char *gameid)
{
	xmpp_stanza_t *iq, *gamerequest, *rematch;
	char id[256];

	Conn_GenID(id, "requestrematch");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(gamerequest, "id", gameid);

	rematch = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(rematch, "rematch");
	xmpp_stanza_set_attribute(rematch, "oldgameid", gameid);

	xmpp_stanza_add_child(gamerequest, rematch);

	xmpp_stanza_release(rematch);

	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);
	
	xmpp_id_handler_add(conn, Conn_HandleRequestRematchReturn, id, strdup(gameid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

int Conn_HandleAcceptMatchReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *gameid = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleAcceptMatchReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (strcmp(type, "error") == 0 || strcmp(type, "auth") == 0 || strcmp(type, "modify") == 0 || strcmp(type, "cancel") == 0))
	{
		Conn_HandleGameRequestError(stanza, NULL, gameid, NULL);
	}

	free(userdata);
	return 0;
}

/*
<iq id="AAAAAAAB:requestmatch" xmlns="jabber:client" type="result" to="jcanete@chesspark.com/cpc" from="match.chesspark.com">
 <game-request id="409" xmlns="http://onlinegamegroup.com/xml/chesspark-01"/>
</iq>

<iq id="match_3" xmlns="jabber:client" type="set" to="jcanete@chesspark.com/cpc" from="match.chesspark.com">
 <game-request id="409" xmlns="http://onlinegamegroup.com/xml/chesspark-01" from="test@chesspark.com/EngineBot">
  <variant name="standard"/>
  <game-accepted/>
 </game-request>
</iq>

<iq id="matchreq2" to="match.chesspark.com" type="set">
 <game-request id="GAMEID"><game-accepted/></game-request>
</iq>
*/

void Conn_AcceptMatch(char *gameid, char *oldgameid)
{
	xmpp_stanza_t *iq, *gamerequest, *gameaccepted;
	char id[256];

	Conn_GenID(id, "acceptmatch");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(gamerequest, gameid);
	xmpp_stanza_set_attribute(gamerequest, "type", "set");
	
	gameaccepted = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gameaccepted, "game-accepted");

	xmpp_stanza_add_child(gamerequest, gameaccepted);

	xmpp_stanza_release(gameaccepted);
	
	if (oldgameid)
	{
		xmpp_stanza_t *rematch = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(rematch, "rematch");
		xmpp_stanza_set_attribute(rematch, "oldgameid", oldgameid);

		xmpp_stanza_add_child(gamerequest, rematch);

		xmpp_stanza_release(rematch);
	}

	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);
	
	xmpp_id_handler_add(conn, Conn_HandleAcceptMatchReturn, id, strdup(gameid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_AcceptCorMatch(char *gameid)
{
	xmpp_stanza_t *iq, *gamerequest, *gameaccepted, *correspondence;
	char id[256];

	Conn_GenID(id, "acceptcormatch");

	iq = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_id(iq, id);
	xmpp_stanza_set_attribute(iq, "to", "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(gamerequest, gameid);
	
	gameaccepted = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gameaccepted, "game-accepted");

	xmpp_stanza_add_child(gamerequest, gameaccepted);

	xmpp_stanza_release(gameaccepted);

	correspondence = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(correspondence, "correspondence");

	xmpp_stanza_add_child(gamerequest, correspondence);
	xmpp_stanza_release(correspondence);
	
	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);
	
	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq id="matchreq2" to="match.chesspark.com" type="set">
  <game-request id="GAMEID">
    <game-rejected>REASON</request-rejected>
  </game-request>
</iq>
*/

void Conn_DeclineMatch(char *gameid, char *oldgameid)
{
	xmpp_stanza_t *iq, *gamerequest, *gamerejected;
	char id[256];

	Conn_GenID(id, "declinematch");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(gamerequest, gameid);
	
	gamerejected = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerejected, "game-rejected");

	xmpp_stanza_add_child(gamerequest, gamerejected);

	xmpp_stanza_release(gamerejected);
	/*
	if (oldgameid)
	{
		xmpp_stanza_t *rematch = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(rematch, "rematch");
		xmpp_stanza_set_attribute(rematch, "oldgameid", oldgameid);

		xmpp_stanza_add_child(gamerequest, rematch);

		xmpp_stanza_release(rematch);
	}
	*/
	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);
	
	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


void Conn_DeclineCorMatch(char *gameid)
{
	xmpp_stanza_t *iq, *gamerequest, *gamerejected;
	char id[256];

	Conn_GenID(id, "declinecormatch");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(gamerequest, gameid);
	
	gamerejected = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerejected, "game-rejected");

	xmpp_stanza_add_child(gamerequest, gamerejected);

	xmpp_stanza_release(gamerejected);
	
	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);
	
	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}


int Conn_HandleReconveneReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *gameid = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleReconveneReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && (strcmp(type, "error") == 0 || strcmp(type, "auth") == 0 || strcmp(type, "modify") == 0 || strcmp(type, "cancel") == 0))
	{
		Conn_HandleGameRequestError(stanza, NULL, userdata, NULL);
	}

	free(userdata);
	return 0;
}


void Conn_GameReconvene(char *gameid)
{
	xmpp_stanza_t *iq, *gamerequest, *gamereconvene;
	char id[256];

	Conn_GenID(id, "reconvene");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	gamerequest = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamerequest, "game-request");
	xmpp_stanza_set_ns(gamerequest, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(gamerequest, gameid);

	gamereconvene = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(gamereconvene, "game-reconvene");
		
	xmpp_stanza_add_child(gamerequest, gamereconvene);

	xmpp_stanza_release(gamereconvene);
	
	xmpp_stanza_add_child(iq, gamerequest);

	xmpp_stanza_release(gamerequest);

	xmpp_id_handler_add(conn, Conn_HandleReconveneReturn, id, strdup(gameid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

struct pingdata_s
{
	unsigned int tick;
	char *jid;
	int repeat;
	int show;
};

int Conn_HandleSendMoveReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	struct pingdata_s *pdata = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleSendMoveReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	Log_Write2(0, "Move round trip time is %d ms\n", GetTickCount() - pdata->tick);

	if (type && (strcmp(type, "error") == 0 || strcmp(type, "auth") == 0 || strcmp(type, "modify") == 0 || strcmp(type, "cancel") == 0))
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			xmpp_stanza_t *illegalmove = xmpp_stanza_get_child_by_name(error, "illegal-move");

			if (strcmp(id + 9, "sendmove") == 0)
			{
				ModelConn_ResetGamePosition(pdata->jid, illegalmove != NULL);
			}
		}
	}
	else if (type && strcmp(type, "result") == 0)
	{
		ModelConn_MoveSuccess(pdata->jid, GetTickCount() - pdata->tick);
	}


	return 0;
}

/*
<iq to='arbiter.server.tld' from='player@server.tld' type='set'>
<game xmlns='http://onlinegamegroup.com/xml/chesspark-01'>
<move>
e2e4e2e4
</move>
[<flag />] <!-- Notify arbiter that time is up. -->
[<adjudication />] <!-- Claim that the sender won the game. -->
[<draw />] <!-- Claim that the game is a draw -->
[<resign />] <!-- Resign from the game -->
</game>
</iq>
*/

void Conn_SendMove(char *gameid, char *movetext, int corgame)
{
	xmpp_stanza_t *iq, *game, *move, *move2;
	char id[256];
	struct pingdata_s *pdata;

	Conn_GenID(id, "sendmove");

	iq = Conn_CreateIQStanza("set", id, "arbiter.chesspark.com");

	game = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(game, "game");
	xmpp_stanza_set_ns(game, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(game, gameid);
	
	move = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(move, "move");

	if (corgame == 2)
	{
		xmpp_stanza_set_attribute(move, "type", "lookahead");
	}

	move2 = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(move2, movetext);

	xmpp_stanza_add_child(move, move2);

	xmpp_stanza_release(move2);

	xmpp_stanza_add_child(game, move);

	xmpp_stanza_release(move);

	if (corgame)
	{
		xmpp_stanza_t *correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");
		xmpp_stanza_add_child(game, correspondence);
		xmpp_stanza_release(correspondence);
	}
	
	xmpp_stanza_add_child(iq, game);

	xmpp_stanza_release(game);

	pdata = malloc(sizeof(*pdata));
	pdata->jid = strdup(gameid);
	pdata->tick = GetTickCount();
	pdata->show = 0;
	pdata->repeat = 0;

	xmpp_id_handler_add(conn, Conn_HandleSendMoveReturn, id, pdata);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

struct gametaginfo_s
{
	char *gameid;
	char *tag;
};

/*
<iq id="AACqp/wo:flag" xmlns="jabber:client" type="error" from="arbiter.chesspark.com">
 <iq id="AACqp/wo:flag" type="set" to="arbiter.chesspark.com">
  <game id="3616174" xmlns="http://onlinegamegroup.com/xml/chesspark-01"><flag/></game>
 </iq>
 <error type="modify"><bad-request xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/>
  <game-flag-time xmlns="http://onlinegamegroup.com/xml/chesspark-01"/>
  <text xmlns="urn:ietf:params:xml:ns:xmpp-stanzas">41.8588659763</text>
 </error>
</iq> 
*/
int Conn_HandleGameTagReply(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	struct gametaginfo_s *gtinfo = userdata;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleGameTagReply(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && stricmp(type, "error") == 0)
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			xmpp_stanza_t *text = xmpp_stanza_get_child_by_name(error, "text");

			if (gtinfo->tag && stricmp(gtinfo->tag, "flag") == 0)
			{
                                xmpp_stanza_t *gameflagtime = xmpp_stanza_get_child_by_name(error, "game-flag-time");

				if (gameflagtime && text)
				{
					char *clocktime = xmpp_stanza_get_text(text);

					ModelConn_HandleFlagError(gtinfo->gameid, clocktime);
				}
			}
		}
	}

	return 0;
}

void Conn_SendGameTag(char *gameid, char *ctag, int correspondence)
{
	xmpp_stanza_t *iq, *game, *tag;
	char id[256];
	struct gametaginfo_s *gtinfo;

	Conn_GenID(id, ctag);

	iq = Conn_CreateIQStanza("set", id, "arbiter.chesspark.com");

	game = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(game, "game");
	xmpp_stanza_set_ns(game, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(game, gameid);
	
	tag = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tag, ctag);

	xmpp_stanza_add_child(game, tag);

	xmpp_stanza_release(tag);

	if (correspondence)
	{
		xmpp_stanza_t *correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");
		xmpp_stanza_add_child(game, correspondence);
		xmpp_stanza_release(correspondence);
	}
	
	xmpp_stanza_add_child(iq, game);

	xmpp_stanza_release(game);

	gtinfo = malloc(sizeof(*gtinfo));
	gtinfo->gameid = strdup(gameid);
	gtinfo->tag = strdup(ctag);
	
	xmpp_id_handler_add(conn, Conn_HandleGameTagReply, id, gtinfo);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_SendAddTime(char *gameid, char *addtext)
{
	xmpp_stanza_t *iq, *game, *tag, *tagtext;
	char id[256];

	Conn_GenID(id, "addtime");

	iq = Conn_CreateIQStanza("set", id, "arbiter.chesspark.com");

	game = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(game, "game");
	xmpp_stanza_set_ns(game, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(game, gameid);
	
	tag = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tag, "addtime");

	tagtext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(tagtext, addtext);

	xmpp_stanza_add_child(tag, tagtext);
	xmpp_stanza_release(tagtext);

	xmpp_stanza_add_child(game, tag);
	xmpp_stanza_release(tag);
	
	xmpp_stanza_add_child(iq, game);
	xmpp_stanza_release(game);
	
	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq from='search.chesspark.com' id='search01' type='result'>
  <results xmlns='http://onlinegamegroup.com/xml/chesspark-01'>
    <game white='blah@blah.blah" black='blah@blah.blah'
          white-rating='blah' black-rating='blah'
          white-award='blah' black-award='blah'
          white-title='blah' black-title='blah'
          id='1234' room='1234@blah.blah'>
      <variant name="blah"/>
      <time-control delayinc="delay">
        <control><moves>40</moves><time>7200</time></control>
        <control><time>1800</time></control>
      </time-control>
      <time-control-range name='standard|speed|blitz|bullet'/>
      <rated/>
      <computers/>
      <limit/>
    </game>
  </results>
</iq>
*/

int Conn_HandleGameSearch(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleGameSearch(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
		xmpp_stanza_t *roster = xmpp_stanza_get_child_by_name(stanza, "roster");

#if 0 /* disabled initial roster */
		if (roster)
		{
			xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(roster, "item");

			ModelConn_SetSharedRosterFlag();

			while (item)
			{
				char *jid = xmpp_stanza_get_attribute(item, "jid");
				char *nick = xmpp_stanza_get_attribute(item, "name");
				char *group = xmpp_stanza_get_attribute(item, "group");
				struct namedlist_s *grouplist = NULL;

				if (!Model_HasFriend(jid) && !Model_IsLoginJid(jid))
				{
					char *barejid = Jid_Strip(jid);

					if (group)
					{
						NamedList_Add(&grouplist, group, NULL, NULL);
					}

					ModelConn_SetFriend(barejid, nick, grouplist, 0, 1, 0);
					Conn_SetFriend(barejid, nick, grouplist);
					/*Conn_ApproveFriend(barejid);*/
					Conn_RequestFriend(barejid);

					NamedList_Destroy(&grouplist);

					free(barejid);
				}

				item = xmpp_stanza_get_next_by_name(item, "item");
			}
		}
#endif
		if (query)
		{
			xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(query, "item");
			char *node = xmpp_stanza_get_attribute(query, "node");
			int gamefound = FALSE;

			ModelConn_ClearGamesSearch(node);

			while (item)
			{
				xmpp_stanza_t *game = xmpp_stanza_get_child_by_name(item, "game");
				xmpp_stanza_t *tournament = xmpp_stanza_get_child_by_name(item, "tournament");
				char *node = xmpp_stanza_get_attribute(item, "node");
				char *jid = xmpp_stanza_get_attribute(item, "jid");

				if (game)
				{
					struct gamesearchinfo_s *info = Conn_ParseGameSearchInfo(game);

					info->gameid = strdup(xmpp_stanza_get_attribute(item, "id"));
					info->node = strdup(node);

					ModelConn_AddSearchGame(info->gameid, node, jid, info);

					Info_DestroyGameSearchInfo(info);

					gamefound = TRUE;

				}
				else if (tournament)
				{
					xmpp_stanza_t *manager	   = xmpp_stanza_get_child_by_name(tournament, "manager");
					xmpp_stanza_t *name	       = xmpp_stanza_get_child_by_name(tournament, "name");
					xmpp_stanza_t *timecontrol = xmpp_stanza_get_child_by_name(tournament, "time-control");
					xmpp_stanza_t *variant     = xmpp_stanza_get_child_by_name(tournament, "variant");

					struct tournamentinfo_s *info = malloc(sizeof(*info));

					memset(info, 0, sizeof(*info));

					info->id = xmpp_stanza_get_attribute(item, "id");

					if (manager)
					{
						info->manager = xmpp_stanza_get_text(manager);
					}

					if (name)
					{
						info->name = xmpp_stanza_get_text(name);
					}

					if (timecontrol)
					{
						info->timecontrol = xmpp_stanza_get_text(timecontrol);
					}

					if (variant)
					{
						info->variant = xmpp_stanza_get_text(variant);
					}

					ModelConn_AddSearchTournament(info);

					gamefound = TRUE;
				}

				item = xmpp_stanza_get_next_by_name(item, "item");
			}

			ModelConn_FinishGameResults(!gamefound);
		}
	}
	else if (type && stricmp(type, "error") == 0)
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			xmpp_stanza_t *notloggedin = xmpp_stanza_get_child_by_name(error, "not-logged-in");

			if (notloggedin)
			{
				ModelConn_ShowGameLogoutMessage(NULL);
			}
			else
			{
				ModelConn_GenericError("Error in search!", xmpp_stanza_get_text(error));
			}
		}
	}

	return 0;
}

void Conn_RequestGameSearch(char *node, struct gamesearchinfo_s *info)
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "gamesearch");

	iq = Conn_CreateIQStanza("get", id, "search.chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#items");
	xmpp_stanza_set_attribute(query, "node", node);

	if (info)
	{
		xmpp_stanza_t *filter;

		filter = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(filter, "filter");

		Conn_AddGameSearchInfo(filter, info);

		xmpp_stanza_add_child(query, filter);

		xmpp_stanza_release(filter);
	}
	
	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, Conn_HandleGameSearch, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

int Conn_HandlePostGameAdReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandlePostGameAdReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *postad = xmpp_stanza_get_child_by_name(stanza, "post-ad");
		char *id = NULL;
		if (postad)
		{
			id = xmpp_stanza_get_attribute(postad, "id");
		}

		ModelConn_AddAdSuccess(id);
	}
	else if (stricmp(type, "error") == 0)
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");

		if (error)
		{
			xmpp_stanza_t *badformat  = xmpp_stanza_get_child_by_name(error, "bad-format");
			xmpp_stanza_t *toomanyads = xmpp_stanza_get_child_by_name(error, "too-many-ads");
			xmpp_stanza_t *dupead     = xmpp_stanza_get_child_by_name(error, "duplicate-ad");
			xmpp_stanza_t *internal   = xmpp_stanza_get_child_by_name(error, "internal-service-error");
			xmpp_stanza_t *notapro    = xmpp_stanza_get_child_by_name(error, "not-a-pro");

			if (badformat)
			{
				ModelConn_GenericError(_("Error posting game!"), _("Your game has a bad format.\n\nTry changing your options and posting again."));
			}
			else if (dupead)
			{
				ModelConn_GenericError(_("Error posting game!"), _("You tried to post a duplicate game.  Try changing your game settings before posting again."));
			}
			else if (toomanyads)
			{
				ModelConn_GenericError(_("Error posting game!"), _("You have too many waiting games.\n\nYou need to remove one of your other games before posting again."));
			}
			else if (internal)
			{
				ModelConn_GenericError(_("Error posting game!"), _("Internal service error.\n\nPlease use the report a problem link on the game finder, or contact a Chesspark representative for assistance."));
			}
			else if (notapro)
			{
				ModelConn_NotAProError("adpost");
			}
			else
			{
				ModelConn_GenericError(_("Error posting game!"), _("Unknown error.\n\nPlease use the report a problem link on the game finder, or contact a Chesspark representative for assistance."));
			}
		}
		else
		{
			ModelConn_GenericError(_("Error posting game!"), _("Unknown error.\n\nPlease use the report a problem link on the game finder, or contact a Chesspark representative for assistance."));
		}
	}
	/*ModelConn_RefreshGameSearch(userdata);*/

	return 0;
}

void Conn_PostGameAd(struct gamesearchinfo_s *info)
{
	xmpp_stanza_t *iq, *postad;
	char id[256];

	Conn_GenID(id, "postgamead");

	iq = Conn_CreateIQStanza("set", id, "search.chesspark.com");

	postad = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(postad, "post-ad");
	xmpp_stanza_set_ns(postad, "http://onlinegamegroup.com/xml/chesspark-01");

	Conn_AddGameSearchInfo(postad, info);

	xmpp_stanza_add_child(iq, postad);

	xmpp_stanza_release(postad);

	xmpp_id_handler_add(conn, Conn_HandlePostGameAdReturn, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}
/*

<iq to='search.chesspark.com' id='post01' type='set'>
    <delete-ad xmlns='http://onlinegamegroup.com/xml/chesspark-01' id='123'>
</iq>

*/
void Conn_RemoveGameAd(char *itemid)
{
	xmpp_stanza_t *iq, *deletead;
	char id[256];

	Conn_GenID(id, "removegamead");

	iq = Conn_CreateIQStanza("set", id, "search.chesspark.com");

	deletead = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(deletead, "delete-ad");
	xmpp_stanza_set_ns(deletead, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(deletead, itemid);

	xmpp_stanza_add_child(iq, deletead);

	xmpp_stanza_release(deletead);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq id="AAAAAAAB:requestprofile" xmlns="jabber:client" type="result" to="test@chesspark.com/cpc" from="profile.chesspark.com">
 <profile jid="tofu@chesspark.com" xmlns="http://jabber.org/protocol/profile"><x xmlns="jabber:x:data" type="result">
  <field var="firstname"><value/></field>
  <field var="surname"><value/></field>
  <field var="email"><value/></field>
  <field var="member-since"><value>2006-03-14</value></field>
  <field var="last-online"><value>2007-05-23 03:45:42.674721-05:00</value></field>
  <field var="ratings">
   <value>
    <rating>
	 <rating>1662</rating>
	 <rd>290</rd>
	 <variant>Standard</variant>
	</rating>
   </value>
  </field>
 </x></profile></iq>
*/

int Conn_HandleProfile(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleProfile(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *profile = xmpp_stanza_get_child_by_name(stanza, "profile");

		if (profile)
		{
			xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(profile, "x");
			char *jid = xmpp_stanza_get_attribute(profile, "jid");

			if (x)
			{
				char *ns = xmpp_stanza_get_attribute(x, "xmlns");

				if (strcmp(ns, "jabber:x:data") == 0)
				{
					xmpp_stanza_t *field = xmpp_stanza_get_child_by_name(x, "field");
					struct namedlist_s *profileinfo = NULL;

					ModelConn_SetGameStatus(jid, NULL, NULL, 0, 0, 0, 1);
					Conn_HandleProfilePlayingFields(jid, x, 0);

					while (field)
					{
						char *var = xmpp_stanza_get_attribute(field, "var");
						xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(field, "value");
						xmpp_stanza_t *item  = xmpp_stanza_get_child_by_name(field, "item");

						if (item)
						{
							if (var && strcmp(var, "ad-hoc") == 0)
							{
								struct adhoccommand_s *command;

								command = malloc(sizeof(*command));
								memset(command, 0, sizeof(*command));

								command->name = strdup(xmpp_stanza_get_attribute(item, "name"));
								command->command = strdup(xmpp_stanza_get_attribute(item, "node"));

								NamedList_Add(&profileinfo, var, command, Info_DestroyAdHocCommand);
							}
						}

						if (value)
						{
							/* retrieve ratings */
							if (var && strcmp(var, "ratings") == 0)
							{
								xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(value, "rating");
								struct namedlist_s *ratings = NULL;

								while (rating)
								{
									xmpp_stanza_t *category = xmpp_stanza_get_child_by_name(rating, "category");
									xmpp_stanza_t *variant  = xmpp_stanza_get_child_by_name(rating, "variant");

									struct rating_s *ratinginfo = Conn_ParseRatingStanza(rating);

									if (category)
									{
										NamedList_Add(&ratings, xmpp_stanza_get_text(category), ratinginfo, Info_DestroyRating);
									}
									else if (variant)
									{
										NamedList_Add(&ratings, xmpp_stanza_get_text(variant), ratinginfo, Info_DestroyRating);
									}
									else
									{
										NamedList_Add(&ratings, NULL, ratinginfo, Info_DestroyRating);
									}

									rating = xmpp_stanza_get_next_by_name(rating, "rating");
								}

								NamedList_Add(&profileinfo, var, ratings, NamedList_Destroy2);
							}
							else if (var && strcmp(var, "roles") == 0)
							{
								xmpp_stanza_t *role = xmpp_stanza_get_child_by_name(value, "role");
								struct namedlist_s *roles = NULL;

								while (role)
								{
									NamedList_AddString(&roles, NULL, xmpp_stanza_get_text(role));

									role = xmpp_stanza_get_next_by_name(role, "role");
								}

								NamedList_Add(&profileinfo, var, roles, NamedList_Destroy2);
							}
							else if (var && strcmp(var, "titles") == 0)
							{
								xmpp_stanza_t *title = xmpp_stanza_get_child_by_name(value, "title");
								struct namedlist_s *titles = NULL;

								while (title)
								{
									NamedList_AddString(&titles, NULL, xmpp_stanza_get_text(title));

									title = xmpp_stanza_get_next_by_name(title, "title");
								}

								NamedList_Add(&profileinfo, var, titles, NamedList_Destroy2);
							}
							else if (var && stricmp(var, "client_info") == 0)
							{
								xmpp_stanza_t *version = xmpp_stanza_get_child_by_name(value, "version");
								xmpp_stanza_t *vendor  = xmpp_stanza_get_child_by_name(value, "vender");
								xmpp_stanza_t *name    = xmpp_stanza_get_child_by_name(value, "name");
								xmpp_stanza_t *os      = xmpp_stanza_get_child_by_name(value, "os");

								struct namedlist_s *clientinfo = NULL;

								if (name)
								{
									NamedList_AddString(&clientinfo, "name", xmpp_stanza_get_text(name));
								}

								if (version)
								{
									NamedList_AddString(&clientinfo, "version", xmpp_stanza_get_text(version));
								}

								if (vendor)
								{
									NamedList_AddString(&clientinfo, "vendor", xmpp_stanza_get_text(vendor));
								}

								if (os)
								{
									NamedList_AddString(&clientinfo, "os", xmpp_stanza_get_text(os));
								}

								NamedList_Add(&profileinfo, var, clientinfo, NamedList_Destroy2);
							}
							else if (var && stricmp(var, "groups") == 0)
							{
								xmpp_stanza_t *groups = xmpp_stanza_get_child_by_name(value, "groups");
								
								if (groups)
								{
									xmpp_stanza_t *group = xmpp_stanza_get_child_by_name(groups, "group");
									struct namedlist_s *grouplist = NULL;

									while (group)
									{
										struct groupinfo_s *ginfo;

										ginfo = Conn_ParseGroupStanza(group);

										if (ginfo)
										{
											NamedList_Add(&grouplist, ginfo->name, ginfo, Info_DestroyGroupInfo);
										}

										group = xmpp_stanza_get_next_by_name(group, "group");
									}

									NamedList_Add(&profileinfo, var, grouplist, NamedList_Destroy2);
								}
							}
							else if (var)
							{
								NamedList_AddString(&profileinfo, var, xmpp_stanza_get_text(value));
							}
						}

						field = xmpp_stanza_get_next_by_name(field, "field");
					}

					ModelConn_SetProfile(jid, profileinfo);

					NamedList_Destroy(&profileinfo);
				}
			}
		}
	}

	return 0;
}


void Conn_RequestProfile(char *jid)
{
	xmpp_stanza_t *iq, *profile;
	char id[256];

	Conn_GenID(id, "requestprofile");

	iq = Conn_CreateIQStanza("get", id, "profile.chesspark.com");

	profile = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(profile, "profile");
	xmpp_stanza_set_ns(profile, "http://jabber.org/protocol/profile");
	xmpp_stanza_set_attribute(profile, "jid", jid);

	xmpp_stanza_add_child(iq, profile);

	xmpp_stanza_release(profile);

	xmpp_id_handler_add(conn, Conn_HandleProfile, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);

}

int Conn_HandleSubscribeProfile(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	char *jid;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleSubscribeProfile(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	jid = userdata;

	if (type && strcmp(type, "result") == 0)
	{
		ModelConn_SubscribeProfileSuccess(jid);
	}

	free(jid);

	return 0;
}

/*
<iq type="set"
    from="sub1@foo.com/home"
    to="ratings.chesspark.com"
    id="sub1">
  <pubsub xmlns="http://jabber.org/protocol/pubsub">
    <subscribe
        node="http://jabber.org/protocol/profile"
        jid="tofu@chesspark.com"/>
  </pubsub>
</iq>
*/

void Conn_SubscribeProfile(char *jid)
{
	xmpp_stanza_t *iq, *pubsub, *subscribe;
	char id[256];

	Conn_GenID(id, "subscribeprofile");

	iq = Conn_CreateIQStanza("set", id, "profile.chesspark.com");

	pubsub = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	subscribe = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(subscribe, "subscribe");
	xmpp_stanza_set_attribute(subscribe, "node", "http://jabber.org/protocol/profile");
	xmpp_stanza_set_attribute(subscribe, "jid", jid);

	xmpp_stanza_add_child(pubsub, subscribe);

	xmpp_stanza_release(subscribe);

	xmpp_stanza_add_child(iq, pubsub);

	xmpp_stanza_release(pubsub);

	xmpp_id_handler_add(conn, Conn_HandleSubscribeProfile, id, strdup(jid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_UnsubscribeProfile(char *jid)
{
	xmpp_stanza_t *iq, *pubsub, *unsubscribe;
	char id[256];

	Conn_GenID(id, "unsubscribeprofile");

	iq = Conn_CreateIQStanza("set", id, "profile.chesspark.com");

	pubsub = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	unsubscribe = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(unsubscribe, "unsubscribe");
	xmpp_stanza_set_attribute(unsubscribe, "node", "http://jabber.org/protocol/profile");
	xmpp_stanza_set_attribute(unsubscribe, "jid", jid);

	xmpp_stanza_add_child(pubsub, unsubscribe);

	xmpp_stanza_release(unsubscribe);

	xmpp_stanza_add_child(iq, pubsub);

	xmpp_stanza_release(pubsub);

	xmpp_id_handler_add(conn, Conn_HandleProfile, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type='result'
    from='plays.shakespeare.lit'
    to='romeo@montague.net/orchard'
    id='info1'>
  <query xmlns='http://jabber.org/protocol/disco#info'>
    <feature var='http://jabber.org/protocol/disco#info'/>
	<feature var='http://onlinegamegroup.com/xml/chesspark-01'/>
  </query>
</iq>
*/

int Conn_HandleDiscoInfo(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleDiscoInfo(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "result") == 0)
	{
		char *from = xmpp_stanza_get_attribute(stanza, "from");

		xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
		
		if (query)
		{
			char *ns = xmpp_stanza_get_ns(query);

			if (ns && strcmp(ns, "http://jabber.org/protocol/disco#info") == 0)
			{
				xmpp_stanza_t *feature = xmpp_stanza_get_child_by_name(query, "feature");

				while (feature)
				{
					char *var = xmpp_stanza_get_attribute(feature, "var");

					if (var && strcmp(var, "http://onlinegamegroup.com/xml/chesspark-01") == 0)
					{
						ModelConn_SetFriendHasChesspark(from);
					}

					feature = xmpp_stanza_get_next_by_name(feature, "feature");
				}
			}
		}
	}

	return 0;
}

/*
<iq type='get'
    from='romeo@montague.net/orchard'
    to='plays.shakespeare.lit'
    id='info1'>
  <query xmlns='http://jabber.org/protocol/disco#info'/>
</iq>
*/

void Conn_RequestDiscoInfo(char *jid)
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "requestdiscoinfo");

	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");

	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleDiscoInfo, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type="set"
    from="sub1@foo.com/home"
    to="ratings.chesspark.com"
    id="sub1">
  <pubsub xmlns="http://jabber.org/protocol/pubsub">
    <subscribe 
        node="ratings"
        jid="tofu@chesspark.com"/>
  </pubsub>
</iq>
*/

void Conn_SubscribeRating(char *jid)
{
	xmpp_stanza_t *iq, *pubsub, *subscribe;
	char id[256];

	Conn_GenID(id, "subscriberating");

	iq = Conn_CreateIQStanza("set", id, "ratings.chesspark.com");

	pubsub = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	subscribe = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(subscribe, "subscribe");
	xmpp_stanza_set_attribute(subscribe, "node", "ratings");
	xmpp_stanza_set_attribute(subscribe, "jid", jid);

	xmpp_stanza_add_child(pubsub, subscribe);
	xmpp_stanza_release(subscribe);

	xmpp_stanza_add_child(iq, pubsub);
	xmpp_stanza_release(pubsub);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_UnsubscribeRating(char *jid)
{
	xmpp_stanza_t *iq, *pubsub, *unsubscribe;
	char id[256];

	Conn_GenID(id, "unsubscriberating");

	iq = Conn_CreateIQStanza("set", id, "ratings.chesspark.com");

	pubsub = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	unsubscribe = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(unsubscribe, "unsubscribe");
	xmpp_stanza_set_attribute(unsubscribe, "node", "ratings");
	xmpp_stanza_set_attribute(unsubscribe, "jid", jid);

	xmpp_stanza_add_child(pubsub, unsubscribe);
	xmpp_stanza_release(unsubscribe);

	xmpp_stanza_add_child(iq, pubsub);
	xmpp_stanza_release(pubsub);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<message
    from='romeo@montague.net'
    to='juliet@capulet.com/balcony'>
  <x xmlns='jabber:x:event'>
    <composing/>
    <id>message22</id>
  </x>
</message>
*/

void Conn_SendComposing(char *jid, char *msgid)
{
	xmpp_stanza_t *message, *x, *composing, *id, *idtext;

	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_attribute(message, "to", jid);

	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:event");

	composing = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(composing, "composing");
	
	xmpp_stanza_add_child(x, composing);
	xmpp_stanza_release(composing);

	if (msgid)
	{
		id = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(id, "id");

		idtext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(idtext, msgid);

		xmpp_stanza_add_child(id, idtext);
		xmpp_stanza_release(idtext);

		xmpp_stanza_add_child(x, id);
		xmpp_stanza_release(id);
	}

	xmpp_stanza_add_child(message, x);
	xmpp_stanza_release(x);

	xmpp_send(conn, message);

	xmpp_stanza_release(message);
}

/*
<message
    from='romeo@montague.net'
    to='juliet@capulet.com/balcony'>
  <x xmlns='jabber:x:event'>
    <id>message22</id>
  </x>
</message>
*/


void Conn_CancelComposing(char *jid, char *msgid)
{
	xmpp_stanza_t *message, *x, *id, *idtext;

	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_attribute(message, "to", jid);

	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:event");

	if (msgid)
	{
		id = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(id, "id");

		idtext = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(idtext, msgid);

		xmpp_stanza_add_child(id, idtext);
		xmpp_stanza_release(idtext);

		xmpp_stanza_add_child(x, id);
		xmpp_stanza_release(id);
	}

	xmpp_stanza_add_child(message, x);
	xmpp_stanza_release(x);

	xmpp_send(conn, message);

	xmpp_stanza_release(message);
}

int Conn_HandleCorGameState(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	xmpp_stanza_t *game = xmpp_stanza_get_child_by_name(stanza, "game");

	if (game)
	{
		Conn_HandleGame(game);
	}
	
	return 1;
}

void Conn_GetCorGameState(char *gameid)
{
	/*
	xmpp_stanza_t *message, *game, *state;

	message = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_attribute(message, "to", "arbiter.chesspark.com");

	game = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(game, "game");
	xmpp_stanza_set_ns(game, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(game, gameid);

	state = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(state, "state");

	{
		xmpp_stanza_t *correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");
		xmpp_stanza_add_child(game, correspondence);
		xmpp_stanza_release(correspondence);
	}
	
	xmpp_stanza_add_child(game, state);
	xmpp_stanza_release(state);

	xmpp_stanza_add_child(message, game);
	xmpp_stanza_release(game);

	xmpp_send(conn, message);

	xmpp_stanza_release(message);
	*/
	
	xmpp_stanza_t *iq, *game, *state;
	char id[256];

	Conn_GenID(id, "getgamestate");

	iq = Conn_CreateIQStanza("get", id, "arbiter.chesspark.com");

	game = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(game, "game");
	xmpp_stanza_set_ns(game, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_id(game, gameid);
	
	state = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(state, "state");

	xmpp_stanza_add_child(game, state);

	xmpp_stanza_release(state);

	{
		xmpp_stanza_t *correspondence = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(correspondence, "correspondence");
		xmpp_stanza_add_child(game, correspondence);
		xmpp_stanza_release(correspondence);
	}
	
	xmpp_stanza_add_child(iq, game);

	xmpp_stanza_release(game);

	xmpp_id_handler_add(conn, Conn_HandleCorGameState, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
	
}

/*
<iq type='get'
    from='romeo@montague.net/orchard'
    to='juliet@capulet.com/balcony'
    id='time_1'>
  <query xmlns='jabber:iq:time'/>
</iq>
*/

void Conn_GetTime(char *jid)
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "gettime");

	iq = Conn_CreateIQStanza("get", id, "arbiter.chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:time");

	xmpp_stanza_add_child(iq, query);

	xmpp_stanza_release(query);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq id="AAAAAAAc:gettournamentinfo" xmlns="jabber:client" type="result" to="jcanete@chesspark.com/latestbuild" from="match.chesspark.com">
 <tournament id="20" xmlns="http://onlinegamegroup.com/xml/chesspark-01">
  <manager>twonds@chesspark.com</manager>
  <name>Twonds' tournament</name>
  <variant>Standard</variant>
  <pairing>Swiss</pairing>
  <time-control>Long</time-control>
  <current-round>1</current-round>
  <start-times>
   <start-time round="1">20060815T00:44:00</start-time>
   <start-time round="2">20060816T11:51:59</start-time>
  </start-times>
  <rounds total="2"/>
  <players>
   <player jid="twonds@chesspark.com"/>
   <player jid="tester@chesspark.com"/>
   <player jid="bishop@chesspark.com"/>
   <player jid="crazyhouse@chesspark.com"/>
  </players>
 </tournament>
</iq>
*/

int Conn_HandleTournamentInfo(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleTournamentInfo(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *tournament = xmpp_stanza_get_child_by_name(stanza, "tournament");

		if (tournament)
		{
			xmpp_stanza_t *manager      = xmpp_stanza_get_child_by_name(tournament, "manager");
			xmpp_stanza_t *name         = xmpp_stanza_get_child_by_name(tournament, "name");
			xmpp_stanza_t *variant      = xmpp_stanza_get_child_by_name(tournament, "variant");
			xmpp_stanza_t *pairing      = xmpp_stanza_get_child_by_name(tournament, "pairing");
			xmpp_stanza_t *round	    = xmpp_stanza_get_child_by_name(tournament, "round");
			xmpp_stanza_t *timecontrol  = xmpp_stanza_get_child_by_name(tournament, "time-control");
			xmpp_stanza_t *currentround = xmpp_stanza_get_child_by_name(tournament, "current-round");
			xmpp_stanza_t *totalrounds  = xmpp_stanza_get_child_by_name(tournament, "rounds");
			xmpp_stanza_t *players		= xmpp_stanza_get_child_by_name(tournament, "players");
			xmpp_stanza_t *starttimes   = xmpp_stanza_get_child_by_name(tournament, "start-times");

			struct tournamentinfo_s *info = malloc(sizeof(*info));

			memset(info, 0, sizeof(*info));

			info->id = xmpp_stanza_get_attribute(tournament, "id");

			if (manager)
			{
				info->manager = xmpp_stanza_get_text(manager);
			}

			if (name)
			{
				info->name = xmpp_stanza_get_text(name);
			}

			if (timecontrol)
			{
				info->timecontrol = xmpp_stanza_get_text(timecontrol);
			}

			if (variant)
			{
				info->variant = xmpp_stanza_get_text(variant);
			}

			if (pairing)
			{
				info->pairingtype = xmpp_stanza_get_text(pairing);
			}

			if (currentround)
			{
				char *currentroundtext = xmpp_stanza_get_text(currentround);

				if (currentroundtext)
				{
					sscanf(currentroundtext, "%d", &(info->currentround));
				}
			}

			if (totalrounds)
			{
				char *totalroundstext = xmpp_stanza_get_attribute(totalrounds, "total");

				if (totalroundstext)
				{
					sscanf(totalroundstext, "%d", &(info->totalrounds));
				}
			}

			if (players)
			{
				xmpp_stanza_t *player = xmpp_stanza_get_child_by_name(players, "player");
				
				while (player)
				{
					char *jid = xmpp_stanza_get_attribute(player, "jid");

					if (jid)
					{
						xmpp_stanza_t *score  = xmpp_stanza_get_child_by_name(player, "score");
						xmpp_stanza_t *wins   = xmpp_stanza_get_child_by_name(player, "wins");
						xmpp_stanza_t *draws  = xmpp_stanza_get_child_by_name(player, "draws");
						xmpp_stanza_t *losses = xmpp_stanza_get_child_by_name(player, "losses");
						xmpp_stanza_t *rating = xmpp_stanza_get_child_by_name(player, "rating");

						struct tournamentplayerinfo_s *pinfo;

						pinfo = malloc(sizeof(*pinfo));

						memset(pinfo, 0, sizeof(*pinfo));

						pinfo->jid = strdup(jid);

						if (score)
						{
							char *txt = xmpp_stanza_get_text(score);

							if (txt)
							{
								sscanf(txt, "%f", &(pinfo->score));
							}
						}

						if (wins)
						{
							char *txt = xmpp_stanza_get_text(wins);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->wins));
							}
						}

						if (draws)
						{
							char *txt = xmpp_stanza_get_text(draws);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->draws));
							}
						}

						if (losses)
						{
							char *txt = xmpp_stanza_get_text(losses);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->losses));
							}
						}

						if (rating)
						{
							char *txt = xmpp_stanza_get_text(rating);

							if (txt)
							{
								sscanf(txt, "%d", &(pinfo->rating));
							}
						}

						NamedList_Add(&(info->players), jid, pinfo, Info_DestroyTournamentPlayerInfo);
					}

					player = xmpp_stanza_get_next_by_name(player, "player");
				}
			}

			while (round)
			{
				xmpp_stanza_t *pairings = xmpp_stanza_get_child_by_name(round, "pairings");
				xmpp_stanza_t *games    = xmpp_stanza_get_child_by_name(round, "games");
				char *roundnum = xmpp_stanza_get_attribute(round, "number");
				struct namedlist_s *pairinglist = NULL;
			
				if (pairings)
				{
					xmpp_stanza_t *pairing = xmpp_stanza_get_child_by_name(pairings, "pairing");

					while (pairing)
					{
						char *white = xmpp_stanza_get_attribute(pairing, "white");
						char *black = xmpp_stanza_get_attribute(pairing, "black");
						char *bye   = xmpp_stanza_get_attribute(pairing, "bye");

						struct tournamentpairing_s *pairinfo;

						pairinfo = malloc(sizeof(*pairinfo));

						memset(pairinfo, 0, sizeof(*pairinfo));

						pairinfo->white = strdup(white);
						pairinfo->black = strdup(black);
						pairinfo->bye   = strdup(bye);

						NamedList_Add(&pairinglist, NULL, pairinfo, Info_DestroyTournamentPairing);

						pairing = xmpp_stanza_get_next_by_name(pairing, "pairing");
					}
				}

				if (games)
				{
					xmpp_stanza_t *game = xmpp_stanza_get_child_by_name(games, "game");
					
					while (game)
					{
						xmpp_stanza_t *result = xmpp_stanza_get_child_by_name(game, "result");

						char *white = Jid_Strip(xmpp_stanza_get_attribute(game, "white"));
						char *black = Jid_Strip(xmpp_stanza_get_attribute(game, "black"));
						char *gameid = xmpp_stanza_get_attribute(game, "id");

						struct namedlist_s *entry = pairinglist;

						while (entry)
						{
							struct tournamentpairing_s *pairinfo = entry->data;
							if ( (pairinfo->white && stricmp(pairinfo->white, white) == 0)
							  && (pairinfo->black && stricmp(pairinfo->black, black) == 0))
							{
								if (result)
								{
									char *side = xmpp_stanza_get_attribute(result, "side");
									char *resulttxt = xmpp_stanza_get_text(result);

									pairinfo->gamestate = 2;

									if (side && resulttxt && strcmp(resulttxt, "win") == 0)
									{
										pairinfo->winner = strdup(side);
									}
									else
									{
										pairinfo->winner = strdup("draw");
									}
								}
							}

							entry = entry->next;
						}

						free(white);
						free(black);

						game = xmpp_stanza_get_next_by_name(game, "game");
					}
				}

				NamedList_Add(&(info->roundpairings), roundnum, pairinglist, NamedList_Destroy2);

				round = xmpp_stanza_get_next_by_name(round, "round");
			}

			if (starttimes)
			{
				xmpp_stanza_t *starttime = xmpp_stanza_get_child_by_name(starttimes, "start-time");

				while (starttime)
				{
					char *round = xmpp_stanza_get_attribute(starttime, "round");
					char *time = xmpp_stanza_get_text(starttime);

					if (round && time)
					{
						NamedList_Add(&(info->roundstarttimes), round, time, NULL);
					}

					starttime = xmpp_stanza_get_next_by_name(starttime, "start-time");
				}
			}

			ModelConn_SetTournamentInfo(info);
		}
	}

	return 1;
}

/*
<iq xmlns='jabber:client' to='match.chesspark.com' type='get' id='H_13' from='user@chesspark.com/test'>
  <tournament xmlns='http://onlinegamegroup.com/xml/chesspark-01' id='20' />
</iq>
*/

void Conn_GetTournamentInfo(char *tid)
{
	xmpp_stanza_t *iq, *tournament;
	char id[256];

	Conn_GenID(id, "gettournamentinfo");

	iq = Conn_CreateIQStanza("get", id, "match.chesspark.com");

	tournament = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tournament, "tournament");
	xmpp_stanza_set_ns(tournament, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(tournament, "id", tid);

	xmpp_stanza_add_child(iq, tournament);

	xmpp_stanza_release(tournament);

	xmpp_id_handler_add(conn, Conn_HandleTournamentInfo, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq xmlns='jabber:client' to='match.chesspark.com' type='set' id='H_13' from='bishop@chesspark.com/trialtest'>
  <tournament xmlns='http://onlinegamegroup.com/xml/chesspark-01' id='20'>
    <join>bishop@chesspark.com</join>
  </tournament>
</iq>
*/

void Conn_JoinTournament(char *tourneyid, char *jid)
{
	xmpp_stanza_t *iq, *tournament, *join, *jointext;
	char id[256];

	Conn_GenID(id, "jointournament");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	tournament = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tournament, "tournament");
	xmpp_stanza_set_ns(tournament, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(tournament, "id", tourneyid);

	join = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(join, "join");

	jointext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(jointext, jid);

	xmpp_stanza_add_child(join, jointext);
	xmpp_stanza_release(jointext);

	xmpp_stanza_add_child(tournament, join);
	xmpp_stanza_release(join);

	xmpp_stanza_add_child(iq, tournament);

	xmpp_stanza_release(tournament);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_ForfeitTournament(char *tourneyid, char *jid)
{
	xmpp_stanza_t *iq, *tournament, *forfeit, *forfeittext;
	char id[256];

	Conn_GenID(id, "forfeittournament");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	tournament = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tournament, "tournament");
	xmpp_stanza_set_ns(tournament, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(tournament, "id", tourneyid);

	forfeit = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(forfeit, "forfeit");

	forfeittext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(forfeittext, jid);

	xmpp_stanza_add_child(forfeit, forfeittext);
	xmpp_stanza_release(forfeittext);

	xmpp_stanza_add_child(tournament, forfeit);
	xmpp_stanza_release(forfeit);

	xmpp_stanza_add_child(iq, tournament);

	xmpp_stanza_release(tournament);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq xmlns='jabber:client' to='match.chesspark.com' type='set' id='H_16' from='twonds@chesspark.com/trialtest'>
  <tournament xmlns='http://onlinegamegroup.com/xml/chesspark-01' id='20'>
    <start-time round='1'>20060731T03:51:42</start-time>
  </tournament>
</iq>
*/

void Conn_SetTournamentRoundStart(char *tourneyid, char *round, char *timestamp)
{
	xmpp_stanza_t *iq, *tournament, *starttime, *starttimetext;
	char id[256];

	Conn_GenID(id, "settournamentroundstart");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	tournament = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tournament, "tournament");
	xmpp_stanza_set_ns(tournament, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(tournament, "id", tourneyid);

	starttime = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(starttime, "start-time");
	xmpp_stanza_set_attribute(starttime, "round", round);

	starttimetext = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(starttimetext, timestamp);

	xmpp_stanza_add_child(starttime, starttimetext);
	xmpp_stanza_release(starttimetext);

	xmpp_stanza_add_child(tournament, starttime);
	xmpp_stanza_release(starttime);

	xmpp_stanza_add_child(iq, tournament);

	xmpp_stanza_release(tournament);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_StartNextTournamentRound(char *tourneyid)
{
	xmpp_stanza_t *iq, *tournament, *startnextround;
	char id[256];

	Conn_GenID(id, "startnexttournamentround");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	tournament = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(tournament, "tournament");
	xmpp_stanza_set_ns(tournament, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(tournament, "id", tourneyid);

	startnextround = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(startnextround, "start-next-round");

	xmpp_stanza_add_child(tournament, startnextround);
	xmpp_stanza_release(startnextround);

	xmpp_stanza_add_child(iq, tournament);

	xmpp_stanza_release(tournament);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type='set' id='pub1'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <publish node='http://jabber.org/protocol/profile'>
      <profile xmlns='http://jabber.org/protocol/profile'>
        <x xmlns='jabber:x:data' type='result'>
          <field var='weblog'>
            <value>http://www.denmark.lit/blogs/princely_musings</value>
          </field>
        </x>
      </profile>
    </publish>
  </pubsub>
</iq>
*/

int Conn_HandleSetProfileField(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleSetProfileField(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "error") == 0)
	{
		ModelConn_GenericError(_("Profile error!"), _("Error setting profile field.  Please see a Chesspark representative for assistance."));
	}

	return 1;
}
		 
void Conn_SetProfileField(char *fieldvar, char *valuecontent)
{
	xmpp_stanza_t *iq, *pubsub, *publish, *profile, *x, *field, *value, *valuetxt;
	char id[256];

	Conn_GenID(id, "SetProfileField");

	iq = Conn_CreateIQStanza("set", id, "profile.chesspark.com");

	pubsub = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(pubsub, "pubsub");
	xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

	publish = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(publish, "publish");
	xmpp_stanza_set_attribute(publish, "node", "http://jabber.org/protocol/profile");

	profile = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(profile, "profile");
	xmpp_stanza_set_ns(profile, "http://jabber.org/protocol/profile");

	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "jabber:x:data");
	xmpp_stanza_set_attribute(x, "type", "result");

	field = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(field, "field");
	xmpp_stanza_set_attribute(field, "var", fieldvar);

	value = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(value, "value");

	if (valuecontent)
	{
		valuetxt = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(valuetxt, EscapeParsedText(valuecontent));

		xmpp_stanza_add_child(value, valuetxt);
		xmpp_stanza_release(valuetxt);
	}

	xmpp_stanza_add_child(field, value);
	xmpp_stanza_release(value);

	xmpp_stanza_add_child(x, field);
	xmpp_stanza_release(field);

	xmpp_stanza_add_child(profile, x);
	xmpp_stanza_release(x);

	xmpp_stanza_add_child(publish, profile);
	xmpp_stanza_release(profile);

	xmpp_stanza_add_child(pubsub, publish);
	xmpp_stanza_release(publish);

	xmpp_stanza_add_child(iq, pubsub);
	xmpp_stanza_release(pubsub);

	xmpp_id_handler_add(conn, Conn_HandleSetProfileField, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type='set' to='responder@domain' id='exec1'>
  <command xmlns='http://jabber.org/protocol/commands'
           node='list'
           action='execute'/>
</iq>
*/

void Conn_AdHocCommand(char *jid, char *node)
{
	xmpp_stanza_t *iq, *command;
	char id[256];

	Conn_GenID(id, "AdHocCommand");

	iq = Conn_CreateIQStanza("set", id, jid);

	command = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(command, "command");
	xmpp_stanza_set_ns(command, "http://jabber.org/protocol/commands");
	xmpp_stanza_set_attribute(command, "node", node);
	xmpp_stanza_set_attribute(command, "action", "execute");

	xmpp_stanza_add_child(iq, command);
	xmpp_stanza_release(command);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq from='southampton@henryv.shakespeare.lit'
    id='ban2'
    to='kinghenryv@shakespeare.lit/throne'
    type='result'>
  <query xmlns='http://jabber.org/protocol/muc#admin'>
    <item affiliation='outcast'
          jid='earlofcambridge@shakespeare.lit'>
      <reason>Treason</reason>
    </item>
  </query>
</iq>
*/

/*
<iq id="AAAgNenE:RequestBanlist" xmlns="jabber:client" type="error" to="jcanete@chesspark.com/cpc" from="peoples-park@chat.chesspark.com">
 <query xmlns="http://jabber.org/protocol/muc#admin"><item affiliation="outcast"/></query>
 <error code="403" type="auth"><forbidden xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/></error>
</iq>
*/
int Conn_HandleBanlist(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleBanlist(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && strcmp(type, "result") == 0)
	{
		xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");
		char *chatjid = xmpp_stanza_get_attribute(stanza, "from");

		if (query)
		{
			xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(query, "item");
			struct namedlist_s *banlist = NULL;

			while (item)
			{
				xmpp_stanza_t *reason = xmpp_stanza_get_child_by_name(item, "reason");
				char *jid = xmpp_stanza_get_attribute(item, "jid");
				char *reasontext = NULL;
				char *affiliation = xmpp_stanza_get_attribute(item, "affiliation");

				if (reason)
				{
					reasontext = xmpp_stanza_get_text(reason);
				}

				if (affiliation && stricmp(affiliation, "outcast") == 0)
				{
					NamedList_AddString(&banlist, jid, reasontext);
				}

				item = xmpp_stanza_get_next_by_name(item, "item");
			}

			ModelConn_HandleBanlist(chatjid, banlist);

			NamedList_Destroy(&banlist);
		}
	}
	else if (type && strcmp(type, "error") == 0)
	{
		xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");
		char *chatjid = xmpp_stanza_get_attribute(stanza, "from");

		if (error)
		{
			char *code = xmpp_stanza_get_attribute(error, "code");
			ModelConn_HandleBanlistError(chatjid, code);
		}
		
	}

	return 1;
}

/*
<iq from='kinghenryv@shakespeare.lit/throne'
    id='ban2'
    to='southampton@henryv.shakespeare.lit'
    type='get'>
  <query xmlns='http://jabber.org/protocol/muc#admin'>
    <item affiliation='outcast'/>
  </query>
</iq>
*/


void Conn_RequestBanlist(char *jid)
{
	xmpp_stanza_t *iq, *query, *item;
	char id[256];

	Conn_GenID(id, "RequestBanlist");

	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#admin");

	item = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(item, "item");
	xmpp_stanza_set_attribute(item, "affiliation", "outcast");

	xmpp_stanza_add_child(query, item);
	xmpp_stanza_release(item);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleBanlist, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq from='kinghenryv@shakespeare.lit/throne'
    id='ban3'
    to='southampton@henryv.shakespeare.lit'
    type='set'>
  <query xmlns='http://jabber.org/protocol/muc#admin'>
    <item affiliation='outcast'
          jid='earlofcambridge@shakespeare.lit'>
      <reason>Treason</reason>
    </item>
    <item affiliation='outcast'>
          jid='lordscroop@shakespeare.lit'>
      <reason>Treason</reason>
    </item>
    <item affiliation='outcast'
          jid='sirthomasgrey@shakespeare.lit'>
      <reason>Treason</reason>
    </item>
  </query>
</iq>
*/

void Conn_SetBanlist(char *jid, struct namedlist_s *addban, struct namedlist_s *removeban)
{
	xmpp_stanza_t *iq, *query, *item;
	struct namedlist_s *entry;
	char id[256];

	Conn_GenID(id, "SetBanlist");

	iq = Conn_CreateIQStanza("set", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/muc#admin");

	entry = addban;
	while (entry)
	{
		item = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(item, "item");
		xmpp_stanza_set_attribute(item, "affiliation", "outcast");
		xmpp_stanza_set_attribute(item, "jid", entry->name);

		if (entry->data)
		{
			xmpp_stanza_t *reason, *reasontext;

			reason = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(reason, "reason");

			reasontext = xmpp_stanza_new(ctx);
			xmpp_stanza_set_text(reasontext, entry->data);

			xmpp_stanza_add_child(reason, reasontext);
			xmpp_stanza_release(reasontext);

			xmpp_stanza_add_child(item, reason);
			xmpp_stanza_release(reason);
		}

		xmpp_stanza_add_child(query, item);
		xmpp_stanza_release(item);

		entry = entry->next;
	}

	entry = removeban;
	while (entry)
	{
		item = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(item, "item");
		xmpp_stanza_set_attribute(item, "affiliation", "none");
		xmpp_stanza_set_attribute(item, "jid", entry->name);

		xmpp_stanza_add_child(query, item);
		xmpp_stanza_release(item);

		entry = entry->next;
	}

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq from='southampton@henryv.shakespeare.lit'
    id='ban3'
    to='kinghenryv@shakespeare.lit/throne'
    type='result'/>
*/

int Conn_HandlePing(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	struct pingdata_s *data = userdata;
	char *jid = data->jid;
	int pingtime = timeGetTime() - data->tick;
	int repeat = data->repeat;
	int show = data->show;
	free(data);

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandlePing(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	Model_AddPing(pingtime);

	if (type && stricmp(type, "error") == 0)
	{
		if (jid)
		{
			/*ModelConn_HandlePingError(jid);*/
		}
		/*return 1;*/
	}

	if (show)
	{
                ModelConn_HandlePing(jid, pingtime, repeat);
	}

	return 1;
}

/*
<iq type='get'
    from='romeo@montague.net/orchard'
    to='plays.shakespeare.lit'
    id='info1'>
  <query xmlns='http://jabber.org/protocol/disco#info'/>
</iq>
*/

void Conn_Ping(char *jid, int show, int repeat)
{
	xmpp_stanza_t *iq, *query;
	char id[256];
	struct pingdata_s *data = malloc(sizeof(*data));

	Conn_GenID(id, "PING");

	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	data->jid = strdup(jid);
	data->show = show;
	data->tick = timeGetTime();
	data->repeat = repeat;

	xmpp_id_handler_add(conn, Conn_HandlePing, id, data);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type='result' id='getlist1' to='romeo@example.net/orchard'>
<query xmlns='jabber:iq:privacy'>
  <active name='private'/>
  <default name='public'/>
  <list name='public'/>
  <list name='private'/>
  <list name='special'/>
</query>
</iq>
*/

int Conn_HandlePrivacyListNames(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandlePrivacyListNames(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		if (ns && stricmp(ns, "jabber:iq:privacy") == 0)
		{
			xmpp_stanza_t *active        = xmpp_stanza_get_child_by_name(query, "active");
			xmpp_stanza_t *defaultstanza = xmpp_stanza_get_child_by_name(query, "default");
			xmpp_stanza_t *list          = xmpp_stanza_get_child_by_name(query, "list");

			char *activename  = NULL;
			char *defaultname = NULL;
			struct namedlist_s *listnames = NULL;

			if (active)
			{
				activename = xmpp_stanza_get_attribute(active, "name");
			}

			if (defaultstanza)
			{
				defaultname = xmpp_stanza_get_attribute(defaultstanza, "name");
			}

			while (list)
			{
				char *listname = xmpp_stanza_get_attribute(list, "name");

				NamedList_AddString(&listnames, listname, NULL);

				list = xmpp_stanza_get_next_by_name(list, "list");
			}

			ModelConn_HandlePrivacyListNames(activename, defaultname, listnames);

			NamedList_Destroy(&listnames);
		}
	}

	return 1;
}

void Conn_GetPrivacyListNames()
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "GetPrivacyListNames");

	iq = Conn_CreateIQStanza("get", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:privacy");

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandlePrivacyListNames, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq type='result' id='getlist4' to='romeo@example.net/orchard'>
<query xmlns='jabber:iq:privacy'>
  <list name='special'>
    <item type='jid'
          value='juliet@example.com'
          action='allow'
          order='6'/>
    <item type='jid'
          value='benvolio@example.org'
          action='allow'
          order='7'/>
    <item type='jid'
          value='mercutio@example.org'
          action='allow'
          order='42'/>
    <item action='deny' order='666'/>
  </list>
</query>
</iq>
*/

int Conn_HandlePrivacyList(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandlePrivacyList(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		if (ns && stricmp(ns, "jabber:iq:privacy") == 0)
		{
			xmpp_stanza_t *list = xmpp_stanza_get_child_by_name(query, "list");

			if (list)
			{
				char *listname = xmpp_stanza_get_attribute(list, "name");
				xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(list, "item");

				struct namedlist_s *privacylist = NULL;

				while (item)
				{
					char *type   = xmpp_stanza_get_attribute(item, "type");
					char *value  = xmpp_stanza_get_attribute(item, "value");
					char *action = xmpp_stanza_get_attribute(item, "action");
					char *order  = xmpp_stanza_get_attribute(item, "order");
					struct privacylistentry_s *entry;

					entry = malloc(sizeof(*entry));
					memset(entry, 0, sizeof(*entry));

					entry->type   = strdup(type);
					entry->value  = strdup(value);
					entry->action = strdup(action);
					entry->order  = strdup(order);

					NamedList_Add(&privacylist, entry->value, entry, Info_DestroyPrivacyListEntry);
					item = xmpp_stanza_get_next_by_name(item, "item");
				}

				ModelConn_HandlePrivacyList(listname, privacylist);

				NamedList_Destroy(&privacylist);
			}
		}
	}
	else /* error */
	{
		ModelConn_HandlePrivacyList(userdata, NULL);
	}

	return 1;
}

int Conn_HandleChessparkPrivacyList(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleChessparkPrivacyList(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		/*if (ns && stricmp(ns, "http://onlinegamegroup.com/xml/chesspark-01") == 0)*/
		{
			xmpp_stanza_t *list = xmpp_stanza_get_child_by_name(query, "list");

			if (list)
			{
				char *listname = xmpp_stanza_get_attribute(list, "name");
				xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(list, "item");
				int order = 0;

				struct namedlist_s *privacylist = NULL;

				while (item)
				{
					char *jid   = xmpp_stanza_get_attribute(item, "jid");
					char ordertxt[64];

					struct privacylistentry_s *entry;

					entry = malloc(sizeof(*entry));
					memset(entry, 0, sizeof(*entry));

					sprintf(ordertxt, "%d", order);
					order++;

					entry->type   = strdup("jid");
					entry->value  = strdup(jid);
					entry->action = strdup("deny");
					entry->order  = strdup(ordertxt);

					NamedList_Add(&privacylist, entry->value, entry, Info_DestroyPrivacyListEntry);
					item = xmpp_stanza_get_next_by_name(item, "item");
				}

				ModelConn_HandleChessparkPrivacyList(privacylist);

				NamedList_Destroy(&privacylist);
			}
		}
	}
	else /* error */
	{
		ModelConn_HandleChessparkPrivacyList(NULL);
	}

	return 1;
}

/*
<iq from='romeo@example.net/orchard' type='get' id='getlist2'>
<query xmlns='jabber:iq:privacy'>
  <list name='public'/>
</query>
</iq>
*/

void Conn_GetPrivacyList(char *listname)
{
	xmpp_stanza_t *iq, *query, *list;
	char id[256];

	Conn_GenID(id, "GetPrivacyList");

	iq = Conn_CreateIQStanza("get", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:privacy");

	list = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(list, "list");
	xmpp_stanza_set_attribute(list, "name", listname);

	xmpp_stanza_add_child(query, list);
	xmpp_stanza_release(list);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandlePrivacyList, id, strdup(listname));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq from='romeo@example.net/orchard' type='set' id='active1'>
<query xmlns='jabber:iq:privacy'>
  <active name='special'/>
</query>
</iq>
*/

void Conn_SetActivePrivacyList(char *listname)
{
	xmpp_stanza_t *iq, *query, *active;
	char id[256];

	Conn_GenID(id, "SetActivePrivacyList");

	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:privacy");

	active = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(active, "active");

	if (listname)
	{
		xmpp_stanza_set_attribute(active, "name", listname);
	}

	xmpp_stanza_add_child(query, active);
	xmpp_stanza_release(active);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	/*xmpp_id_handler_add(conn, Conn_HandlePrivacyList, id, NULL);*/

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_SetDefaultPrivacyList(char *listname)
{
	xmpp_stanza_t *iq, *query, *defaultstanza;
	char id[256];

	Conn_GenID(id, "SetDefaultPrivacyList");

	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:privacy");

	defaultstanza = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(defaultstanza, "default");

	if (listname)
	{
		xmpp_stanza_set_attribute(defaultstanza, "name", listname);
	}

	xmpp_stanza_add_child(query, defaultstanza);
	xmpp_stanza_release(defaultstanza);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	/*xmpp_id_handler_add(conn, Conn_HandlePrivacyList, id, NULL);*/

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

int Conn_HandleSetPrivacyListResult(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	ModelConn_OnSetPrivacyList();
}

/*
<iq from='romeo@example.net/orchard' type='set' id='edit1'>
<query xmlns='jabber:iq:privacy'>
  <list name='public'>
    <item type='jid'
          value='tybalt@example.com'
          action='deny'
          order='3'/>
    <item type='jid'
          value='paris@example.org'
          action='deny'
          order='5'/>
    <item action='allow' order='68'/>
  </list>
</query>
</iq>
*/

void Conn_SetPrivacyList(char *listname, struct namedlist_s *privacylist)
{
	xmpp_stanza_t *iq, *query, *list, *item;
	struct namedlist_s *listentry;
	struct privacylistentry_s *entry;
	char id[256];

	Conn_GenID(id, "SetPrivacyList");

	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:privacy");

	list = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(list, "list");
	xmpp_stanza_set_attribute(list, "name", listname);

	listentry = privacylist;
	while (listentry)
	{
		entry = listentry->data;

		item = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(item, "item");
		xmpp_stanza_set_attribute(item, "type",   entry->type);
		xmpp_stanza_set_attribute(item, "value",  entry->value);
		xmpp_stanza_set_attribute(item, "action", entry->action);
		xmpp_stanza_set_attribute(item, "order",  entry->order);

		xmpp_stanza_add_child(list, item);
		xmpp_stanza_release(item);

		listentry = listentry->next;
	}

	xmpp_stanza_add_child(query, list);
	xmpp_stanza_release(list);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleSetPrivacyListResult, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_GetChessparkPrivacyList()
{
	xmpp_stanza_t *iq, *query, *list;
	char id[256];

	Conn_GenID(id, "GetChessparkPrivacyList");

	iq = Conn_CreateIQStanza("get", id, "match.chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://onlinegamegroup.com/xml/chesspark-01");

	list = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(list, "list");
	xmpp_stanza_set_attribute(list, "name", "ignore");

	xmpp_stanza_add_child(query, list);
	xmpp_stanza_release(list);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleChessparkPrivacyList, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_SetChessparkPrivacyList(struct namedlist_s *privacylist)
{
	xmpp_stanza_t *iq, *query, *list, *item;
	struct namedlist_s *listentry;
	struct privacylistentry_s *entry;
	char id[256];

	Conn_GenID(id, "SetChessparkPrivacyList");

	iq = Conn_CreateIQStanza("set", id, "match.chesspark.com");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "http://onlinegamegroup.com/xml/chesspark-01");

	list = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(list, "list");
	xmpp_stanza_set_attribute(list, "name", "ignore");

	listentry = privacylist;
	while (listentry)
	{
		entry = listentry->data;

		item = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(item, "item");
		xmpp_stanza_set_attribute(item, "jid", entry->value);

		xmpp_stanza_add_child(list, item);
		xmpp_stanza_release(item);

		listentry = listentry->next;
	}

	xmpp_stanza_add_child(query, list);
	xmpp_stanza_release(list);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq xmlns='jabber:client' type='result' id='7224:getprefs'>
 <query xmlns='jabber:iq:private'>
  <preferences xmlns='http://chesspark.com/xml/chesspark-01'>
   <statuses/>
   <favorite-channels>
    <channel jid='centralpark@chat.chesspark.com' autojoin='no'/>
    <channel jid='help@chat.chesspark.com' autojoin='no'/>
   </favorite-channels>
   <time-controls/>
   <noinitialroster/>
   <boardsize size='41'/>
   <showoffline/>
   <showavatars/>
  </preferences>
 </query>
</iq> 
*/
int Conn_HandleServerPrefs(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	Model_ShowLoginTime("gotprefs");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleServerPrefs(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (query)
	{
		char *ns = xmpp_stanza_get_ns(query);

		if (ns && stricmp(ns, "jabber:iq:private") == 0)
		{
			xmpp_stanza_t *preferences = xmpp_stanza_get_child_by_name(query, "preferences");

			while (preferences)
			{
				char *ns = xmpp_stanza_get_ns(query);

				if (ns && stricmp(ns, "http://chesspark.com/xml/chesspark-01"))
				{
#if 0
					xmpp_stanza_t *autoapprove       = xmpp_stanza_get_child_by_name(preferences, "autoapprove");
					xmpp_stanza_t *autoaway          = xmpp_stanza_get_child_by_name(preferences, "autoaway");
					            /* boardaligntop */
					xmpp_stanza_t *boardsize         = xmpp_stanza_get_child_by_name(preferences, "boardsize");
					            /* disconnectionlogout*/
					xmpp_stanza_t *hideparticipants  = xmpp_stanza_get_child_by_name(preferences, "hideparticipants");
					xmpp_stanza_t *hidepending       = xmpp_stanza_get_child_by_name(preferences, "hidepending");
					xmpp_stanza_t *hidewelcomedialog = xmpp_stanza_get_child_by_name(preferences, "hidewelcomedialog");
					            /* newlineonshift */
					xmpp_stanza_t *nochatnotify      = xmpp_stanza_get_child_by_name(preferences, "nochatnotify");
					xmpp_stanza_t *nogamenotify      = xmpp_stanza_get_child_by_name(preferences, "nogamenotify");
					xmpp_stanza_t *noinitialroster   = xmpp_stanza_get_child_by_name(preferences, "noinitialroster");
					xmpp_stanza_t *showavatars       = xmpp_stanza_get_child_by_name(preferences, "showavatars");
					xmpp_stanza_t *showmucpresenceinchat = xmpp_stanza_get_child_by_name(preferences, "showmucpresenceinchat");
					xmpp_stanza_t *showmucpresenceinchatwhenclosed = xmpp_stanza_get_child_by_name(preferences, "showmucpresenceinchatwhenclosed");
					xmpp_stanza_t *showoffline       = xmpp_stanza_get_child_by_name(preferences, "showoffline");
					
					xmpp_stanza_t *statuses          = xmpp_stanza_get_child_by_name(preferences, "statuses");
					xmpp_stanza_t *favoritechannels  = xmpp_stanza_get_child_by_name(preferences, "favorite-channels");
					xmpp_stanza_t *timecontrols      = xmpp_stanza_get_child_by_name(preferences, "time-controls");

					enum uiflags_e *newflags = 0;
					int autoawaytime = 10;

					if (autoapprove)
					{
						newflags |= UIFLAG_AUTOAPPROVEFRIEND;
					}

					if (autoaway)
					{
						char *minutes = xmpp_stanza_get_attribute(autoaway, "minutes");
						if (minutes)
						{
							sscanf(minutes, "%d", &autoawaytime);
						}
						newflags |= UIFLAG_AUTOAWAY;
					}

					if (boardsize)
					{
						/* IMPLEMENT ME */
					}

					if (hideparticipants)
					{
						newflags |= UIFLAG_HIDEPARTICIPANTS;
					}

					if (hidepending)
					{
						/* IMPLEMENT ME */
					}

					if (hidewelcomedialog)
					{
						newflags |= UIFLAG_DONTSHOWWELCOME;
					}

					if (nochatnotify)
					{
						newflags |= UIFLAG_NOALERTONMESSAGE;
					}

					if (nogamenotify)
					{
						newflags |= UIFLAG_NOALERTONGAME;
					}

					if (noinitialroster)
					{
						newflags |= UIFLAG_NOINITIALROSTER;
					}

					if (showavatars)
					{
						newflags |= UIFLAG_SHOWFRIENDAVATARS;
					}

					if (showmucpresenceinchat)
					{
						newflags |= UIFLAG_CHATSHOWNOTICES;
					}

					if (showmucpresenceinchatwhenclosed)
					{
						newflags |= UIFLAG_CHATNOTICESCLOSED;
					}

					if (showoffline)
					{
						newflags |= UIFLAG_SHOWFRIENDOFFLINE;
					}
#endif
					struct namedlist_s *favoritechats      = NULL;
					struct namedlist_s *autojoinchats      = NULL;
					struct namedlist_s *availablestatuses  = NULL;
					struct namedlist_s *awaystatuses       = NULL;
					struct namedlist_s *customtimecontrols = NULL;
					struct namedlist_s *options            = NULL;

					xmpp_stanza_t *child = xmpp_stanza_get_children(preferences);

					while (child)
					{
						char *childname = xmpp_stanza_get_name(child);

						if (!childname || !childname[0])
						{
							/* wtf */
						}
						else if (stricmp(childname, "favorite-channels") == 0)
						{
							xmpp_stanza_t *channel = xmpp_stanza_get_child_by_name(child, "channel");

							while (channel)
							{
								char *jid = xmpp_stanza_get_attribute(channel, "jid");
								char *autojoin = xmpp_stanza_get_attribute(channel, "autojoin");

								if (jid && jid[0])
								{
									NamedList_AddString(&favoritechats, jid, NULL);
									if (autojoin && stricmp(autojoin, "yes") == 0)
									{
										NamedList_AddString(&autojoinchats, jid, NULL);
									}
								}
								channel = xmpp_stanza_get_next_by_name(channel, "channel");
							}
						}
						else if (stricmp(childname, "statuses") == 0)
						{
							xmpp_stanza_t *available = xmpp_stanza_get_child_by_name(child, "available");
							xmpp_stanza_t *away = xmpp_stanza_get_child_by_name(child, "away");

							while (available)
							{
								char *status = xmpp_stanza_get_text(available);
								NamedList_AddString(&availablestatuses, status, xmpp_stanza_get_attribute(available, "last"));

								available = xmpp_stanza_get_next_by_name(available, "available");
							}

							while (away)
							{
								char *status = xmpp_stanza_get_text(away);
								NamedList_AddString(&awaystatuses, status, xmpp_stanza_get_attribute(away, "last"));

								away = xmpp_stanza_get_next_by_name(away, "away");
							}
						}
						else if (stricmp(childname, "time-controls") == 0)
						{
							xmpp_stanza_t *custom = xmpp_stanza_get_child_by_name(child, "custom");
							
							while (custom)
							{
								xmpp_stanza_t *timecontrol = xmpp_stanza_get_child_by_name(custom, "time-control");
								char *name = xmpp_stanza_get_attribute(custom, "name");
								struct tcpair_s *tcp = malloc(sizeof(*tcp));
								memset(tcp, 0, sizeof(*tcp));

								while (timecontrol)
								{
									char *side = xmpp_stanza_get_attribute(timecontrol, "side");

									if (!side || stricmp(side, "white") == 0 || stricmp(side, "both") == 0)
									{
										tcp->white = malloc(sizeof(*(tcp->white)));
										memset(tcp->white, 0, sizeof(*(tcp->white)));
										Conn_ParseTimeControl(timecontrol, &(tcp->white->delayinc), &(tcp->white->controlarray));
									}
									else
									{
										tcp->black = malloc(sizeof(*(tcp->black)));
										memset(tcp->black, 0, sizeof(*(tcp->black)));
										Conn_ParseTimeControl(timecontrol, &(tcp->black->delayinc), &(tcp->black->controlarray));
									}

									timecontrol = xmpp_stanza_get_next_by_name(timecontrol, "time-control");
								}

								NamedList_Add(&customtimecontrols, name, tcp, Info_DestroyTCPair);
								custom = xmpp_stanza_get_next_by_name(custom, "custom");
							}
						}
						else if (stricmp(childname, OPTION_AUTOAWAY) == 0)
						{
							char *minutes = xmpp_stanza_get_attribute(child, "minutes");
							NamedList_AddString(&options, childname, minutes);
						}
						else if (stricmp(childname, OPTION_BOARDSIZE) == 0)
						{
							char *size = xmpp_stanza_get_attribute(child, "size");
							NamedList_AddString(&options, childname, size);
						}
						else if (stricmp(childname, OPTION_VOLUME) == 0)
						{
							char *setting = xmpp_stanza_get_attribute(child, "setting");
							if (setting)
							{
								NamedList_AddString(&options, childname, setting);
							}
						}
						else if (stricmp(childname, OPTION_NOTIFICATIONTIME) == 0)
						{
							char *seconds = xmpp_stanza_get_attribute(child, "seconds");
							if (seconds)
							{
								NamedList_AddString(&options, childname, seconds);
							}
						}
						else if (stricmp(childname, OPTION_NOTIFICATIONLOCATION) == 0)
						{
							char *location = xmpp_stanza_get_attribute(child, "location");
							if (location)
							{
								NamedList_AddString(&options, childname, location);
							}
						}
						else if (stricmp(childname, OPTION_PIECESTHEME) == 0)
						{
							char *theme = xmpp_stanza_get_text(child);
							if (theme)
							{
								char *slash = strrchr(theme, '/');

								if (slash)
								{
									theme = slash + 1;
								}

								NamedList_AddString(&options, childname, theme);
							}
						}
						else if (stricmp(childname, OPTION_SEARCHFILTERS) == 0)
						{
							struct namedlist_s *searchfilters = NULL;
							xmpp_stanza_t *filter = xmpp_stanza_get_child_by_name(child, "filter");

							while (filter)
							{
								char *node = xmpp_stanza_get_attribute(filter, "node");
								struct gamesearchinfo_s *info;

								info = Conn_ParseGameSearchInfo(filter);
								NamedList_Add(&searchfilters, node, Info_DupeGameSearchInfo(info), Info_DestroyGameSearchInfo);
								filter = xmpp_stanza_get_next_by_name(filter, "filter");
							}
							NamedList_Add(&options, childname, searchfilters, NamedList_Destroy2);
						}
						else if (stricmp(childname, OPTION_LOGINROOMS) == 0)
						{
							struct namedlist_s *loginrooms = NULL;
							xmpp_stanza_t *room = xmpp_stanza_get_child_by_name(child, "room");

							while (room)
							{
								char *roomname = xmpp_stanza_get_text(room);

								NamedList_AddString(&loginrooms, roomname, roomname);

								room = xmpp_stanza_get_next_by_name(room, "room");
							}
							NamedList_Add(&options, childname, loginrooms, NamedList_Destroy2);
						}
						else
						{
							NamedList_AddString(&options, childname, NULL);
						}

						child = xmpp_stanza_get_next(child);
					}

					ModelConn_HandlePrefs(favoritechats, autojoinchats, availablestatuses, awaystatuses, customtimecontrols, options);
				}

				preferences = xmpp_stanza_get_next_by_name(preferences, "preferences");
			}
		}
	}
	else if (type && stricmp(type, "error") == 0)
	{
		/* no prefs, set everything to NULL */
		ModelConn_HandlePrefs(NULL, NULL, NULL, NULL, NULL, NULL);
	}

	return 1;
}
/*
<iq id='7224:getprefs' type='get'>
 <query xmlns='jabber:iq:private'>
  <preferences xmlns='http://chesspark.com/xml/chesspark-01'/>
 </query>
</iq>
*/

void Conn_GetServerPrefs()
{
	xmpp_stanza_t *iq, *query, *preferences;
	char id[256];

	Conn_GenID(id, "GetServerPrefs");

	iq = Conn_CreateIQStanza("get", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:private");

	preferences = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(preferences, "preferences");
	xmpp_stanza_set_ns(preferences, "http://chesspark.com/xml/chesspark-01");
	
	xmpp_stanza_add_child(query, preferences);
	xmpp_stanza_release(preferences);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleServerPrefs, id, NULL);

	Model_ShowLoginTime("getprefs");

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_SetServerPrefs(struct namedlist_s *outfavoritechats,
	struct namedlist_s *outautojoinchats,
	struct namedlist_s *outavailablestatuses,
	struct namedlist_s *outawaystatuses,
	struct namedlist_s *outcustomtimecontrols,
	struct namedlist_s *outoptions)
{
	xmpp_stanza_t *iq, *query, *preferences;
	struct namedlist_s *entry;
	char id[256];

	Conn_GenID(id, "SetServerPrefs");

	iq = Conn_CreateIQStanza("set", id, NULL);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:private");

	preferences = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(preferences, "preferences");
	xmpp_stanza_set_ns(preferences, "http://chesspark.com/xml/chesspark-01");

	entry = outoptions;
	while (entry)
	{
		xmpp_stanza_t *child = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(child, entry->name);

		if (stricmp(entry->name, OPTION_AUTOAWAY) == 0)
		{
			xmpp_stanza_set_attribute(child, "minutes", entry->data);
		}
		else if (stricmp(entry->name, OPTION_BOARDSIZE) == 0)
		{
			xmpp_stanza_set_attribute(child, "size", entry->data);
		}
		else if (stricmp(entry->name, OPTION_VOLUME) == 0)
		{
			xmpp_stanza_set_attribute(child, "setting", entry->data);
		}
		else if (stricmp(entry->name, OPTION_NOTIFICATIONTIME) == 0)
		{
			xmpp_stanza_set_attribute(child, "seconds", entry->data);
		}
		else if (stricmp(entry->name, OPTION_NOTIFICATIONLOCATION) == 0)
		{
			xmpp_stanza_set_attribute(child, "location", entry->data);
		}
		else if (stricmp(entry->name, OPTION_PIECESTHEME) == 0)
		{
			xmpp_stanza_t *text = xmpp_stanza_new(ctx);
			xmpp_stanza_set_text(text, entry->data);
			xmpp_stanza_add_child(child, text);
			xmpp_stanza_release(text);
		}
		else if (stricmp(entry->name, OPTION_SEARCHFILTERS) == 0)
		{
			struct namedlist_s *entry2 = entry->data;

			while (entry2)
			{
				struct gamesearchinfo_s *info = entry2->data;
				xmpp_stanza_t *filter = xmpp_stanza_new(ctx);

				xmpp_stanza_set_name(filter, "filter");
				xmpp_stanza_set_attribute(filter, "node", entry2->name);

				Conn_AddGameSearchInfo(filter, info);

				if (info->quickapply)
				{
					xmpp_stanza_set_attribute(filter, "apply", "yes");
				}

				if (info->filteropen)
				{
					xmpp_stanza_set_attribute(filter, "open", "yes");
				}

				if (info->hideown)
				{
					xmpp_stanza_t *hideown = xmpp_stanza_new(ctx);
					xmpp_stanza_set_name(hideown, "hide-own");

					xmpp_stanza_add_child(filter, hideown);
					xmpp_stanza_release(hideown);
				}

				xmpp_stanza_add_child(child, filter);
				xmpp_stanza_release(filter);

				entry2 = entry2->next;
			}
		}
		else if (stricmp(entry->name, OPTION_LOGINROOMS) == 0)
		{
			struct namedlist_s *entry2 = entry->data;

			while (entry2)
			{
				xmpp_stanza_t *room = xmpp_stanza_new(ctx);
				xmpp_stanza_t *roomtext = xmpp_stanza_new(ctx);

				xmpp_stanza_set_name(room, "room");

				xmpp_stanza_set_text(roomtext, entry2->name);

				xmpp_stanza_add_child(room, roomtext);
				xmpp_stanza_release(roomtext);

				xmpp_stanza_add_child(child, room);
				xmpp_stanza_release(room);

				entry2 = entry2->next;
			}
		}

		xmpp_stanza_add_child(preferences, child);
		xmpp_stanza_release(child);

		entry = entry->next;
	}

	if (outfavoritechats)
	{
                xmpp_stanza_t *favoritechannels = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(favoritechannels, "favorite-channels");

		entry = outfavoritechats;
		while (entry)
		{
			xmpp_stanza_t *channel = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(channel, "channel");

			xmpp_stanza_set_attribute(channel, "jid", entry->name);
			if (NamedList_GetByName(&outautojoinchats, entry->name))
			{
				xmpp_stanza_set_attribute(channel, "autojoin", "yes");
			}

			xmpp_stanza_add_child(favoritechannels, channel);
			xmpp_stanza_release(channel);

			entry = entry->next;
		}

		xmpp_stanza_add_child(preferences, favoritechannels);
		xmpp_stanza_release(favoritechannels);
	}

	if (outavailablestatuses || outawaystatuses)
	{
		xmpp_stanza_t *statuses = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(statuses, "statuses");

		entry = outavailablestatuses;
		while (entry)
		{
			xmpp_stanza_t *available, *availabletext;
			
			available = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(available, "available");

			availabletext = xmpp_stanza_new(ctx);
			xmpp_stanza_set_text(availabletext, entry->name);

			xmpp_stanza_add_child(available, availabletext);
			xmpp_stanza_release(availabletext);

			xmpp_stanza_add_child(statuses, available);
			xmpp_stanza_release(available);

			entry = entry->next;
		}

		entry = outawaystatuses;
		while (entry)
		{
			xmpp_stanza_t *away, *awaytext;
			
			away = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(away, "away");

			awaytext = xmpp_stanza_new(ctx);
			xmpp_stanza_set_text(awaytext, entry->name);

			xmpp_stanza_add_child(away, awaytext);
			xmpp_stanza_release(awaytext);

			xmpp_stanza_add_child(statuses, away);
			xmpp_stanza_release(away);

			entry = entry->next;
		}

		xmpp_stanza_add_child(preferences, statuses);
		xmpp_stanza_release(statuses);
	}

	if (outcustomtimecontrols)
	{
		xmpp_stanza_t *timecontrols = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(timecontrols, "time-controls");

		entry = outcustomtimecontrols;
		while (entry)
		{
			struct tcpair_s *tcp = entry->data;
			xmpp_stanza_t *custom, *wtc;

			custom = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(custom, "custom");
			if (entry->name)
			{
				xmpp_stanza_set_attribute(custom, "name", entry->name);
			}

			wtc = Conn_CreateTimeControl2(tcp->white->delayinc, tcp->white->controlarray, 1);

			if (tcp->black)
			{
				xmpp_stanza_set_attribute(wtc, "side", "white");
			}
			else
			{
				xmpp_stanza_set_attribute(wtc, "side", "both");
			}

			xmpp_stanza_add_child(custom, wtc);
			xmpp_stanza_release(wtc);

			if (tcp->black)
			{
				xmpp_stanza_t *btc = Conn_CreateTimeControl2(tcp->black->delayinc, tcp->black->controlarray, 1);
				xmpp_stanza_set_attribute(btc, "side", "black");

				xmpp_stanza_add_child(custom, btc);
				xmpp_stanza_release(btc);
			}

			xmpp_stanza_add_child(timecontrols, custom);
			xmpp_stanza_release(custom);

			entry = entry->next;
		}

		xmpp_stanza_add_child(preferences, timecontrols);
		xmpp_stanza_release(timecontrols);
	}
	
	xmpp_stanza_add_child(query, preferences);
	xmpp_stanza_release(preferences);

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	/*xmpp_id_handler_add(conn, Conn_HandleServerPrefs, id, NULL);*/

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq
    type='result'
    to='romeo@montague.net/orchard'
    from='juliet@capulet.com/balcony'
    id='version_1'>
  <query xmlns='jabber:iq:version'>
    <name>Exodus</name>
    <version>0.7.0.4</version>
    <os>Windows-XP 5.01.2600</os>
  </query>
</iq>
 */

int Conn_HandleVersion(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandleVersion(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (query)
	{
		xmpp_stanza_t *name    = xmpp_stanza_get_child_by_name(query, "name");
		xmpp_stanza_t *version = xmpp_stanza_get_child_by_name(query, "version");
		xmpp_stanza_t *os      = xmpp_stanza_get_child_by_name(query, "os");
		char *nametxt = NULL;
		char *versiontxt = NULL;
		char *ostxt = NULL;
		char *from = xmpp_stanza_get_attribute(stanza, "from");

		if (name)
		{
			nametxt = xmpp_stanza_get_text(name);
		}

		if (version)
		{
			versiontxt = xmpp_stanza_get_text(version);
		}

		if (os)
		{
			ostxt = xmpp_stanza_get_text(os);
		}

		/*ModelConn_SetProfileVersion(from, nametxt, versiontxt, ostxt);*/
	}

	return 0;
}

/*
<iq
    type='get'
    from='romeo@montague.net/orchard'
    to='juliet@capulet.com/balcony'
    id='version_1'>
  <query xmlns='jabber:iq:version'/>
</iq>
*/

void Conn_RequestVersion(char *jid)
{
	xmpp_stanza_t *iq, *query;
	char id[256];

	Conn_GenID(id, "RequestVersion");

	iq = Conn_CreateIQStanza("get", id, jid);

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, "jabber:iq:version");

	xmpp_stanza_add_child(iq, query);
	xmpp_stanza_release(query);

	xmpp_id_handler_add(conn, Conn_HandleVersion, id, NULL);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq to='search.chesspark.com' from='chessparkrocks@chesspark.com/cpc' type='set' id='search1'>
  <search xmlns='http://onlinegamegroup.com/xml/chesspark-01'
     type='off'/>
</iq>
*/

void Conn_SetPushGamesActivation(int active)
{
	xmpp_stanza_t *iq, *search;
	char id[256];

	Conn_GenID(id, "SetPush");

	iq = Conn_CreateIQStanza("set", id, "search.chesspark.com");

	search = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(search, "search");
	xmpp_stanza_set_ns(search, "http://onlinegamegroup.com/xml/chesspark-01");
	if (active)
	{
		xmpp_stanza_set_attribute(search, "node", "on");
	}
	else
	{
		xmpp_stanza_set_attribute(search, "node", "off");
	}

	xmpp_stanza_add_child(iq, search);
	xmpp_stanza_release(search);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

/*
<iq id="AACyLSuV:PlayAd" xmlns="jabber:client" type="error" to="someone@chesspark.com/cpc" from="search.chesspark.com">
 <play-ad id="33214"/>
 <error type="cancel">
  <internal-service-error xmlns="urn:ietf:params:xml:ns:xmpp-stanzas"/>
 </error>
 </iq>
*/
int Conn_HandlePlayAdReturn(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
	char *type, *id, *name;
	xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, "query");

	type = xmpp_stanza_get_type(stanza);
	id = xmpp_stanza_get_id(stanza);
	name = xmpp_stanza_get_name(stanza);
	Log_Write(0, "Conn_HandlePlayAdReturn(): got a stanza, type \"%s\" id \"%s\" name \"%s\"\n", type, id, name);

	if (type && stricmp(type, "error") == 0)
	{
		Conn_HandleGameRequestError(stanza, NULL, NULL, userdata);
	}

	return 0;
}

/*
<iq to='search.chesspark.com' from='chessparkrocks@chesspark.com/cpc' type='set' id='search1'>
    <play-ad id='41'/>
</iq>
*/

void Conn_PlayAd(char *adid)
{
	xmpp_stanza_t *iq, *playad;
	char id[256];

	Conn_GenID(id, "PlayAd");

	iq = Conn_CreateIQStanza("set", id, "search.chesspark.com");

	playad = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(playad, "play-ad");
	xmpp_stanza_set_attribute(playad, "id", adid);

	xmpp_stanza_add_child(iq, playad);
	xmpp_stanza_release(playad);

	xmpp_id_handler_add(conn, Conn_HandlePlayAdReturn, id, strdup(adid));

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}

void Conn_SetPushFilter(char *node, struct gamesearchinfo_s *info)
{
	xmpp_stanza_t *iq, *search;
	char id[256];

	Conn_GenID(id, "pushfilter");

	iq = Conn_CreateIQStanza("set", id, "search.chesspark.com");

	search = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(search, "search");
	xmpp_stanza_set_ns(search, "http://onlinegamegroup.com/xml/chesspark-01");
	xmpp_stanza_set_attribute(search, "type", node);

	if (info)
	{
		xmpp_stanza_t *filter;

		filter = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(filter, "filter");

		if (info->variant)
		{
			xmpp_stanza_t *variant;

			variant = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(variant, "variant");
			xmpp_stanza_set_attribute(variant, "name", info->variant);
			
			xmpp_stanza_add_child(filter, variant);

			xmpp_stanza_release(variant);
		}

		if (info->keywords)
		{
			xmpp_stanza_t *keywords, *keywordstext;
			char *text = EscapeParsedText(info->keywords);

			keywords = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(keywords, "keywords");

			keywordstext = xmpp_stanza_new(ctx);
			xmpp_stanza_set_text(keywordstext, text);

			xmpp_stanza_add_child(keywords, keywordstext);
			
			xmpp_stanza_release(keywordstext);
			
			xmpp_stanza_add_child(filter, keywords);

			xmpp_stanza_release(keywords);

			free(text);
		}

		if (info->rated)
		{
			xmpp_stanza_t *rated;

			rated = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(rated, "rated");

			xmpp_stanza_add_child(filter, rated);

			xmpp_stanza_release(rated);
		}		

		if (info->computers)
		{
			xmpp_stanza_t *computer;

			computer = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(computer, "computer");

			xmpp_stanza_add_child(filter, computer);

			xmpp_stanza_release(computer);
		}

		if (info->timecontrolrange)
		{
			xmpp_stanza_t *timecontrolrange;

			timecontrolrange = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(timecontrolrange, "time-control-range");

			xmpp_stanza_set_attribute(timecontrolrange, "name", info->timecontrolrange);

			xmpp_stanza_add_child(filter, timecontrolrange);

			xmpp_stanza_release(timecontrolrange);
		}

		if (info->titled)
		{
			xmpp_stanza_t *titled;

			titled = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(titled, "titled");

			xmpp_stanza_add_child(filter, titled);

			xmpp_stanza_release(titled);
		}

		if (info->limit)
		{
			xmpp_stanza_t *limit;

			limit = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(limit, "limit");

			xmpp_stanza_set_attribute(limit, "type", info->limit->type);

			if (info->limit->low != -1)
			{
				char txt[120];

				sprintf(txt, "%d", info->limit->low);
				xmpp_stanza_set_attribute(limit, "low", txt);
			}

			if (info->limit->high != -1)
			{
				char txt[120];

				sprintf(txt, "%d", info->limit->high);
				xmpp_stanza_set_attribute(limit, "high", txt);
			}

			xmpp_stanza_add_child(filter, limit);

			xmpp_stanza_release(limit);
		}

		if (info->relativerating)
		{
			xmpp_stanza_t *relativerating;
			char txt[20];

			sprintf(txt, "%d", info->relativerating);

			relativerating = Conn_CreateTextStanza("relative-rating", txt);

			xmpp_stanza_add_child(filter, relativerating);
			xmpp_stanza_release(relativerating);
		}

		if (info->filterunrated)
		{
			xmpp_stanza_t *filterunrated;

			filterunrated = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(filterunrated, "filter-unrated");

			xmpp_stanza_add_child(filter, filterunrated);
			xmpp_stanza_release(filterunrated);
		}

		if (info->pairingtype)
		{
			xmpp_stanza_t *pairing, *pairingtext;

			pairing = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(pairing, "pairing");

			pairingtext = xmpp_stanza_new(ctx);

			if (stricmp(info->pairingtype, "Swiss Pairing") == 0)
			{
                                xmpp_stanza_set_text(pairingtext, "s");
			}
			else if (stricmp(info->pairingtype, "Round Robin") == 0)
			{
                                xmpp_stanza_set_text(pairingtext, "r");
			}
			else if (stricmp(info->pairingtype, "Elimination") == 0)
			{
                                xmpp_stanza_set_text(pairingtext, "e");
			}

			xmpp_stanza_add_child(pairing, pairingtext);
			
			xmpp_stanza_release(pairingtext);
			
			xmpp_stanza_add_child(filter, pairing);

			xmpp_stanza_release(pairing);
		}

		/*
		if (info->correspondence)
		{
			xmpp_stanza_t *correspondence;

			correspondence = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(correspondence, "correspondence");

			xmpp_stanza_add_child(filter, correspondence);

			xmpp_stanza_release(correspondence);
		}
		*/

		if (info->hideown)
		{
			xmpp_stanza_t *hideown = xmpp_stanza_new(ctx);
			xmpp_stanza_set_name(hideown, "hide-own");

			xmpp_stanza_add_child(filter, hideown);
			xmpp_stanza_release(hideown);
		}

		if (info->groupids)
		{
			xmpp_stanza_t *groups = xmpp_stanza_new(ctx);
			struct namedlist_s *entry;

			xmpp_stanza_set_name(groups, "groups");

			entry = info->groupids;
			while (entry)
			{
				xmpp_stanza_t *group = Conn_CreateTextStanza("group", entry->name);

				xmpp_stanza_add_child(groups, group);
				xmpp_stanza_release(group);

				entry = entry->next;
			}

			xmpp_stanza_add_child(filter, groups);
			xmpp_stanza_release(groups);
		}
		
		xmpp_stanza_add_child(search, filter);

		xmpp_stanza_release(filter);
	}
	
	xmpp_stanza_add_child(iq, search);

	xmpp_stanza_release(search);

	xmpp_send(conn, iq);

	xmpp_stanza_release(iq);
}
