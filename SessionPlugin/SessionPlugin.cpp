#include "SessionPlugin.h"

#include <sstream>
#include <iomanip>

#include <bakkesmod/plugin/bakkesmodsdk.h>
#include <bakkesmod/wrappers/arraywrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>


#include <fstream>

#include <Settings.h>

#define CVAR_NAME_DISPLAY "cl_session_plugin_display"
#define CVAR_NAME_DISPLAY_IN_MATCH "cl_session_plugin_display_in_matches"
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
#define CVAR_NAME_OUTPUT_MMR_OPPONENT "cl_session_plugin_output_mmr_include_other"
#define CVAR_NAME_RESET_COLORS "cl_session_plugin_reset_colors"

#define HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE "Function OnlineGameJoinGame_X.WaitForAllPlayers.BeginState"
#define HOOK_COUNTDOWN_BEGINSTATE "Function GameEvent_TA.Countdown.BeginState"
#define HOOK_PLAYER_SCORED "Function TAGame.GameEvent_Soccar_TA.EventPlayerScored"
#define HOOK_ON_WINNER_SET "Function TAGame.GameEvent_Soccar_TA.EventMatchWinnerSet"

BAKKESMOD_PLUGIN( ssp::SessionPlugin, "Session plugin (shows session stats)", "1.11", 0 )

ssp::SessionPlugin::SessionPlugin():
	mmrSessionOutput(),
	renderer(),
	currentMatch(),
	currentPlayerID(),
	stats(),
	displayStats( std::make_shared<bool>( true ) ),
	displayStatsInMatch( std::make_shared<bool>( true ) ),
	displayStatsTest( std::make_shared<bool>( false ) )
{ }

void ssp::SessionPlugin::onLoad()
{
	// Variable initialization
	ResetStats();

	// log startup to console
	cvarManager->log( std::string( exports.pluginName ) + std::string( " version: " ) + std::string( exports.pluginVersion ) );

	// Display CVar initialization
	cvarManager->registerCvar( CVAR_NAME_DISPLAY, "1", "Display session stats", false, true, 0, true, 1, true ).bindTo( displayStats );
	cvarManager->registerCvar( CVAR_NAME_DISPLAY_IN_MATCH, "1", "Show stats during matches", false, true, 0, true, 1, true ).bindTo( displayStatsInMatch );

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
	cvarManager->registerCvar( CVAR_NAME_OUTPUT_MMR, "0", "Whether the MMR should be saved in a csv file", false, true, 0, true, 1, true ).bindTo( mmrSessionOutput.cvarMMROutputter );
	cvarManager->registerCvar( CVAR_NAME_OUTPUT_MMR_OPPONENT, "0", "Wheter the MMR gain of all other game members should be included in the output", false, true, 0, true, 1, true ).bindTo( mmrSessionOutput.outputOtherGain );

	// CVar Hooks
	cvarManager->registerNotifier( CVAR_NAME_RESET, [this] ( std::vector<std::string> params ) {
		ResetStats();
	}, "Start a fresh stats session", PERMISSION_ALL );

	cvarManager->registerNotifier( CVAR_NAME_RESET_COLORS, [this] ( std::vector<std::string> params ) {
		ResetColors();
	}, "Reset to default colors", PERMISSION_ALL );

	// Bind event hooks
	gameWrapper->HookEventPost( HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE, std::bind( &SessionPlugin::WaitForAllPlayers_BeginState, this, std::placeholders::_1 ) );
	gameWrapper->HookEventPost( HOOK_COUNTDOWN_BEGINSTATE, std::bind( &SessionPlugin::CountDown_BeginState, this, std::placeholders::_1 ) );
	gameWrapper->HookEventPost( HOOK_PLAYER_SCORED, std::bind( &SessionPlugin::GameEvent_PlayerScored, this, std::placeholders::_1 ) );
	gameWrapper->HookEventPost( HOOK_ON_WINNER_SET, std::bind( &SessionPlugin::GameEvent_MatchWinnerSet, this, std::placeholders::_1 ) );

	// Register to MMR notifier
	mmrNotifierToken = gameWrapper->GetMMRWrapper().RegisterMMRNotifier( std::bind( &ssp::SessionPlugin::MMRWrapper_Notifier, this, std::placeholders::_1 ) );

	// Register drawable
	gameWrapper->RegisterDrawable( std::bind( &SessionPlugin::Render, this, std::placeholders::_1 ) );

	// Set default properties
	mmrSessionOutput.Initialize( this );
}

