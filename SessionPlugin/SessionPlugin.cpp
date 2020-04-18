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

#define TITLE_COLOR 255, 255, 255, 127
#define LABEL_COLOR 255, 255, 255, 150
#define GREEN_COLOR 95, 232, 95, 127
#define RED_COLOR 232, 95, 95, 127

BAKKESMOD_PLUGIN( ssp::SessionPlugin, "Session plugin (shows session stats)", "1.1", 0 )

#ifdef DEBUGGING
void SessionPlugin::testHook( std::string eventName )
{
	cvarManager->log( "-----------------------" );
	cvarManager->log( eventName );
	cvarManager->log( "-----------------------" );
}
#endif

void ssp::SessionPlugin::onLoad()
{
	// Variable initialization
	ResetStats();

	// log startup to console
	cvarManager->log( std::string(exports.pluginName) + std::string( " version: ") + std::string( exports.pluginVersion) );

	// CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY, "1", "Display session stats", false, true, 0, true, 1, true ).bindTo(display_stats);
	cvarManager->registerCvar( CVAR_NAME_SHOULD_LOG, "0", "Should log stats to console", false, true, 0, true, 1, true ).bindTo( should_log );
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_X, "420", "X position of the display", false, true, 0, true, 3840, true ).bindTo( displayMetrics.positionX );
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_Y, "0", "Y position of the display", false, true, 0, true, 2160, true ).bindTo( displayMetrics.positionY );

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
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()) )
	{
		if( *should_log )
		{
			cvarManager->log( "---------" );
			cvarManager->log( "Main menu" );
		}

		if( steamID.ID > 0 )
		{
			// Update MMR stats
			UpdateCurrentMmr( 10 );
		}

		if( *should_log )
		{
			cvarManager->log( "---------" );
		}
	}
}

void ssp::SessionPlugin::OnPlayerScored( std::string eventName )
{
	if( currentMatch.IsActive() && stats.find( static_cast<int>(currentMatch.GetMatchType())) != stats.end() )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		if( *should_log )
		{
			cvarManager->log( "-------------" );
		}

		gameWrapper->SetTimeout( [this] ( GameWrapper *gameWrapper ) {
			currentMatch.SetCurrentGameGoals(gameWrapper);

			if( *should_log )
			{
				ssp::match::Result matchResult = currentMatch.GetStanding();
				cvarManager->log( "A player scored! Current score: " + std::to_string( currentMatch.GetTeamScore(0) ) + " - " + std::to_string( currentMatch.GetTeamScore( 1 ) ) );
				cvarManager->log( "Your team is " + std::string(matchResult == ssp::match::Result::WIN ? "winning" : ( matchResult == ssp::match::Result::LOSS ? "losing" : "tied" ) ) + "!" );
				cvarManager->log( "-------------" );
			}
		}, 0.25 );
	}
}

void ssp::SessionPlugin::StartGame( std::string eventName )
{
	if( *should_log )
	{
		cvarManager->log( "-------------" );
	}

	if( !currentMatch.IsActive() )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();

		currentMatch.OnMatchStart(&*gameWrapper);

		steamID.ID = gameWrapper->GetSteamID();

		// Get current playlist
		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();

		currentMatch.SetMatchType(mmrWrapper.GetCurrentPlaylist());

		// Get initial MMR if playlist wasn't tracked yet
		int matchType = static_cast<int>( currentMatch.GetMatchType() );
		if( stats.find( matchType) == stats.end() )
		{
			float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );

			stats[matchType] = ssp::playlist::Stats{ mmr, mmr, 0, 0, 0 };
		}

		if( *should_log )
		{
			cvarManager->log( "Game started!" );
			cvarManager->log( "Received SteamID: " + std::to_string( steamID.ID ) );
			cvarManager->log( "Player is on team " + std::to_string( currentMatch.GetCurrentTeam() ) );
			if( matchType >= 0 )
			{
				cvarManager->log( "MMR: " + std::to_string( stats[matchType].currentMmr ) );
			}
			cvarManager->log( "Current playlist: " + ssp::playlist::GetName( currentMatch.GetMatchType() ) );
		}
	}
	else
	{
		if( *should_log )
		{
			cvarManager->log( "The game already started. Ignoring this event." );
		}
	}

	if( *should_log )
	{
		cvarManager->log( "-------------" );
	}
}

