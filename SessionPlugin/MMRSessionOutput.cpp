#include "MMRSessionOutput.h"

#include <fstream>

#include <bakkesmod/wrappers/wrapperstructs.h>

#include <SessionPlugin.h>

#define MMR_OUTPUT_CURRENT_PLAYER 0

#define MMR_OUTPUT_FILE_FOLDER "SessionPlugin"
#define MMR_OUTPUT_FILE_NAME "MMR_"
#define MMR_OUTPUT_FILE_EXTENSION ".csv"

#define HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE "Function OnlineGameJoinGame_X.WaitForAllPlayers.BeginState"
#define HOOK_ON_MAIN_MENU "Function TAGame.GFxData_MainMenu_TA.MainMenuAdded"
#define HOOK_COUNTDOWN_BEGINSTATE "Function GameEvent_TA.Countdown.BeginState"
#define HOOK_GAMEEVENTSOCCAR_EVENTMATCHWINNERSET "Function TAGame.GameEvent_Soccar_TA.EventMatchWinnerSet"


ssp::MMRSessionOutput::MMRSessionOutput() :
	cvarMMROutputter(std::make_shared<bool>(false)),
	outputOtherGain(std::make_shared<bool>(false)),
	plugin(nullptr),
	MMRNotifierToken(),
	currentPlaylist(ssp::playlist::Type::PLAYLIST_UNKNOWN),
	currentPlayerId(),
	mmrPlayers(std::vector<MMR_ENTRY>())
{ }

ssp::MMRSessionOutput::~MMRSessionOutput()
{ 
	Reset();

	if( plugin != nullptr )
	{
		plugin->gameWrapper->UnhookEventPost( HOOK_COUNTDOWN_BEGINSTATE );
		plugin->gameWrapper->UnhookEventPost( HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE );
		plugin->gameWrapper->UnhookEventPost( HOOK_ON_MAIN_MENU );
		plugin->gameWrapper->UnhookEventPost( HOOK_GAMEEVENTSOCCAR_EVENTMATCHWINNERSET );
	}

	plugin = nullptr;
}

void ssp::MMRSessionOutput::Initialize(SessionPlugin* newPlugin)
{
	if (newPlugin == nullptr)
		return;

	plugin = newPlugin;

	SSP_LOG("[INITIALIZE FLAG] 1.");

	// Hook to Countdown Begin State
	plugin->gameWrapper->HookEventPost(HOOK_COUNTDOWN_BEGINSTATE, std::bind(&MMRSessionOutput::CountDown_BeginState, this, std::placeholders::_1));
	// Hook to Wait for all players begin state
	plugin->gameWrapper->HookEventPost( HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE, std::bind(&MMRSessionOutput::WaitForAllPlayers_BeginState, this, std::placeholders::_1));
	// Hook to Online Game On Main Menu
	plugin->gameWrapper->HookEventPost(HOOK_ON_MAIN_MENU, std::bind(&MMRSessionOutput::OnlineGame_OnMainMenu, this, std::placeholders::_1));
	// Hook to Game Event Soccar Event Match Winner Set
	plugin->gameWrapper->HookEventPost(HOOK_GAMEEVENTSOCCAR_EVENTMATCHWINNERSET, std::bind(&MMRSessionOutput::GameEventSoccar_EventMatchWinnerSet, this, std::placeholders::_1));
	SSP_LOG("[INITIALIZE FLAG] 2.");

	// Create folder if it doesn't exist yet
	CreateDirectory( ( plugin->gameWrapper->GetDataFolder() / MMR_OUTPUT_FILE_FOLDER ).string().c_str(), NULL );
}

void ssp::MMRSessionOutput::Reset()
{
	SSP_LOG("[RESET FLAG] 1.");
	MMRNotifierToken.release();
	currentPlaylist = ssp::playlist::Type::PLAYLIST_UNKNOWN;
	mmrPlayers.clear();
	SSP_LOG("[RESET FLAG] 2.");
}

void ssp::MMRSessionOutput::WaitForAllPlayers_BeginState(std::string eventName)
{
	SSP_LOG("[WAITFOR ALL PLAYERS FLAG] 1.");
	WriteToFile(true, true);
	SSP_LOG("[WAITFOR ALL PLAYERS FLAG] 2.");
}

