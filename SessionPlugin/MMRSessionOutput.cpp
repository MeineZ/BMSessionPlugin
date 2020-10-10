#include "MMRSessionOutput.h"

#include <fstream>

#define MMR_OUTPUT_CURRENT_PLAYER 0

#define MMR_OUTPUT_FILE_PATH ".\\bakkesmod\\data\\SessionPlugin\\MMR_"
#define MMR_OUTPUT_FILE_EXTENSION ".csv"

ssp::MMRSessionOutput::MMRSessionOutput() :
	didDetermine(false),
	stopMMRfetch(false)
{
#ifdef SSP_SETTINGS_DEBUG_MMR_OUTPUT
	allMMR[ssp::playlist::Type::PLAYLIST_DUEL] = std::vector<float>(2);
#endif
	allMMR[ssp::playlist::Type::PLAYLIST_RANKEDDUEL] = std::vector<float>( 2 );
	allMMR[ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES] = std::vector<float>( 4 );
	allMMR[ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD] = std::vector<float>( 6 );
	allMMR[ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD] = std::vector<float>( 6 );
}

ssp::MMRSessionOutput::~MMRSessionOutput()
{
	allMMR.clear();
}

void ssp::MMRSessionOutput::OnNewGame( CVarManagerWrapper *cvarManager, GameWrapper * gameWrapper, ssp::playlist::Type playlist, ssp::MMR & currentPlayerMMR, SteamID & currentPlayerSteamID, int currentTeam)
{ 
	stopMMRfetch = false;
	int gameSize = GetPlaylistGameSize( playlist );
	if( gameSize > 0 )
	{
		// The game may only be tracked if the game is an online game and the online game is online multiplayer
		if( gameWrapper->IsInOnlineGame() && !gameWrapper->IsInReplay() && !gameWrapper->IsInFreeplay() )
		{
			ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
			if( serverWrapper.IsOnlineMultiplayer() )
			{
				int memberCounter[2] = { 1, gameSize / 2 };
				ArrayWrapper<PriWrapper> gameMembers = serverWrapper.GetPRIs();
				if( gameMembers.Count() == gameSize )
				{
					for( int i = 0; i < gameMembers.Count(); ++i )
					{
						PriWrapper gameMember = gameMembers.Get( i );
						SteamID id = gameMember.GetUniqueId();
						if( id.ID == currentPlayerSteamID.ID )
						{
							allMMR[playlist][MMR_OUTPUT_CURRENT_PLAYER] = currentPlayerMMR.current;
						}
						else
						{
							// Get MMR from random player
							RequestMMR( gameWrapper, id, playlist, 50, [this, cvarManager, gameSize, gameWrapper, playlist,  &memberCounter, &gameMember, currentTeam] ( float newMMR ) {
								allMMR[playlist][memberCounter[gameMember.GetTeamNum() == currentTeam ? 0 : 1]++] = newMMR;
							});
						}
					}
				}
				else
				{
					allMMR[playlist][MMR_OUTPUT_CURRENT_PLAYER] = -1.0f;
				}
			} // is in online game
		} // Is in valid game
	} // if gameSize > 0
}

void ssp::MMRSessionOutput::OnEndGame(CVarManagerWrapper * cvarManager, ssp::playlist::Type playlist, ssp::MMR & currentPlayerMMR)
{
	stopMMRfetch = true;

	int gameSize = GetPlaylistGameSize( playlist );
	if( gameSize > 0 )
	{
		if( allMMR[playlist][MMR_OUTPUT_CURRENT_PLAYER] < 0.0f )
		{
			return;
		}

		std::ofstream outfile;
		outfile.open( MMR_OUTPUT_FILE_PATH + GetPlaylistFileName(playlist) + MMR_OUTPUT_FILE_EXTENSION, std::ios_base::app ); // append
		for( int i = 0; i < gameSize; ++i )
		{
			outfile << allMMR[playlist][i] << ",";
		}
		outfile << currentPlayerMMR.lastDiffDisplay << ",\n";

		outfile.close();
	}
}

void ssp::MMRSessionOutput::RequestMMR( GameWrapper *gameWrapper, SteamID &steamId, const ssp::playlist::Type matchType, int retryCount, std::function<void( float )> onSuccess )
{
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	ssp::MMR mmr( 0.0f );
	if( !mmr.RequestMmrUpdate( gameWrapper, steamId, &matchType, false ) )
	{
		if( !mmr.RequestMmrUpdate( gameWrapper, steamId, &matchType, true ) )
		{
			allMMR[matchType][MMR_OUTPUT_CURRENT_PLAYER] = -1.0f;
			if( !stopMMRfetch )
			{
				gameWrapper->SetTimeout( [this, gameWrapper, steamId, matchType, retryCount, onSuccess] ( GameWrapper *gameWrapper ) {
					this->RequestMMR( gameWrapper, const_cast<SteamID &>( steamId ), matchType, retryCount - 1, onSuccess );
				}, 2.f );
			}
			return;
		}
	}

	if( onSuccess )
	{
		onSuccess(mmr.current);
	}
}
