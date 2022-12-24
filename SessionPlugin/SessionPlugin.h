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
		ssp::Renderer renderer; // Renderer that can render different objects
		ssp::Match currentMatch; // Contains info about the current match
		UniqueIDWrapper currentPlayerID; // Unique ID info
		std::map<int, ssp::playlist::Stats> stats; // All stats per playlist
		std::unique_ptr<MMRNotifierToken> mmrNotifierToken;

		std::shared_ptr<bool> displayStats; // Setting if we should display stats
		std::shared_ptr<bool> displayStatsInMatch; // Setting if we should display stats during matches
		std::shared_ptr<bool> displayStatsTest; // Setting if we should display stats test

	public:
		SessionPlugin();

		// Required plugin hooks
		virtual void onLoad();
		virtual void onUnload();

		// Reset functions
		void ResetRendererScale();
		void ResetStats();
		void ResetColors();

		// Render hook
		void Render( CanvasWrapper canvas );

		// Check if game is valid to track
		bool CheckValidGame();

		// Events
		void WaitForAllPlayers_BeginState( std::string eventName );
		void CountDown_BeginState( std::string eventName );
		void GameEvent_PlayerScored( std::string eventName );
		void GameEvent_MatchWinnerSet( std::string eventName );
		void TAGame_MainMenuAdded( std::string eventName );
		void MMRWrapper_Notifier( UniqueIDWrapper uniqueID );

		// Helpers
		ssp::playlist::Stats *GetPlaylistStats( const ssp::playlist::Type playlistType );
		inline UniqueIDWrapper &GetUniqueID();
	};
}
inline UniqueIDWrapper &ssp::SessionPlugin::GetUniqueID()
{
	return currentPlayerID;
}

