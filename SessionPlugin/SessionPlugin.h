#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

#include <bakkesmod/plugin/bakkesmodplugin.h>
#include <bakkesmod/wrappers/canvaswrapper.h>

//#define DEBUGGING

typedef struct
{
	float initialMmr, currentMmr;
	int wins, losses, streak;
} PlaylistSessionStats;

typedef struct
{
	int team1Goals, team2Goals, type, teamNumber;
	bool isActive;

	void MatchEndReset()
	{
		team1Goals = 0;
		team2Goals = 0;
		teamNumber = -1;
		isActive = false;
	}
} Game;

typedef struct
{
	std::shared_ptr<int> positionX = std::make_shared<int>( 420 );
	std::shared_ptr<int> positionY = std::make_shared<int>( 0 );
} DisplayMetrics;

class SessionPlugin: public BakkesMod::Plugin::BakkesModPlugin
{ 
private:
	std::map<int, std::string> playlists; // Contains all supported playlists and their names
	std::map<int, PlaylistSessionStats> stats; // All stats per playlist
	Game currentGame; // Contains info about the current game
	SteamID steamID; // Steam ID info

	std::shared_ptr<bool> display_stats = std::make_shared<bool>( true ); // Setting if we should display stats
	std::shared_ptr<bool> should_log = std::make_shared<bool>( false ); // Setting if we should log info to the console

	DisplayMetrics displayMetrics; // Metrics of the stats display

	// Events
	void OnPlayerScored( std::string eventName );
	void StartGame( std::string eventName );
	void EndGame( std::string eventName );
	void DestroyedGame( std::string eventName );
	void InMainMenu( std::string eventName );

	// Render hook
	void Render( CanvasWrapper canvas );

	// Stats reset
	void ResetStats();

	// Check if game is valid to track
	bool CheckValidGame();

	// Set match score (ASSUMES WE ARE IN A VALID GAME! GUARD THIS FUNCTION WITH AN SessionPlugin::CheckValidGame()
	void SetCurrentGameGoals();

	// Updates the playlist stats according to the goals scored
	void SetWinOrLoss();

	// Updates the current Mmr of the player
	void UpdateCurrentMmr( int retryCount );

#ifdef DEBUGGING
	void testHook( std::string eventName );
#endif
	
public:
	// Required plugin hooks
	virtual void onLoad();
	virtual void onUnload();

};

