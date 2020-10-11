#include "SessionPlugin.h"

#include <sstream>
#include <iomanip>

#include <bakkesmod/plugin/bakkesmodsdk.h>
#include <bakkesmod/wrappers/arraywrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>

#include <Settings.h>

#define CVAR_NAME_DISPLAY "cl_session_plugin_display"
#define CVAR_NAME_SHOULD_LOG "cl_session_plugin_should_log"
#define CVAR_NAME_DISPLAY_X "cl_session_plugin_display_x"
#define CVAR_NAME_DISPLAY_Y "cl_session_plugin_display_y"
#define CVAR_NAME_DISPLAY_TEST "cl_session_plugin_display_test"
#define CVAR_NAME_COLOR_BACKGROUND "cl_session_plugin_background_color"
#define CVAR_NAME_COLOR_TITLE "cl_session_plugin_title_color"
#define CVAR_NAME_COLOR_LABEL "cl_session_plugin_label_color"
#define CVAR_NAME_COLOR_POSITIVE "cl_session_plugin_positive_color"
#define CVAR_NAME_COLOR_NEGATIVE "cl_session_plugin_negative_color"
#define CVAR_NAME_RESET "cl_session_plugin_reset"
#define CVAR_NAME_OUTPUT_MMR "cl_session_plugin_output_mmr"
#define CVAR_NAME_RESET_COLORS "cl_session_plugin_reset_colors"

#define HOOK_COUNTDOWN_BEGINSTATE "Function GameEvent_TA.Countdown.BeginState"
#define HOOK_PLAYER_SCORED "Function TAGame.GameEvent_Soccar_TA.EventPlayerScored"
#define HOOK_ON_WINNER_SET "Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet"
#define HOOK_MATCH_ENDED "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
#define HOOK_EVENT_DESTROYED "Function TAGame.GameEvent_TA.EventDestroyed"
#define HOOK_HANDLE_PENALTY_CHANGED "Function TAGame.GFxHUD_TA.HandlePenaltyChanged"
#define HOOK_ON_MAIN_MENU "Function TAGame.OnlineGame_TA.OnMainMenuOpened"

BAKKESMOD_PLUGIN( ssp::SessionPlugin, "Session plugin (shows session stats)", "1.4", 0 )

ssp::SessionPlugin::SessionPlugin():
	mmrSessionOutput(),
	stats(),
	currentMatch(),
	renderer(),
	steamID(),
	displayStats( std::make_shared<bool>( true ) ),
	displayStatsTest( std::make_shared<bool>( false ) ),
	cvarMMROutputter( std::make_shared<bool>( false ) )
{ }