void ssp::SessionPlugin::onUnload()
{
	ResetStats();

	gameWrapper->UnregisterDrawables();

	gameWrapper->UnhookEventPost( HOOK_ONLINEGAMEJOINGAME_WAITFORALLPLAYERS_BEGINSTATE );
	gameWrapper->UnhookEventPost( HOOK_COUNTDOWN_BEGINSTATE );
	gameWrapper->UnhookEventPost( HOOK_PLAYER_SCORED );
	gameWrapper->UnhookEventPost( HOOK_ON_WINNER_SET );
}

void ssp::SessionPlugin::ResetStats()
{
	// Clear all session stats
	stats.clear();

	// Completely reset the current match data
	currentMatch.Reset();
}

void ssp::SessionPlugin::ResetColors()
{
	cvarManager->log( "RESETING COLORS!" );
	renderer.colorBackground->R = 0.f;
	renderer.colorBackground->G = 0.f;
	renderer.colorBackground->B = 0.f;
	renderer.colorBackground->A = 75.f;
	cvarManager->getCvar( CVAR_NAME_COLOR_BACKGROUND ).setValue( *( renderer.colorBackground ) );

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

void ssp::SessionPlugin::Render( CanvasWrapper canvas )
{
	auto currentPlaylistStats = GetPlaylistStats( currentMatch.GetMatchType() );

	// Only render if its allowed to, the playlist is known and there are stats to show
	if( *displayStats && ssp::playlist::IsKnown(currentMatch.GetMatchType()) && currentPlaylistStats != nullptr )
	{
		if( !*displayStatsInMatch && currentMatch.GetMatchState() == ssp::match::State::ONGOING )
			return;

		// Render current session stats
		renderer.RenderStats( &canvas, *currentPlaylistStats, currentMatch.GetMatchType() );
	}
	else if( *displayStatsTest || SSP_SETTINGS_DEBUG_RENDERER )
	{
		ssp::playlist::Stats stats;
		stats.SetTestData();
		renderer.RenderStats( &canvas, stats, ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES );
	}
}

void ssp::SessionPlugin::WaitForAllPlayers_BeginState( std::string eventName )
{
	ssp::playlist::Stats *currentStats = GetPlaylistStats( currentMatch.GetMatchType() );

	if( currentStats == nullptr || currentMatch.GetMatchState() == ssp::match::State::IDLE )
		return;

	currentStats->Update( this, &currentMatch, true );
}

void ssp::SessionPlugin::CountDown_BeginState( std::string eventName )
{
	if( !CheckValidGame() )
		return;

	// First time here
	if( currentMatch.GetMatchState() != ssp::match::State::ONGOING )
	{
		// Trigger match start
		currentMatch.OnMatchStart(&*gameWrapper);
	}

	// Retrieve player ID if possible
	if( currentPlayerID.GetUID() == ID_INVALID_VALUE )
	{
		currentPlayerID = gameWrapper->GetUniqueID();
	}
}

void ssp::SessionPlugin::GameEvent_PlayerScored( std::string eventName )
{
	if( !CheckValidGame() )
		return;

	currentMatch.OnGoalScored(&*gameWrapper);
}

void ssp::SessionPlugin::GameEvent_MatchWinnerSet( std::string eventName )
{ 
	currentMatch.OnMatchEnded( &*gameWrapper );
}

void ssp::SessionPlugin::MMRWrapper_Notifier( UniqueIDWrapper uniqueID )
{ 
	if( currentPlayerID.GetUID() == ID_INVALID_VALUE || uniqueID.GetUID() != currentPlayerID.GetUID() )
		return;

	ssp::playlist::Stats *currentStats = GetPlaylistStats( currentMatch.GetMatchType() );

	if( currentStats == nullptr )
		return;

	currentStats->Update( this, &currentMatch, false );
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

ssp::playlist::Stats * ssp::SessionPlugin::GetPlaylistStats( const ssp::playlist::Type playlistType )
{
	if( !ssp::playlist::IsKnown( playlistType ) )
		return nullptr;

	int matchType = static_cast<int>( ssp::playlist::ConvertToCasualType( playlistType ) );
	if( stats.find( matchType ) == stats.end() )
	{
		// Initialize session stats whenplaylist hasn't been played yet during this session
		float mmr = gameWrapper->GetMMRWrapper().GetPlayerMMR( currentPlayerID, static_cast<int>( playlistType ) );
		stats[matchType] = ssp::playlist::Stats( mmr );
	}

	return &stats[matchType];
}