void ssp::MMRSessionOutput::CountDown_BeginState(std::string eventName)
{
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 1.");
	if (plugin == nullptr)
		return;

	SSP_LOG("[COUNT DOWN BEGIN FLAG] 2. ");

	SSP_LOG("[COUNT DOWN BEGIN FLAG] 3. " + std::to_string(currentPlayerId.GetUID()));
	if (currentPlayerId.GetUID() == ID_INVALID_VALUE)
	{
		currentPlayerId = plugin->gameWrapper->GetUniqueID();
	}
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 4. " + std::to_string(currentPlayerId.GetUID()));

	if (!plugin->CheckValidGame())
		return;

	// Get current playlist and find game size
	MMRWrapper mmrWrapper = plugin->gameWrapper->GetMMRWrapper();
	ssp::playlist::Type playlist = static_cast<ssp::playlist::Type>(mmrWrapper.GetCurrentPlaylist());
	int gameSize = ssp::playlist::GetPlaylistGameSize(playlist);

	SSP_LOG("[COUNT DOWN BEGIN FLAG] 5. " + std::to_string(static_cast<int>(playlist)) + " , " + std::to_string(static_cast<int>(gameSize)));
	if (gameSize <= 0)
		return;

	// Find the team of the current player
	unsigned char currentTeam = TEAM_INVALID_VALUE;
	CarWrapper car = plugin->gameWrapper->GetLocalCar();
	if (!car.IsNull())
	{
		currentTeam = car.GetTeamNum2();
	}
	else return;
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 6. " + std::to_string(currentTeam) + " == " + std::to_string(TEAM_INVALID_VALUE));
	if (currentTeam == TEAM_INVALID_VALUE)
		return;

	SSP_LOG("[COUNT DOWN BEGIN FLAG] 7. " + std::to_string(mmrPlayers.size()) + " != " + std::to_string(gameSize));
	// Calibrate mmrPlayers size to the game's size
	if (mmrPlayers.size() != gameSize)
		mmrPlayers.clear();

	while (mmrPlayers.size() < gameSize)
	{
		SSP_LOG("[COUNT DOWN BEGIN FLAG] 7.1 " + std::to_string(mmrPlayers.size()) + " < " + std::to_string(gameSize));
		MMR_ENTRY temp = std::make_pair(UniqueIDWrapper(), ssp::MMR(MMR_INVALID_VALUE));
		SSP_LOG("[COUNT DOWN BEGIN FLAG] 7.2 " + std::to_string(temp.first.GetUID()) + " < " + std::to_string(temp.second.current));
		mmrPlayers.push_back(temp);
	}
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 8. " + std::to_string(mmrPlayers.size()));

	// Set current playlist
	currentPlaylist = playlist;

	// Find all players and store their MMR if possible
	int memberCounter[2] = { 1, gameSize / 2 };
	ArrayWrapper<PriWrapper> gameMembers = plugin->gameWrapper->GetOnlineGame().GetPRIs();
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 9. " + std::to_string(gameMembers.Count()));

	for (int m = 0; m < gameMembers.Count(); ++m)
	{
		// Get unique ID of PRI
		PriWrapper gameMember = gameMembers.Get(m);
		UniqueIDWrapper id = gameMember.GetUniqueIdWrapper();

		SSP_LOG("[COUNT DOWN BEGIN FLAG] 11. " + std::to_string(id.GetUID()));

		bool hasMMRDetermined = false;
		for (int i = 0; i < mmrPlayers.size(); ++i)
		{
			SSP_LOG("[COUNT DOWN BEGIN FLAG] 12. " + std::to_string(mmrPlayers[i].first.GetUID()) + " -> " + std::to_string(mmrPlayers[i].second.initial));
			if (mmrPlayers[i].first.GetUID() == id.GetUID() && mmrPlayers[i].first.GetUID() != ID_INVALID_VALUE && mmrPlayers[i].second.initial != MMR_INVALID_VALUE)
			{
				hasMMRDetermined = true;
				break;
			}
			SSP_LOG("[COUNT DOWN BEGIN FLAG] 13. " + std::to_string(hasMMRDetermined));
		}
		SSP_LOG("[COUNT DOWN BEGIN FLAG] 14. " + std::to_string(hasMMRDetermined));
		if (hasMMRDetermined)
			continue;

		float mmr = mmrWrapper.GetPlayerMMR(id, static_cast<int>(currentPlaylist));

		SSP_LOG("[COUNT DOWN BEGIN FLAG] 15. " + std::to_string(mmr));
		// Set current player mmr if we're talking to the current player
		if (id.GetUID() == currentPlayerId.GetUID())
		{
			mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].first = id;
			mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.Reset(mmr);
			SSP_LOG("[COUNT DOWN BEGIN FLAG] 16. " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial));
		}
		else
		{
			// Get MMR from random player
			int index = memberCounter[gameMember.GetTeamNum() == currentTeam ? 0 : 1]++;
			SSP_LOG("[COUNT DOWN BEGIN FLAG] 17. " + std::to_string(gameMember.GetTeamNum()));
			mmrPlayers[index].first = id;
			mmrPlayers[index].second.Reset(mmr);
			SSP_LOG("[COUNT DOWN BEGIN FLAG] 18. " + std::to_string(index) + " -- " + std::to_string(mmrPlayers[index].second.initial));
		}
	}
	SSP_LOG("[COUNT DOWN BEGIN FLAG] 19.");
}