void ssp::SessionPlugin::onLoad()
{
	// Variable initialization
	ResetStats();

	// log startup to console
	cvarManager->log( std::string(exports.pluginName) + std::string( " version: ") + std::string( exports.pluginVersion) );

	// CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY, "1", "Display session stats", false, true, 0, true, 1, true ).bindTo( displayStats );

	// Renderer CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_X, "420", "X position of the display", false, true, 0, true, 3840, true ).bindTo( renderer.posX );
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_Y, "0", "Y position of the display", false, true, 0, true, 2160, true ).bindTo( renderer.posY );

	// Manage configurable colors
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_TEST, "0", "Show test stats", false, false, 0, false, 1, true ).bindTo( displayStatsTest );
	cvarManager->registerCvar( CVAR_NAME_COLOR_BACKGROUND, "(0, 0, 0, 75)", "Background color", false, false, 0, false, 255, true ).bindTo( renderer.colorBackground );
	cvarManager->registerCvar( CVAR_NAME_COLOR_TITLE, "(255, 255, 255, 127)", "Title color", false, false, 0, false, 255, true ).bindTo( renderer.colorTitle );
	cvarManager->registerCvar( CVAR_NAME_COLOR_LABEL, "(255, 255, 255, 150)", "Label color", false, false, 0, false, 255, true ).bindTo( renderer.colorLabel );
	cvarManager->registerCvar( CVAR_NAME_COLOR_POSITIVE, "(95, 232, 95, 127)", "Positive color", false, false, 0, false, 255, true ).bindTo( renderer.colorPositive );
	cvarManager->registerCvar( CVAR_NAME_COLOR_NEGATIVE, "(232, 95, 95, 127)", "Negative color", false, false, 0, false, 255, true ).bindTo( renderer.colorNegative );

	// MMR ourputter CVar initialization
	cvarManager->registerCvar( CVAR_NAME_OUTPUT_MMR, "0", "Whether the MMR should be saved in a csv file", false, true, 0, true, 1, true ).bindTo( cvarMMROutputter );

	// CVar Hooks
	cvarManager->registerNotifier( CVAR_NAME_RESET, [this] ( std::vector<std::string> params ) {
		ResetStats();
	}, "Start a fresh stats session", PERMISSION_ALL );

	cvarManager->registerNotifier( CVAR_NAME_RESET_COLORS, [this] ( std::vector<std::string> params ) {
		ResetColors();
	}, "Reset to default colors", PERMISSION_ALL );

	// Hook event: Match start
	gameWrapper->HookEvent( HOOK_COUNTDOWN_BEGINSTATE, bind( &SessionPlugin::StartGame, this, std::placeholders::_1 ) );

	// Hook event: Player scored
	gameWrapper->HookEvent( HOOK_PLAYER_SCORED, bind( &SessionPlugin::OnPlayerScored, this, std::placeholders::_1 ) );

	// Hook event: Match end
	gameWrapper->HookEvent( HOOK_ON_WINNER_SET, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );
	gameWrapper->HookEvent( HOOK_MATCH_ENDED, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );

	// Hook event: Match destroyed
	gameWrapper->HookEvent( HOOK_EVENT_DESTROYED, bind( &SessionPlugin::DestroyedGame, this, std::placeholders::_1 ) );

	// This event is called at the very start of the game and as soon as players forfeited or the game ended
	// This is not quite handling the rage quit case, but it will handle the final state of a match even before the player enters the winner circle
	gameWrapper->HookEvent( HOOK_HANDLE_PENALTY_CHANGED, bind( &SessionPlugin::EndGame, this, std::placeholders::_1 ) );

	// Hook event: On main menu
	gameWrapper->HookEventPost( HOOK_ON_MAIN_MENU, bind( &SessionPlugin::InMainMenu, this, std::placeholders::_1 ) );

	// Register drawable
	gameWrapper->RegisterDrawable( std::bind( &SessionPlugin::Render, this, std::placeholders::_1 ) );
}

void ssp::SessionPlugin::onUnload()
{
	ResetStats();

	gameWrapper->UnregisterDrawables();

	gameWrapper->UnhookEvent( HOOK_ON_MAIN_MENU );
	gameWrapper->UnhookEvent( HOOK_HANDLE_PENALTY_CHANGED );
	gameWrapper->UnhookEvent( HOOK_EVENT_DESTROYED );
	gameWrapper->UnhookEvent( HOOK_MATCH_ENDED );
	gameWrapper->UnhookEvent( HOOK_ON_WINNER_SET );
	gameWrapper->UnhookEvent( HOOK_PLAYER_SCORED );
	gameWrapper->UnhookEvent( HOOK_COUNTDOWN_BEGINSTATE );
}

void ssp::SessionPlugin::InMainMenu( std::string eventName )
{
	// Try to update the current MMR if a steam id is known and a playlist session is active
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()) )
	{
		if( steamID.ID > 0 )
		{
			// Update MMR stats
			UpdateCurrentMmr( 3 , std::bind( &SessionPlugin::DetermineMatchResult, this, std::placeholders::_1, std::placeholders::_2 ) );
		}
	}
}

