#include "SessionPlugin.h"

#include <sstream>
#include <iomanip>

#include <bakkesmod/plugin/bakkesmodsdk.h>
#include <bakkesmod/wrappers/arraywrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>

#define CVAR_NAME_DISPLAY "cl_session_plugin_display"
#define CVAR_NAME_SHOULD_LOG "cl_session_plugin_should_log"
#define CVAR_NAME_DISPLAY_X "cl_session_plugin_display_x"
#define CVAR_NAME_DISPLAY_Y "cl_session_plugin_display_y"
#define CVAR_NAME_RESET "cl_session_plugin_reset"

#define HOOK_COUNTDOWN_BEGINSTATE "Function GameEvent_TA.Countdown.BeginState"
#define HOOK_PLAYER_SCORED "Function TAGame.GameEvent_Soccar_TA.EventPlayerScored"
#define HOOK_ON_WINNER_SET "Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet"
#define HOOK_MATCH_ENDED "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
#define HOOK_EVENT_DESTROYED "Function TAGame.GameEvent_TA.EventDestroyed"
#define HOOK_ON_MAIN_MENU "Function TAGame.OnlineGame_TA.OnMainMenuOpened"

BAKKESMOD_PLUGIN( ssp::SessionPlugin, "Session plugin (shows session stats)", "1.1", 0 )

ssp::SessionPlugin::SessionPlugin():
	stats(),
	currentMatch(),
	renderer(),
	steamID(),
	displayStats( std::make_shared<bool>(true) ),
	shouldLog( std::make_shared<bool>( false ) )
{ }

void ssp::SessionPlugin::onLoad()
{
	// Variable initialization
	ResetStats();

	// log startup to console
	cvarManager->log( std::string(exports.pluginName) + std::string( " version: ") + std::string( exports.pluginVersion) );

	// CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY, "1", "Display session stats", false, true, 0, true, 1, true ).bindTo( displayStats );
	cvarManager->registerCvar( CVAR_NAME_SHOULD_LOG, "0", "Should log stats to console", false, true, 0, true, 1, true ).bindTo( shouldLog );

	// Renderer CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_X, "420", "X position of the display", false, true, 0, true, 3840, true ).bindTo( renderer.posX );
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_Y, "0", "Y position of the display", false, true, 0, true, 2160, true ).bindTo( renderer.posY );

	// CVar Hooks
	cvarManager->registerNotifier( CVAR_NAME_RESET, [this] ( std::vector<string> params ) {
		ResetStats();
	}, "Start a fresh stats session", PERMISSION_ALL );

	// Hook event: Match start
	gameWrapper->HookEvent( HOOK_COUNTDOWN_BEGINSTATE, bind( &SessionPlugin::StartGame, this, std::placeholders::_1 ) );

	// Hook event: Player scored
	gameWrapper->HookEvent( HOOK_PLAYER_SCORED, bind( &SessionPlugin::OnPlayerScored, this, std::placeholders::_1 ) );

	// Hook event: Match end
	gameWrapper->HookEvent( HOOK_ON_WINNER_SET, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );
	gameWrapper->HookEvent( HOOK_MATCH_ENDED, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );

	// Hook event: Match destroyed
	gameWrapper->HookEvent( HOOK_EVENT_DESTROYED, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );

	// Hook event: On main menu
	gameWrapper->HookEventPost( HOOK_ON_MAIN_MENU, bind( &SessionPlugin::InMainMenu, this, std::placeholders::_1 ) );

	// Register drawable
	gameWrapper->RegisterDrawable( std::bind( &SessionPlugin::Render, this, std::placeholders::_1 ) );
}


void ssp::SessionPlugin::onUnload()
{
	ResetStats();

	gameWrapper->UnregisterDrawables();

	gameWrapper->UnhookEvent( HOOK_COUNTDOWN_BEGINSTATE );
	gameWrapper->UnhookEvent( HOOK_PLAYER_SCORED );
	gameWrapper->UnhookEvent( HOOK_ON_WINNER_SET );
	gameWrapper->UnhookEvent( HOOK_MATCH_ENDED );
	gameWrapper->UnhookEvent( HOOK_EVENT_DESTROYED );
	gameWrapper->UnhookEvent( HOOK_ON_MAIN_MENU );
}

void ssp::SessionPlugin::InMainMenu( std::string eventName )
{
	// Try to update the current MMR if a steam id is known and a playlist session is active
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()) )
	{
		if( *shouldLog )
		{
			cvarManager->log( "---------" );
			cvarManager->log( "Main menu" );
		}

		if( steamID.ID > 0 )
		{
			// Update MMR stats
			UpdateCurrentMmr( 10 );
		}

		if( *shouldLog )
		{
			cvarManager->log( "---------" );
		}
	}
}