void ssp::MMRSessionOutput::OnlineGame_OnMainMenu(std::string eventName)
{
	SSP_LOG("[ONLINE GAME ON MAIN MENU FLAG] 1.");
	WriteToFile(false, true);
	SSP_LOG("[ONLINE GAME ON MAIN MENU FLAG] 2.");
}

void ssp::MMRSessionOutput::GameEventSoccar_EventMatchWinnerSet(std::string eventName)
{
	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 1.");
	if (plugin == nullptr)
		return;
	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 2.");

	// Try to fetch all mmr immediately
	ArrayWrapper<PriWrapper> gameMembers = plugin->gameWrapper->GetOnlineGame().GetPRIs();
	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 3.");

	bool foundAllMmr = true;
	for (int m = 0; m < gameMembers.Count(); ++m)
	{
		// Get unique ID of PRI
		PriWrapper gameMember = gameMembers.Get(m);
		UniqueIDWrapper id = gameMember.GetUniqueIdWrapper();

		for (int i = 0; i < mmrPlayers.size(); ++i)
		{
			SSP_LOG("[EVENT MATCH WINNER SET FLAG] 4. " + std::to_string(id.GetUID()) + " , " + std::to_string(mmrPlayers[i].first.GetUID()) + " , " + std::to_string(mmrPlayers[i].second.current));
			if (mmrPlayers[i].first.GetUID() == id.GetUID() && mmrPlayers[i].first.GetUID() != ID_INVALID_VALUE && (mmrPlayers[i].second.current == MMR_INVALID_VALUE || mmrPlayers[i].second.initial == mmrPlayers[i].second.current))
			{
				if (!mmrPlayers[i].second.RequestMmrUpdate(&*plugin->gameWrapper, id, currentPlaylist, false))
				{
					foundAllMmr = false;
				}
				SSP_LOG("[EVENT MATCH WINNER SET FLAG] 5. " + std::to_string(foundAllMmr) + " , " + std::to_string(mmrPlayers[i].second.current));
				break;
			}
		}

		if (!foundAllMmr)
			break;
	}

	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 6. " + std::to_string(foundAllMmr));
	if (foundAllMmr)
	{
		WriteToFile(false, false);
		return;
	}

	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 7.");

	// Register to MMR notifier
	MMRNotifierToken = plugin->gameWrapper->GetMMRWrapper().RegisterMMRNotifier(std::bind(&ssp::MMRSessionOutput::MMRWrapper_Notifier, this, std::placeholders::_1));
	SSP_LOG("[EVENT MATCH WINNER SET FLAG] 8.");
}

void ssp::MMRSessionOutput::MMRWrapper_Notifier(UniqueIDWrapper uniqueID)
{
	SSP_LOG("[MMR NOTIFIER FLAG] 1. " + std::to_string(mmrPlayers.size()) + " -- " + std::to_string(uniqueID.GetUID()));
	if (plugin == nullptr || mmrPlayers.size() <= 0)
		return;

	SSP_LOG("[MMR NOTIFIER FLAG] 2. " + std::to_string(uniqueID.GetUID()) + " == " + std::to_string(currentPlayerId.GetUID()));
	// If we're talking to the current player
	if (uniqueID.GetUID() == currentPlayerId.GetUID())
	{
		SSP_LOG("[MMR NOTIFIER FLAG] 3. " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial) + " == " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current));
		if (mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial == mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current)
		{
			mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.RequestMmrUpdate(&*plugin->gameWrapper, uniqueID, currentPlaylist, true);
			SSP_LOG("[MMR NOTIFIER FLAG] 4. " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current));
		}
	}
	else
	{
		SSP_LOG("[MMR NOTIFIER FLAG] 5.");
		// Handle other players
		float mmr = MMR_INVALID_VALUE;
		for (int i = 0; i < mmrPlayers.size(); ++i)
		{
			SSP_LOG("[MMR NOTIFIER FLAG] 6. " + std::to_string(mmrPlayers[i].first.GetUID()) + " == " + std::to_string(uniqueID.GetUID()) + " || " + std::to_string(ID_INVALID_VALUE));
			if (mmrPlayers[i].first.GetUID() == uniqueID.GetUID() && mmrPlayers[i].first.GetUID() != ID_INVALID_VALUE)
			{
				mmrPlayers[i].second.RequestMmrUpdate(&*plugin->gameWrapper, uniqueID, currentPlaylist, true);
				SSP_LOG("[MMR NOTIFIER FLAG] 7. " + std::to_string(mmrPlayers[i].second.current));
				break;
			}
		}
	}
	SSP_LOG("[MMR NOTIFIER FLAG] 8.");

	bool allSetCorrectly = HaveAllMMRBeenSetCorrectly();
	SSP_LOG("[MMR NOTIFIER FLAG] 9." + std::to_string(allSetCorrectly));

	if (allSetCorrectly) {
		SSP_LOG("[MMR NOTIFIER FLAG] 10.");
		WriteToFile(false, true);
	}
	SSP_LOG("[MMR NOTIFIER FLAG] 11.");
}