void ssp::SessionPlugin::StartGame( std::string eventName )
{
	DetermineMatchResult( CheckValidGame(), true);

	// If the match has been determined and it's a valid (trackable) game
	if( !currentMatch.IsActive() )
	{
		if( !CheckValidGame() )
			return;

		// We can start a new one
		// Get server wrapper and steam id
		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		steamID.ID = gameWrapper->GetSteamID();

		// Initialize current match data
		currentMatch.OnMatchStart( &*gameWrapper );

		// Get current playlist
		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
		currentMatch.SetMatchType( mmrWrapper.GetCurrentPlaylist() );

		// Get initial MMR if playlist wasn't tracked yet
		int matchType = static_cast<int>( ssp::playlist::ConvertToCasualType( currentMatch.GetMatchType() ) );
		if( stats.find( matchType ) == stats.end() )
		{
			// Initialize session stats whenplaylist hasn't been played yet during this session
			float mmr = mmrWrapper.GetPlayerMMR( steamID, matchType );

			stats[matchType] = ssp::playlist::Stats( mmr );
		}

		mmrSessionOutput.OnNewGame( &*cvarManager , &*gameWrapper, currentMatch.GetMatchType(), stats[matchType].mmr, steamID, currentMatch.GetCurrentTeam() );

		// Set last found diff as this also tells us that a new game was started
		stats[matchType].mmr.lastDiff = 0.f;
	}
}

void ssp::SessionPlugin::OnPlayerScored( std::string eventName )
{
	// Try to update the current MMR if a match and a playlist session are active
	if( currentMatch.IsActive() && stats.find( static_cast<int>( ssp::playlist::ConvertToCasualType( currentMatch.GetMatchType() ) ) ) != stats.end() )
	{
		// Check if we are good to process the state of the player
		if( !CheckValidGame() )
			return;

		gameWrapper->SetTimeout( [this] ( GameWrapper *gameWrapper ) {
			// Get the current score to update the current game data (since a player scored)
			currentMatch.SetCurrentGameGoals( gameWrapper );
		}, 0.2 );
	}
}

void ssp::SessionPlugin::EndGame( std::string eventName )
{
	// Only mark the end of a new game when one is active and the current playlist exists
	/// just in case the player reset his stats just before the end of th game (you never know :) ) 
	int matchType = static_cast<int>( ssp::playlist::ConvertToCasualType( currentMatch.GetMatchType() ));
	if( currentMatch.IsActive() && stats.find( matchType ) != stats.end() )
	{
		// Check if we are good to process the state of the game
		if( !CheckValidGame() )
			return;

		// Reset match
		currentMatch.MatchEndReset();

		ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
		if( !serverWrapper.IsNull() )
		{
			// Get final score
			currentMatch.SetCurrentGameGoals( &*gameWrapper );
		}

		// Update MMR stats and determine win/loss
		UpdateCurrentMmr( 5, std::bind( &SessionPlugin::DetermineMatchResult, this, std::placeholders::_1, std::placeholders::_2 ));

	} // If currentgame is active
}

void ssp::SessionPlugin::DestroyedGame( std::string eventName )
{
	int matchType = static_cast<int>( ssp::playlist::ConvertToCasualType( currentMatch.GetMatchType() ) );
	if( currentMatch.IsActive() && stats.find( matchType ) != stats.end() )
	{ // We know that we are not in a valid game anymore, so we'll have to work with the latest know score
		// End current game
		currentMatch.Deactivate();

		// Reset match
		currentMatch.MatchEndReset();

		// Update MMR stats
		UpdateCurrentMmr( 5, std::bind( &SessionPlugin::DetermineMatchResult, this, std::placeholders::_1, std::placeholders::_2 ) );
	}
}

void ssp::SessionPlugin::Render( CanvasWrapper canvas )
{
	ssp::playlist::Type matchType = currentMatch.GetMatchType();
	auto currentPlaylistStats = stats.find( static_cast<int>( ssp::playlist::ConvertToCasualType( matchType ) ) );

	// Only render if its allowed to, the playlist is known and there are stats to show
	if( *displayStats && matchType != ssp::playlist::Type::PLAYLIST_UNKOWN && currentPlaylistStats != stats.end() )
	{
		// Render current session stats
		renderer.RenderStats( &canvas, currentPlaylistStats->second, currentMatch.GetMatchType() );
	}
	else if( *displayStatsTest || SSP_SETTINGS_DEBUG_RENDERER )
	{
		ssp::playlist::Stats stats;
		stats.SetTestData();
		renderer.RenderStats( &canvas, stats, ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES );
	}
}

