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

BAKKESMOD_PLUGIN( SessionPlugin, "Session plugin (shows session stats)", "1.0", 0 )

#ifdef DEBUGGING
void SessionPlugin::testHook( std::string eventName )
{
	cvarManager->log( "-----------------------" );
	cvarManager->log( eventName );
	cvarManager->log( "-----------------------" );
}
#endif

void SessionPlugin::onLoad()
{
	// Variable initialization
	ResetStats();

#ifdef DEBUGGING
	playlists[/* PLAYLIST_DOUBLES */ 2] = "Doubles";
	playlists[/* PLAYLIST_STANDARD */ 3] = "Standard";
#endif
	playlists[/* PLAYLIST_RANKEDDUEL */ 10] = "Ranked Solo Duel";
	playlists[/* PLAYLIST_RANKEDDOUBLES */ 11] = "Ranked Doubles";
	playlists[/* PLAYLIST_RANKEDSOLOSTANDARD */ 12] = "Ranked Solo Standard";
	playlists[/* PLAYLIST_RANKEDSTANDARD */ 13] = "Ranked Standard";

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


void SessionPlugin::onUnload()
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

void SessionPlugin::InMainMenu( std::string eventName )
{
	if( currentGame.type != -1 )
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

void SessionPlugin::OnPlayerScored( std::string eventName )
{
	if( currentGame.isActive && stats.find( currentGame.type ) != stats.end() )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		if( *should_log )
		{
			cvarManager->log( "-------------" );
		}

		gameWrapper->SetTimeout( [this] ( GameWrapper *gameWrapper ) {
			SetCurrentGameGoals();

			if( *should_log )
			{
				cvarManager->log( "A player scored! Current score: " + std::to_string( currentGame.team1Goals ) + " - " + std::to_string( currentGame.team2Goals ) );
				cvarManager->log( "Your team is: " + std::to_string( currentGame.teamNumber ) );
				cvarManager->log( "-------------" );
			}
		}, 0.25 );
	}
}

void SessionPlugin::StartGame( std::string eventName )
{
	if( *should_log )
	{
		cvarManager->log( "-------------" );
	}

	if( !currentGame.isActive )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();

		// Current game is active
		currentGame.isActive = true;

		steamID.ID = gameWrapper->GetSteamID();

		// Get car and team number
		CarWrapper car = gameWrapper->GetLocalCar();
		currentGame.teamNumber = -1;
		if( !car.IsNull() )
		{
			currentGame.teamNumber = car.GetTeamNum2();
		}

		// Get current playlist
		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();

		currentGame.type = mmrWrapper.GetCurrentPlaylist();

		// Get initial MMR if playlist wasn't tracked yet
		if( stats.find( currentGame.type ) == stats.end() )
		{
			float mmr = mmrWrapper.GetPlayerMMR( steamID, currentGame.type );

			stats[currentGame.type] = PlaylistSessionStats{ mmr, mmr, 0, 0, 0 };
		}

		if( *should_log )
		{
			cvarManager->log( "Game started!" );
			cvarManager->log( "Received SteamID: " + std::to_string( steamID.ID ) );
			cvarManager->log( "Player is on team " + std::to_string( currentGame.teamNumber ) );
			cvarManager->log( "MMR: " + std::to_string( stats[currentGame.type].currentMmr ) );
			cvarManager->log( "Current playlist: " + std::to_string( currentGame.type ) );
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

void SessionPlugin::EndGame( std::string eventName )
{
	if( currentGame.isActive && stats.find( currentGame.type ) != stats.end() )
	{
		// Check if we are good to process the state of the game
		if( !CheckValidGame() )
			return;

		// End current session
		currentGame.isActive = false;

		if( *should_log )
		{
			cvarManager->log( "-------------" );
		}

		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		if( !serverWrapper.IsNull() )
		{
			// Get final score
			SetCurrentGameGoals();

			// Update playlist stats
			SetWinOrLoss();
		}

		// Reset current game
		currentGame.MatchEndReset();

		if( *should_log )
		{
			cvarManager->log( "Game over!" );
			cvarManager->log( "Wins: " + std::to_string(stats[currentGame.type].wins) );
			cvarManager->log( "Losses: " + std::to_string( stats[currentGame.type].losses ) );
			cvarManager->log( "Streak: " + std::to_string( stats[currentGame.type].streak ) );
			cvarManager->log( "-------------" );
		}

		// Update MMR stats
		UpdateCurrentMmr( 10 );
	} // If currentgame is active
}

void SessionPlugin::DestroyedGame( std::string eventName )
{
	if( currentGame.isActive && stats.find( currentGame.type ) != stats.end() )
	{ // We know that we are not in a valid game anymore, so we'll have to work with the latest know score
		// End current session
		currentGame.isActive = false;

		if( *should_log )
		{
			cvarManager->log( "-------------" );
			cvarManager->log( "Game destroyed! Rage quit? Stay happy!" );
		}

		// Update playlist stats
		SetWinOrLoss();

		// Reset current game
		currentGame.MatchEndReset();


		if( *should_log )
		{
			cvarManager->log( "Wins: " + std::to_string( stats[currentGame.type].wins ) );
			cvarManager->log( "Losses: " + std::to_string( stats[currentGame.type].losses ) );
			cvarManager->log( "Streak: " + std::to_string( stats[currentGame.type].streak ) );
			cvarManager->log( "-------------" );
		}

		// Update MMR stats
		UpdateCurrentMmr(10);
	}
}

void SessionPlugin::Render( CanvasWrapper canvas )
{
#ifndef DEBUGGING
	auto itPlaylist = playlists.find( currentGame.type );
	auto currentPlaylistStats = stats.find( currentGame.type );

	if( *display_stats && itPlaylist != playlists.end() && currentPlaylistStats != stats.end() )
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
		canvas.DrawString( itPlaylist->second ); 
	#else
		canvas.DrawString( "Ranked Solo Standard" );
	#endif

		// DRAW MMR
		canvas.SetColor( LABEL_COLOR );
		canvas.SetPosition( Vector2{ position.X + 24, position.Y + 21 } );
		canvas.DrawString( "MMR:" );

	#ifndef DEBUGGING
		float mmrGain = stats[currentGame.type].currentMmr - stats[currentGame.type].initialMmr;
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

void SessionPlugin::ResetStats()
{
	stats.clear();

	currentGame = { 0, 0, -1, -1, false };
}

bool SessionPlugin::CheckValidGame()
{
	if( gameWrapper->IsInOnlineGame() && !gameWrapper->IsInReplay() && !gameWrapper->IsInFreeplay() )
	{
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		return serverWrapper.IsOnlineMultiplayer();
	}

	return false;
}

void SessionPlugin::SetCurrentGameGoals()
{
	// Something must have gone wrong if the team number wasn't found
	// We just retry again (important to note is that the current team number is very important to figure out who won)
	if( currentGame.teamNumber == -1 )
	{
		CarWrapper car = gameWrapper->GetLocalCar();
		if( !car.IsNull() )
		{
			currentGame.teamNumber = car.GetTeamNum2();
		}
	}

	ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
	if( !serverWrapper.IsNull() )
	{
		// Get teams
		ArrayWrapper<TeamWrapper> teams = serverWrapper.GetTeams();
		if( teams.Count() == 2 )
		{
			// Get the score of both teams
			currentGame.team1Goals = teams.Get( 0 ).GetScore();
			currentGame.team2Goals = teams.Get( 1 ).GetScore();
		}
	}
}

void SessionPlugin::SetWinOrLoss()
{
	if( ( currentGame.team1Goals > currentGame.team2Goals && currentGame.teamNumber == 0 ) || ( currentGame.team2Goals > currentGame.team1Goals && currentGame.teamNumber == 1 ) )
	{ // log win
		stats[currentGame.type].wins++;
		if( stats[currentGame.type].streak < 0 )
		{
			stats[currentGame.type].streak = 1;
		}
		else
		{
			stats[currentGame.type].streak++;
		}
	}
	else
	{ // log loss
		stats[currentGame.type].losses++;
		if( stats[currentGame.type].streak > 0 )
		{
			stats[currentGame.type].streak = -1;
		}
		else
		{
			stats[currentGame.type].streak--;
		}
	}
}

void SessionPlugin::UpdateCurrentMmr(int retryCount)
{
	if( currentGame.type > 0 )
	{
		gameWrapper->SetTimeout( [retryCount, this] ( GameWrapper *gameWrapper ) {
			if( retryCount > 0 )
			{
				if( *should_log )
				{
					cvarManager->log( "Trying to get the player MMR (try: " + std::to_string( retryCount ) + ")" );
				}

				MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
				if( mmrWrapper.IsSynced( steamID, currentGame.type ) && !mmrWrapper.IsSyncing( steamID ) )
				{
					float mmr = mmrWrapper.GetPlayerMMR( steamID, currentGame.type );
					if( *should_log )
					{
						cvarManager->log( "Received MMR! (" + std::to_string( mmr ) + ")" );
					}

					if( mmr != stats[currentGame.type].currentMmr )
					{
						stats[currentGame.type].currentMmr = mmr;
						if( *should_log )
						{
							float mmrGain = stats[currentGame.type].currentMmr - stats[currentGame.type].initialMmr;
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
					float mmr = mmrWrapper.GetPlayerMMR( steamID, currentGame.type );
					
					if( stats[currentGame.type].currentMmr == mmr )
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
						stats[currentGame.type].currentMmr = mmr;

						if( *should_log )
						{
							float mmrGain = stats[currentGame.type].currentMmr - stats[currentGame.type].initialMmr;
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
				float mmr = mmrWrapper.GetPlayerMMR( steamID, currentGame.type );
				stats[currentGame.type].currentMmr = mmr;

				if( *should_log )
				{
					float mmrGain = stats[currentGame.type].currentMmr - stats[currentGame.type].initialMmr;
					std::stringstream mmrGainStream;
					mmrGainStream << std::fixed << std::setprecision( 2 ) << mmrGain;
					cvarManager->log( "Updated session MMR: " + std::string( mmrGain > 0 ? "+" : "" ) + mmrGainStream.str() );
				}
			}
		}, 1.f );
	}
}