void ssp::SessionPlugin::OnPlayerScored( std::string eventName )
{
	// Try to update the current MMR if a match and a playlist session are active
	if( currentMatch.IsActive() && stats.find( static_cast<int>(currentMatch.GetMatchType())) != stats.end() )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		if( *shouldLog )
		{
			cvarManager->log( "-------------" );
		}

		gameWrapper->SetTimeout( [this] ( GameWrapper *gameWrapper ) {
			// Get the current score to update the current game data (since a player scored)
			currentMatch.SetCurrentGameGoals(gameWrapper);

			if( *shouldLog )
			{
				ssp::match::Result matchResult = currentMatch.GetStanding();
				cvarManager->log( "A player scored! Current score: " + std::to_string( currentMatch.GetTeamScore(0) ) + " - " + std::to_string( currentMatch.GetTeamScore( 1 ) ) );
				cvarManager->log( "Your team is " + std::string(matchResult == ssp::match::Result::WIN ? "winning" : ( matchResult == ssp::match::Result::LOSS ? "losing" : "tied" ) ) + "!" );
				cvarManager->log( "-------------" );
			}
		}, 0.2 );
	}
}

void ssp::SessionPlugin::StartGame( std::string eventName )
{
	// Only mark the start of a new game when none is active yet
	if( !currentMatch.IsActive() )
	{
		if( *shouldLog )
		{
			cvarManager->log( "-------------" );
		}

		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		// When we're in a vlid game, we can get the server wrapper
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();

		// Initialize current match data
		currentMatch.OnMatchStart(&*gameWrapper); 

		// Receive steam ID
		steamID.ID = gameWrapper->GetSteamID();

		// Get the MMRWrapper to receive the current playlist and the player's initial MMR
		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();

		// Set the current playlist type to display the correct values
		currentMatch.SetMatchType(mmrWrapper.GetCurrentPlaylist());

		// Update MMR stats
		UpdateCurrentMmr( 10 );

		// Get initial MMR if playlist wasn't tracked yet
		int matchType = static_cast<int>( currentMatch.GetMatchType() );
		if( stats.find( matchType) == stats.end() )
		{
			// Initialize session stats whenplaylist hasn't been played yet during this session
			float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );

			stats[matchType] = ssp::playlist::Stats{ mmr, mmr, 0, 0, 0 };
		}

		if( *shouldLog )
		{
			cvarManager->log( "Game started!" );
			cvarManager->log( "Received SteamID: " + std::to_string( steamID.ID ) );
			cvarManager->log( "Player is on team " + std::to_string( currentMatch.GetCurrentTeam() ) );
			if( matchType >= 0 )
			{
				cvarManager->log( "MMR: " + std::to_string( stats[matchType].currentMmr ) );
			}
			cvarManager->log( "Current playlist: " + ssp::playlist::GetName( currentMatch.GetMatchType() ) );
			cvarManager->log( "-------------" );
		}
	}
	else
	{
		if( *shouldLog )
		{
			cvarManager->log( "The game already started. Ignoring this event." );
		}
	}
}

void ssp::SessionPlugin::EndGame( std::string eventName )
{
	// Only mark the end of a new game when one is active and the current playlist exists
	/// just in case the player reset his stats just before the end of th game (you never know :) ) 
	int matchType = static_cast<int>( currentMatch.GetMatchType() );
	if( currentMatch.IsActive() && stats.find( matchType ) != stats.end() )
	{
		// Check if we are good to process the state of the game
		if( !CheckValidGame() )
			return;

		// End current match
		currentMatch.Deactivate();

		if( *shouldLog )
		{
			cvarManager->log( "-------------" );
		}

		// Determine win/loss
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		if( !serverWrapper.IsNull() )
		{
			// Get final score
			currentMatch.SetCurrentGameGoals(&*gameWrapper);

			// Update playlist stats
			currentMatch.SetWinOrLoss( stats[matchType]);
		}

		// Reset current match
		currentMatch.MatchEndReset();

		if( *shouldLog )
		{
			cvarManager->log( "Game over!" );
			cvarManager->log( "Wins: " + std::to_string(stats[matchType].wins) );
			cvarManager->log( "Losses: " + std::to_string( stats[matchType].losses ) );
			cvarManager->log( "Streak: " + std::to_string( stats[matchType].streak ) );
			cvarManager->log( "-------------" );
		}

		// Update MMR stats
		UpdateCurrentMmr( 10 );
	} // If currentgame is active
}