void ssp::MMRSessionOutput::TestEvent(std::string eventName)
{
	SSP_LOG(" [TEST EVENT FLAG] " + eventName);
}

void ssp::MMRSessionOutput::WriteToFile(bool hardForce, bool forceIfNotAllCorrect)
{
	SSP_LOG("[WRITE TO FILE FLAG] 1. " + std::to_string(mmrPlayers.size()) + " -- " + std::to_string( hardForce ));
	if (plugin == nullptr || mmrPlayers.size() <= 0)
		return;
	SSP_LOG("[WRITE TO FILE FLAG] 2. " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial) + " == " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current));

	// Get current player mmr if the end/new mmr wasn't set
	if (mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial == mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current)
	{
		SSP_LOG("[WRITE TO FILE FLAG] 3. ");
		// Stop executing this method if the mmr didn't update and we're not forced to write
		if (!mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.RequestMmrUpdate(&*plugin->gameWrapper, currentPlayerId, currentPlaylist, true) && !hardForce )
		{
			SSP_LOG("[WRITE TO FILE FLAG] 4." + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial) + " == " + std::to_string(mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current));
			return;
		}
	}
	SSP_LOG("[WRITE TO FILE FLAG] 5." + std::to_string( mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.initial ) + " == " + std::to_string( mmrPlayers[MMR_OUTPUT_CURRENT_PLAYER].second.current ) );

	// Return if not all mmr has been set (yet) and there's no force write requested
	if (!forceIfNotAllCorrect ) {
		if (!HaveAllMMRBeenSetCorrectly()) {
			return;
		}
	}

	std::ofstream outfile;
	std::string fn = (plugin->gameWrapper->GetDataFolder() / MMR_OUTPUT_FILE_FOLDER / (
		MMR_OUTPUT_FILE_NAME +
		ssp::playlist::GetPlaylistName(currentPlaylist) +
		MMR_OUTPUT_FILE_EXTENSION
		)).generic_string();

	SSP_LOG("[WRITE TO FILE FLAG] 6. " + fn);

	outfile.open(fn, std::ios_base::app); // append
	if (!outfile.is_open())
		return;

	SSP_LOG("[WRITE TO FILE FLAG] 7. ");

	// Output starting mmrs
	for (int i = 0; i < mmrPlayers.size(); ++i)
	{
		SSP_LOG("[WRITE TO FILE FLAG] 8. " + std::to_string(mmrPlayers[i].second.initial));
		outfile << mmrPlayers[i].second.initial << ",";
	}

	SSP_LOG("[WRITE TO FILE FLAG] 9. ");
	// Output mmr gain(s)
	for (int i = 0; i < mmrPlayers.size(); ++i)
	{
		SSP_LOG("[WRITE TO FILE FLAG] 10. " + std::to_string(*outputOtherGain) + " -- " + std::to_string(mmrPlayers[i].first.GetUID()) + " -- " + std::to_string(currentPlayerId.GetUID()) + " -- " + std::to_string(mmrPlayers[i].second.lastDiff));
		// Output all gains if allowed
		if (*outputOtherGain)
		{
			SSP_LOG("[WRITE TO FILE FLAG] 11. ");
			outfile << mmrPlayers[i].second.lastDiff << ",";
		}
		// Else only output current player mmr
		else if (mmrPlayers[i].first.GetUID() == currentPlayerId.GetUID())
		{
			SSP_LOG("[WRITE TO FILE FLAG] 12. ");
			outfile << mmrPlayers[i].second.lastDiff << ",";
			break;
		}
	}
	SSP_LOG("[WRITE TO FILE FLAG] 13. ");

	outfile << std::endl;

	SSP_LOG("[WRITE TO FILE FLAG] 14. ");
	// Close file and reset to collect new data
	outfile.close();
	Reset();
	SSP_LOG("[WRITE TO FILE FLAG] 15. ");

}

bool ssp::MMRSessionOutput::HaveAllMMRBeenSetCorrectly()
{
	if (mmrPlayers.size() <= 0)
		return false;

	for (int i = 0; i < mmrPlayers.size(); ++i) {
		if (mmrPlayers[i].first.GetUID() == ID_INVALID_VALUE ||
			mmrPlayers[i].second.initial == MMR_INVALID_VALUE ||
			mmrPlayers[i].second.current == MMR_INVALID_VALUE ||
			mmrPlayers[i].second.initial == mmrPlayers[i].second.current)
			return false;
	}

	return true;
}