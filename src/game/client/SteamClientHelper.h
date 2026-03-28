#ifndef MS_STEAM_CLIENT_HELPER_H
#define MS_STEAM_CLIENT_HELPER_H

#ifndef XASH_BUILD
#include <steam/steam_api.h>

class CSteamClientHelper
{
public:
	CSteamClientHelper();
	~CSteamClientHelper();

	void Think();
	void SetAchievement(const char* str);
	void SetStat(const char* str, int value);

private:
	bool m_bHasLoadedSteamStats;
	STEAM_CALLBACK(CSteamClientHelper, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(CSteamClientHelper, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored);
	STEAM_CALLBACK(CSteamClientHelper, OnAchievementStored, UserAchievementStored_t, m_CallbackAchievementStored);
};

extern CSteamAPIContext* steamapicontext;
extern CSteamClientHelper* steamhelper;
#else
// Stub for Xash3D FWGS builds (no Steam dependency)
class CSteamClientHelper
{
public:
	void Think() {}
	void SetAchievement(const char*) {}
	void SetStat(const char*, int) {}
};
extern CSteamClientHelper* steamhelper;
#endif // XASH_BUILD

#endif // MS_STEAM_HELPER_H