#include "clientlibrary.h"
#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "net_api.h"
#include "filesystem_shared.h"
#include "SteamClientHelper.h"
#include "richpresence.h"
#include "fmod/soundengine.h"

#ifdef XASH_BUILD
#include <winsock2.h>
#include <windows.h>

// Handle to the steam_broker.exe child process, if we launched it.
static HANDLE g_hBrokerProcess = NULL;

// Finds steam_broker.exe and steam_api.dll next to the engine executable.
// If both exist, launches the broker and sets cl_ticket_generator to "steam".
// Silently does nothing if either file is absent.
static void SteamBroker_Launch()
{
	char exeDir[MAX_PATH];
	if (!GetModuleFileNameA(NULL, exeDir, MAX_PATH))
		return;
	char* slash = strrchr(exeDir, '\\');
	if (!slash) return;
	*slash = '\0';

	char brokerPath[MAX_PATH], dllPath[MAX_PATH];
	_snprintf(brokerPath, MAX_PATH, "%s\\steam_broker.exe", exeDir);
	_snprintf(dllPath,    MAX_PATH, "%s\\steam_api.dll",    exeDir);

	if (GetFileAttributesA(brokerPath) == INVALID_FILE_ATTRIBUTES) return;
	if (GetFileAttributesA(dllPath)    == INVALID_FILE_ATTRIBUTES) return;

	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_MINIMIZE; // visible but out of the way

	PROCESS_INFORMATION pi = {};
	char cmdLine[MAX_PATH];
	_snprintf(cmdLine, MAX_PATH, "\"%s\"", brokerPath);

	if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
	                   CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
	                   NULL, exeDir, &si, &pi))
	{
		g_hBrokerProcess = pi.hProcess;
		CloseHandle(pi.hThread);
		// Tell Xash3D to use the broker for Steam auth tickets.
		// The cvar is read at connect time, so no need to wait for the broker to finish init.
		gEngfuncs.pfnClientCmd("cl_ticket_generator steam\n");
		gEngfuncs.Con_Printf("[MSC] Steam Broker launched (Steam auth enabled).\n");
	}
	else
	{
		gEngfuncs.Con_Printf("[MSC] Steam Broker found but failed to launch (error %lu).\n",
		                     GetLastError());
	}
}

// Sends sb_terminate to the broker and waits for clean exit.
static void SteamBroker_Shutdown()
{
	if (!g_hBrokerProcess) return;

	DWORD exitCode = 0;
	if (!GetExitCodeProcess(g_hBrokerProcess, &exitCode) || exitCode != STILL_ACTIVE)
	{
		CloseHandle(g_hBrokerProcess);
		g_hBrokerProcess = NULL;
		return;
	}

	// Ask the broker to shut down gracefully via its UDP command
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
	{
		SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s != INVALID_SOCKET)
		{
			struct sockaddr_in addr = {};
			addr.sin_family      = AF_INET;
			addr.sin_port        = htons(27420);
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			const char* msg = "sb_terminate";
			sendto(s, msg, (int)strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
			closesocket(s);
		}
		WSACleanup();
	}

	// Give it 2 seconds to exit, then force-kill
	if (WaitForSingleObject(g_hBrokerProcess, 2000) == WAIT_TIMEOUT)
		TerminateProcess(g_hBrokerProcess, 0);

	CloseHandle(g_hBrokerProcess);
	g_hBrokerProcess = NULL;
}
#endif // XASH_BUILD

CSoundEngine gSoundEngine;
CRichPresence gRichPresence;
CHud gHUD;

bool CClientLibrary::Initialize() 
{
	logfile << Logger::LOG_INFO << "[INIT: Loading filesystem]\n";
	if (!FileSystem_Init())
		return false;

	logfile << Logger::LOG_INFO << "[INIT: Loading sound engine]\n";
	if (!gSoundEngine.InitFMOD())
		return false;

	logfile << Logger::LOG_INFO << "[INIT: Loading HUD]\n";
	gHUD.Init();

	return true;
}

void CClientLibrary::PostInitialize()
{

}