void ssp::SessionPlugin::DestroyedGame( std::string eventName )
{
	int matchType = static_cast<int>( currentMatch.GetMatchType() );
	if( currentMatch.IsActive() && stats.find( matchType ) != stats.end() )
	{ // We know that we are not in a valid game anymore, so we'll have to work with the latest know score
		// End current game
		currentMatch.Deactivate();

		if( *shouldLog )
		{
			cvarManager->log( "-------------" );
			cvarManager->log( "Game destroyed! Rage quit? Stay happy!" );
		}

		// Update playlist stats
		currentMatch.SetWinOrLoss( stats[matchType]);

		// Reset current match
		currentMatch.MatchEndReset();


		if( *shouldLog )
		{
			cvarManager->log( "Wins: " + std::to_string( stats[matchType].wins ) );
			cvarManager->log( "Losses: " + std::to_string( stats[matchType].losses ) );
			cvarManager->log( "Streak: " + std::to_string( stats[matchType].streak ) );
			cvarManager->log( "-------------" );
		}

		// Update MMR stats
		UpdateCurrentMmr(10);
	}
}

void ssp::SessionPlugin::Render( CanvasWrapper canvas )
{
	ssp::playlist::Type matchType = currentMatch.GetMatchType();
	auto currentPlaylistStats = stats.find( static_cast<int>( currentMatch.GetMatchType() ) );

	// Only render if its allowed to, the playlist is known and there are stats to show
	if( *displayStats && matchType != ssp::playlist::Type::PLAYLIST_UNKOWN && currentPlaylistStats != stats.end() )
	{
		// Render current session stats
		renderer.RenderStats( &canvas, currentPlaylistStats->second, currentMatch.GetMatchType() );
	}
}

void ssp::SessionPlugin::ResetStats()
{
	// Clear all session stats
	stats.clear();

	// Completely reset the current match data
	currentMatch.FullReset();
}

bool ssp::SessionPlugin::CheckValidGame()
{
	// The game may only be tracked if the game is an online game and the online game is online multiplayer
	if( gameWrapper->IsInOnlineGame() && !gameWrapper->IsInReplay() && !gameWrapper->IsInFreeplay() )
	{
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		return serverWrapper.IsOnlineMultiplayer();
	}

	return false;
}

void ssp::SessionPlugin::UpdateCurrentMmr(int retryCount)
{
	// Only update MMR if we know what playlist type to update
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()))
	{
		// Convert match type to int (for easy map usage)
		int matchType = static_cast<int>( currentMatch.GetMatchType() );

		gameWrapper->SetTimeout( [matchType, retryCount, this] ( GameWrapper *gameWrapper ) {
			// Only try to receive MMR if we have tries left
			if( retryCount >= 0 )
			{
				if( *shouldLog )
				{
					cvarManager->log( "Trying to get the player MMR (try: " + std::to_string( retryCount ) + ")" );
				}

				// Check if the MMR is currently synced
				MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
				if( mmrWrapper.IsSynced( steamID, matchType ) && !mmrWrapper.IsSyncing( steamID ) )
				{
					// Get the player MMR
					float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );
					if( *shouldLog )
					{
						cvarManager->log( "Received MMR! (" + std::to_string( mmr ) + ")" );
					}

					// Check if it updated relative to the currently known MMR
					if( mmr != stats[matchType].currentMmr )
					{
						// If it updated, we update the session data too
						stats[matchType].currentMmr = mmr;
						if( *shouldLog )
						{
							float mmrGain = stats[matchType].currentMmr - stats[matchType].initialMmr;
							std::stringstream mmrGainStream;
							mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
							cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+": "" ) + mmrGainStream.str() );
						}
					}
					else
					{
						// When it's the same MMR, the player may not have gained any MMR,
						// but since that chance is very slim, we try again.
						if( *shouldLog )
						{
							cvarManager->log( "The MMR was the same. It may not have changed, but retrying just in case!" );
						}
						UpdateCurrentMmr( retryCount - 1 );
						return;
					}
				}
				else
				{
					if( *shouldLog )
					{
						cvarManager->log( "Couldn't get the MMR because it was syncing.." );
						cvarManager->log( "Try to force it." );
					}

					// When the MMR wasn't synced, it may happen that the MMR is already updated but didn't register its syncing state (yet)
					// So, we just try to receive it anyway and check if the player's MMR changed 
					MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
					float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );
					
					// When it's the same MMR, the player may not have gained any MMR,
					// but since that chance is very slim, we try again.
					if( stats[matchType].currentMmr == mmr )
					{
						if( *shouldLog )
						{
							cvarManager->log( "Received MMR is the same as it already was, so it's probably not updated. Retrying.." );
						}
						UpdateCurrentMmr( retryCount - 1 );
						return;
					}
					else
					{
						// If it updated, we update the session data too
						stats[matchType].currentMmr = mmr;

						if( *shouldLog )
						{
							float mmrGain = stats[matchType].currentMmr - stats[matchType].initialMmr;
							std::stringstream mmrGainStream;
							mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
							cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+" : "" ) + mmrGainStream.str() );
						}
					}
				}
			}
		}, 1.f );
	}
}