void ssp::SessionPlugin::EndGame( std::string eventName )
{
	int matchType = static_cast<int>( currentMatch.GetMatchType() );
	if( currentMatch.IsActive() && stats.find( matchType ) != stats.end() )
	{
		// Check if we are good to process the state of the game
		if( !CheckValidGame() )
			return;

		// End current match
		currentMatch.Deactivate();

		if( *should_log )
		{
			cvarManager->log( "-------------" );
		}

		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		if( !serverWrapper.IsNull() )
		{
			// Get final score
			currentMatch.SetCurrentGameGoals(&*gameWrapper);

			// Update playlist stats
			currentMatch.SetWinOrLoss( stats[matchType]);
		}

		// Reset current game
		currentMatch.MatchEndReset();

		if( *should_log )
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

		if( *should_log )
		{
			cvarManager->log( "-------------" );
			cvarManager->log( "Game destroyed! Rage quit? Stay happy!" );
		}

		// Update playlist stats
		currentMatch.SetWinOrLoss( stats[matchType]);

		// Reset current match
		currentMatch.MatchEndReset();


		if( *should_log )
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
#ifndef DEBUGGING
	std::string playlistName = ssp::playlist::GetName( currentMatch.GetMatchType() );
	auto currentPlaylistStats = stats.find( static_cast<int>( currentMatch.GetMatchType() ) );

	if( *display_stats && playlistName.size() > 0 && currentPlaylistStats != stats.end() )
#else
	if( *display_stats )
#endif
	{
		Vector2 position = {
			*displayMetrics.positionX,
			*displayMetrics.positionY
		};

		// DRAW BOX
		Vector2 canvasSize = canvas.GetSize();
		canvas.SetColor( 0, 0, 0, 75 );
		canvas.SetPosition( position );
		canvas.FillBox( Vector2{ 175, 88 } );

		// DRAW CURRENT PLAYLIST
		canvas.SetColor( TITLE_COLOR );
		canvas.SetPosition( Vector2{ position.X + 10, position.Y + 5 } );
	#ifndef DEBUGGING
		canvas.DrawString( playlistName );
	#else
		canvas.DrawString( "Ranked Solo Standard" );
	#endif

		// DRAW MMR
		canvas.SetColor( LABEL_COLOR );
		canvas.SetPosition( Vector2{ position.X + 24, position.Y + 21 } );
		canvas.DrawString( "MMR:" );

	#ifndef DEBUGGING
		float mmrGain = currentPlaylistStats->second.currentMmr - currentPlaylistStats->second.initialMmr;
	#else
		float mmrGain = 0;
	#endif
		if( mmrGain > 0 )
		{
			canvas.SetColor( GREEN_COLOR );
		}
		else if( mmrGain < 0 )
		{
			canvas.SetColor( RED_COLOR );
		}
		canvas.SetPosition( Vector2{ position.X + 65, position.Y + 21 } );

		std::stringstream mmrGainStream;
		mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
		canvas.DrawString( ( mmrGain > 0 ? "+" : "" ) + mmrGainStream.str() );

		// DRAW WINS
		canvas.SetColor( LABEL_COLOR );
		canvas.SetPosition( Vector2{ position.X + 23, position.Y + 37 } );
		canvas.DrawString( "Wins:" );

		canvas.SetColor( GREEN_COLOR );
		canvas.SetPosition( Vector2{ position.X + 65, position.Y + 37 } );
	#ifndef DEBUGGING
		canvas.DrawString( std::to_string( currentPlaylistStats->second.wins ) );
	#else
		canvas.DrawString( "4" );
	#endif

		// DRAW LOSSES
		canvas.SetColor( LABEL_COLOR );
		canvas.SetPosition( Vector2{ position.X + 10, position.Y + 53 } );
		canvas.DrawString( "Losses:" );

		canvas.SetColor( RED_COLOR );
		canvas.SetPosition( Vector2{ position.X + 65, position.Y + 53 } );
	#ifndef DEBUGGING
		canvas.DrawString( std::to_string( currentPlaylistStats->second.losses ) );
	#else
		canvas.DrawString( "3" );
	#endif

		// DRAW STREAK
		canvas.SetColor( LABEL_COLOR );
		canvas.SetPosition( Vector2{ position.X + 11, position.Y + 69 } );
		canvas.DrawString( "Streak:" );

	#ifndef DEBUGGING
		int streak = currentPlaylistStats->second.streak;
	#else
		int streak = 1;
	#endif
		if( streak > 0 )
		{
			canvas.SetColor( GREEN_COLOR );
		}
		else if(streak < 0)
		{
			canvas.SetColor( RED_COLOR );
		}
		canvas.SetPosition( Vector2{ position.X + 65, position.Y + 69 } );
		canvas.DrawString( ( streak > 0 ? "+" : "" ) + std::to_string( streak ) );
	}
}

void ssp::SessionPlugin::ResetStats()
{
	stats.clear();

	currentMatch.FullReset();
}

bool ssp::SessionPlugin::CheckValidGame()
{
	if( gameWrapper->IsInOnlineGame() && !gameWrapper->IsInReplay() && !gameWrapper->IsInFreeplay() )
	{
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		return serverWrapper.IsOnlineMultiplayer();
	}

	return false;
}

void ssp::SessionPlugin::UpdateCurrentMmr(int retryCount)
{
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()))
	{
		int matchType = static_cast<int>( currentMatch.GetMatchType() );

		gameWrapper->SetTimeout( [matchType, retryCount, this] ( GameWrapper *gameWrapper ) {
			if( retryCount > 0 )
			{
				if( *should_log )
				{
					cvarManager->log( "Trying to get the player MMR (try: " + std::to_string( retryCount ) + ")" );
				}

				MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
				if( mmrWrapper.IsSynced( steamID, matchType ) && !mmrWrapper.IsSyncing( steamID ) )
				{
					float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );
					if( *should_log )
					{
						cvarManager->log( "Received MMR! (" + std::to_string( mmr ) + ")" );
					}

					if( mmr != stats[matchType].currentMmr )
					{
						stats[matchType].currentMmr = mmr;
						if( *should_log )
						{
							float mmrGain = stats[matchType].currentMmr - stats[matchType].initialMmr;
							std::stringstream mmrGainStream;
							mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
							cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+": "" ) + mmrGainStream.str() );
						}
					}
					else
					{
						if( *should_log )
						{
							cvarManager->log( "The MMR was the same. It may not have changed, but retrying just in case!" );
						}
						UpdateCurrentMmr( retryCount - 1 );
						return;
					}
				}
				else
				{
					if( *should_log )
					{
						cvarManager->log( "Couldn't get the MMR because it was syncing.." );
						cvarManager->log( "Try to force it." );
					}

					MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
					float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );
					
					if( stats[matchType].currentMmr == mmr )
					{
						if( *should_log )
						{
							cvarManager->log( "Received MMR is the same as it already was, so it's probably not updated. Retrying.." );
						}
						UpdateCurrentMmr( retryCount - 1 );
						return;
					}
					else
					{
						stats[matchType].currentMmr = mmr;

						if( *should_log )
						{
							float mmrGain = stats[matchType].currentMmr - stats[matchType].initialMmr;
							std::stringstream mmrGainStream;
							mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
							cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+" : "" ) + mmrGainStream.str() );
						}
					}
				}
			} // retryCount > 0
			else if( retryCount == 0 )
			{
				if( *should_log )
				{
					cvarManager->log( "Couldn't get the MMR.. So, we're gonna try to force it." );
				}

				MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
				float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );
				stats[matchType].currentMmr = mmr;

				if( *should_log )
				{
					float mmrGain = stats[matchType].currentMmr - stats[matchType].initialMmr;
					std::stringstream mmrGainStream;
					mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
					cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+" : "" ) + mmrGainStream.str() );
				}
			}
		}, 1.f );
	}
}