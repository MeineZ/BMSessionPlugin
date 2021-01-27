#pragma once

#include <map>

#include <bakkesmod/wrappers/GameWrapper.h>
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/wrapperstructs.h>

#include <Settings.h>
#include <Playlist.h>
#include <MMR.h>

#define MMR_ENTRY std::pair<UniqueIDWrapper, ssp::MMR>

namespace ssp
{  // SessionPlugin

	class SessionPlugin;

	class MMRSessionOutput
	{
	public:
		MMRSessionOutput();
		~MMRSessionOutput();

		void Initialize( SessionPlugin *plugin );

		std::shared_ptr<bool> cvarMMROutputter; // Setting if we should output mmr	
		std::shared_ptr<bool> outputOtherGain; // Setting if we should output the other game members gain as well	

	private:
		void WaitForAllPlayers_BeginState( std::string eventName );
		void CountDown_BeginState( std::string eventName );
		void OnlineGame_OnMainMenu( std::string eventName );
		void GameEventSoccar_EventMatchWinnerSet( std::string eventName );
		void MMRWrapper_Notifier( UniqueIDWrapper uniqueID );
		void TestEvent( std::string eventName );

		bool HaveAllMMRBeenSetCorrectly();
		void WriteToFile( bool hardForce = false, bool forceIfNotAllCorrect = false );
		void Reset();

		SessionPlugin *plugin;
		std::unique_ptr<MMRNotifierToken> MMRNotifierToken;
		std::vector<MMR_ENTRY> mmrPlayers;
		UniqueIDWrapper currentPlayerId;
		ssp::playlist::Type currentPlaylist;
	};
}