void ssp::SessionPlugin::ResetStats()
{
	// Clear all session stats
	stats.clear();

	// Completely reset the current match data
	currentMatch.FullReset();
}

void ssp::SessionPlugin::ResetColors()
{ 
	cvarManager->log( "RESETING COLORS!" );
	renderer.colorBackground->R = 0.f;
	renderer.colorBackground->G = 0.f;
	renderer.colorBackground->B = 0.f;
	renderer.colorBackground->A = 75.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_BACKGROUND ).setValue( *(renderer.colorBackground) );

	renderer.colorTitle->R = 255.f;
	renderer.colorTitle->G = 255.f;
	renderer.colorTitle->B = 255.f;
	renderer.colorTitle->A = 127.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_TITLE ).setValue( *( renderer.colorTitle ) );

	renderer.colorLabel->R = 255.f;
	renderer.colorLabel->G = 255.f;
	renderer.colorLabel->B = 255.f;
	renderer.colorLabel->A = 150.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_LABEL ).setValue( *( renderer.colorLabel ) );

	renderer.colorPositive->R = 95.f;
	renderer.colorPositive->G = 232.f;
	renderer.colorPositive->B = 95.f;
	renderer.colorPositive->A = 127.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_POSITIVE ).setValue( *( renderer.colorPositive ) );

	renderer.colorNegative->R = 232.f;
	renderer.colorNegative->G = 95.f;
	renderer.colorNegative->B = 95.f;
	renderer.colorNegative->A = 127.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_NEGATIVE ).setValue( *( renderer.colorNegative ) );
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

void ssp::SessionPlugin::UpdateCurrentMmr(int retryCount, std::function<void(bool, bool)> onSuccess )
{
	ssp::playlist::Type playlistType = currentMatch.GetMatchType();
	// Only update MMR if we know what playlist type to update
	if( ssp::playlist::IsKnown(currentMatch.GetMatchType()))
	{
		// Only try to receive MMR if we have tries left
		if( retryCount >= 0 )
		{
			// Convert match type to int (for easy map usage)
			int matchType = static_cast<int>( playlistType );
			int convertedMatchType = static_cast<int>( ssp::playlist::ConvertToCasualType( playlistType ) );

			// Check if the MMR is currently synced
			if( !stats[convertedMatchType].mmr.RequestMmrUpdate( &*gameWrapper, steamID, &playlistType, false ) )
			{
				if( !stats[convertedMatchType].mmr.RequestMmrUpdate( &*gameWrapper, steamID, &playlistType, true ) )
				{
					gameWrapper->SetTimeout( [this, retryCount, onSuccess] ( GameWrapper *gameWrapper ) {
						this->UpdateCurrentMmr( retryCount - 1, onSuccess );
					}, 1.f );
					return;
				}
			}

			if( onSuccess )
			{
				if( *cvarMMROutputter == true )
				{
					mmrSessionOutput.OnEndGame( &*cvarManager, playlistType, stats[convertedMatchType].mmr );
				}
				onSuccess(false, false);
			}
		}
	}
}

void ssp::SessionPlugin::DetermineMatchResult( bool allowForce, bool updateMmr )
{
	// If the match result still has to be determined
	if( currentMatch.CanBeDetermined() )
	{
		if( updateMmr )
		{
			UpdateCurrentMmr( 0 );
		}

		// Determine match result based on mmr of the last known playlist
		int matchType = static_cast<int>( ssp::playlist::ConvertToCasualType( currentMatch.GetMatchType() ) );
		if( ssp::playlist::IsKnown( currentMatch.GetMatchType() ) && stats.find( matchType ) != stats.end() )
		{
			if( !currentMatch.SetWinOrLoss( this, stats[matchType], true ) )
			{
				// If its a valid match and the determination still didn't work,
				// force the issue and determine the result based on the goals scored
				if( allowForce )
				{
					currentMatch.SetWinOrLoss( this, stats[matchType], false );
				}
			}
		}
	}
}
