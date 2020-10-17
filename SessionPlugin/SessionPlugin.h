#pragma once

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <string>

#include <bakkesmod/plugin/bakkesmodplugin.h>
#include <bakkesmod/wrappers/canvaswrapper.h>

#include <MMRSessionOutput.h>
#include <Playlist.h>
#include <Match.h>
#include <Renderer.h>

namespace ssp // SessionPlugin
{
	class SessionPlugin: public BakkesMod::Plugin::BakkesModPlugin
	{
	private:
		ssp::MMRSessionOutput mmrSessionOutput; // Outputs stats (also from team members to csv)
		std::map<int, ssp::playlist::Stats> stats; // All stats per playlist
		ssp::Match currentMatch; // Contains info about the current match
		ssp::Renderer renderer; // Renderer that can render different objects
		SteamID steamID; // Steam ID info
		bool isInMatch;

		std::shared_ptr<bool> displayStats; // Setting if we should display stats
		std::shared_ptr<bool> displayStatsInMatch; // Setting if we should display stats during matches
		std::shared_ptr<bool> displayStatsTest; // Setting if we should display stats test
		std::shared_ptr<bool> cvarMMROutputter; // Setting if we should output mmr		

	public:
		SessionPlugin();

		// Required plugin hooks
		virtual void onLoad();
		virtual void onUnload();

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

		// Colors reset
		void ResetColors();

		// Check if game is valid to track
		bool CheckValidGame();

		// Updates the current Mmr of the player
		void UpdateCurrentMmr( int retryCount, std::function<void( bool, bool )> onSuccess = nullptr );

		// Try to determine the match result (if possible and allowed)
		void DetermineMatchResult( bool allowForce, bool updateMmr = true );
	};
}