void CClientLibrary::HUDInit()
{
	//Client CVARs
	//JAN2010_11 - not optional - used to blind player sometimes
	//CVAR_CREATE( "ms_showhudimgs", "1", FCVAR_CLIENTDLL );		// Drigien MAY2008 - Shows/Hides The HUD Images
	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE);				// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					// controls whether or not to automatically take screenshots at the end of a round

	CVAR_CREATE("ms_clgender", "0", FCVAR_CLIENTDLL);					// Thothie FEB2011_22 - Adding a cvar to store client gender
	CVAR_CREATE("ms_xpdisplay", "0", FCVAR_ARCHIVE);					// Thothie AUG2007a - XP Display Options
	CVAR_CREATE("ms_developer", "0", FCVAR_CLIENTDLL);					// Thothie MAR2007b - Hides client side developer messages when set to 0

	CVAR_CREATE("ms_help", "1", FCVAR_ARCHIVE); // Whether help tips are shown
	CVAR_CREATE("ms_reflect", "1", FCVAR_ARCHIVE);						// Allow reflective surfaces
	CVAR_CREATE("ms_reflect_dbg", "0", FCVAR_ARCHIVE);					// Debug reflective surfaces
	//CVAR_CREATE("ms_reconnect_delay", "5", FCVAR_ARCHIVE);				// Thothie AUG2017 - Make reconnect delay adjustable client side
	CVAR_CREATE("ms_quickslot_timeout", "2.5", FCVAR_ARCHIVE);		// Timeout for the quickslots
	CVAR_CREATE("ms_autocharge", "1", FCVAR_ARCHIVE);					// MiB MAR2012_05 - Let's you auto-charge your attack
	CVAR_CREATE("ms_doubletapdodge", "0", FCVAR_ARCHIVE);				// MiB MAR2012_05 -Enable/Disable double tapping to dodge
	CVAR_CREATE("ms_showotherglow", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_chargebar_sound", "magic/chargebar_alt1.wav", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_glowcolor", "(255,255,255)", FCVAR_ARCHIVE); // This is called in player think and was breaking the entire think cycle.
	CVAR_CREATE("ms_chargebar_volume", "15", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_doubletap_delay", "0.45", FCVAR_ARCHIVE); // The higher the amount, the longer the player has to hit left/right/back for a second time
	CVAR_CREATE("ms_sprint_verbose", "2", FCVAR_ARCHIVE); // 0 for no messages , 1 for only warnings , 2 for everything
	CVAR_CREATE("ms_sprint_toggle", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_sprint_doubletap", "1", FCVAR_ARCHIVE);
	//ui
	CVAR_CREATE("ms_status_icons", "1", FCVAR_CLIENTDLL); // Drigien MAY2008 - Shows/Hides The HUD Status Icons
	CVAR_CREATE("msui_id_offsetx", "100", FCVAR_ARCHIVE);
	CVAR_CREATE("msui_id_offsety", "-30", FCVAR_ARCHIVE);
	CVAR_CREATE("msui_id_background", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_evthud_decaytime", "5", FCVAR_ARCHIVE);				// Time each line in the Event Console lasts before it shrinks
	CVAR_CREATE("ms_evthud_history", "10", FCVAR_ARCHIVE);				// Max number of text lines to keep in the Event Console history
	CVAR_CREATE("ms_evthud_size", "5", FCVAR_ARCHIVE);					// Max number of text lines shown at once
	CVAR_CREATE("ms_evthud_bgtrans", "0", FCVAR_CLIENTDLL);				// Transparency of the background
	CVAR_CREATE("ms_txthud_decaytime", "9", FCVAR_ARCHIVE);				// Time each line in the Event Console lasts before it shrinks
	CVAR_CREATE("ms_txthud_history", "50", FCVAR_ARCHIVE);				// Max number of text lines to keep in the Event Console history
	CVAR_CREATE("ms_txthud_size", "8", FCVAR_ARCHIVE);					// Max number of text lines shown at once
	CVAR_CREATE("ms_txthud_bgtrans", "0", FCVAR_ARCHIVE);				// Transparency of the background
	CVAR_CREATE("ms_txthud_width", "640", FCVAR_ARCHIVE);				// Width of console
	CVAR_CREATE("ms_hidehud", "0", FCVAR_ARCHIVE);						// Hides the HUD and viewmodel completely
	CVAR_CREATE("ms_lildude", "1", FCVAR_ARCHIVE);						// Thothie MAR2007a - Hides the 3d Guy if set 0
	CVAR_CREATE("ms_invtype", "1", FCVAR_ARCHIVE);						// MiB FEB2012_12 - Inventory types (added post-doc by Thothie)
	CVAR_CREATE("ms_alpha_inventory", "0", FCVAR_ARCHIVE); // MiB FEB2019_24 [ALPHABETICAL_INVENTORY]
	CVAR_CREATE("ms_doubleclicktime", "0.5", FCVAR_ARCHIVE);
	CVAR_CREATE("ms_scrollamount", "30", FCVAR_ARCHIVE);

	//debug outputs ; remove later - always add where they are set! This way let's us get data after something goes wrong out of a string
	CVAR_CREATE("DEBUG_bestxpstat", "0", FCVAR_CLIENTDLL); // Called @ playerstats.cpp line 123

	gRichPresence.Init();

#ifdef XASH_BUILD
	SteamBroker_Launch();
#endif
}

void CClientLibrary::VideoInit()
{
	ResetClient(); // this gets called on start of every map

	logfile << Logger::LOG_INFO << "[INIT: Video Init]\n";
	gHUD.VidInit();
}

void CClientLibrary::Shutdown()
{
	logfile << Logger::LOG_INFO << "[INIT: Shutdown]\n";
#ifdef XASH_BUILD
	SteamBroker_Shutdown();
#endif
	gHUD.Shutdown();
	FileSystem_Shutdown();
	gSoundEngine.ExitFMOD();
	gRichPresence.Shutdown();
}

void CClientLibrary::ResetClient()
{
	logfile << Logger::LOG_INFO << "[INIT: Reset Client]\n";
	gHUD.ReloadClient();
}

void CClientLibrary::RunFrame() 
{
	net_status_t status;
	gEngfuncs.pNetAPI->Status(&status);

	bool isConnected = status.connected != 0;

	// An attempt to detect disconnect to stop all sounds. should make it work with map change as well.
	if (isConnected != m_IsConnected || m_ConnectionTime > status.connection_time)
	{
		ResetClient();

		m_IsConnected = isConnected;
		m_ConnectionTime = status.connection_time;
	}

	gSoundEngine.Update();
	steamhelper->Think();
	gRichPresence.Update();
}

extern CClientLibrary gClient;