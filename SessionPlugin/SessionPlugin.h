#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

#include <bakkesmod/plugin/bakkesmodplugin.h>
#include <bakkesmod/wrappers/canvaswrapper.h>

#include <Playlist.h>
#include <Match.h>

//#define DEBUGGING

typedef struct
{
	std::shared_ptr<int> positionX = std::make_shared<int>( 420 );
	std::shared_ptr<int> positionY = std::make_shared<int>( 0 );
} DisplayMetrics;

namespace ssp // SessionPlugin
{
	class SessionPlugin: public BakkesMod::Plugin::BakkesModPlugin
	{
	private:
		std::map<int, ssp::playlist::Stats> stats; // All stats per playlist
		ssp::Match currentMatch; // Contains info about the current match
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
}

