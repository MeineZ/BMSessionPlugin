#pragma once

#include <map>

#include <bakkesmod/wrappers/GameWrapper.h>
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/wrapperstructs.h>

#include <Settings.h>
#include <Playlist.h>
#include <MMR.h>

namespace ssp
{  // SessionPlugin

	class MMRSessionOutput
	{ 
	public:
		MMRSessionOutput();
		~MMRSessionOutput();

		void OnNewGame( CVarManagerWrapper *cvarManager, GameWrapper * gameWrapper, ssp::playlist::Type playlist, ssp::MMR & currentPlayerMMR, SteamID & currentPlayerSteamID, int currentTeam );

		void OnEndGame( CVarManagerWrapper *cvarManager, ssp::playlist::Type playlist, ssp::MMR &currentPlayerMMR );

	private:
		std::map<ssp::playlist::Type, std::vector<float>> allMMR;
		bool didDetermine;
		bool stopMMRfetch;

		void RequestMMR( GameWrapper *gameWrapper, SteamID &steamId, const ssp::playlist::Type matchType, int retryCount, std::function<void( float )> onSuccess );

		inline int GetPlaylistGameSize( ssp::playlist::Type playlist);
		inline std::string GetPlaylistFileName( ssp::playlist::Type playlist );
	};

	inline int MMRSessionOutput::GetPlaylistGameSize( ssp::playlist::Type playlist )
	{
		switch( playlist )
		{
		#ifdef SSP_SETTINGS_DEBUG_MMR_OUTPUT
			case ssp::playlist::Type::PLAYLIST_DUEL:
		#endif
			case ssp::playlist::Type::PLAYLIST_RANKEDDUEL:
				return 2;
			case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES:
				return 4;
			case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD:
			case ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD:
				return 6;
			default:
				return -1;
		}
	}
	inline std::string MMRSessionOutput::GetPlaylistFileName( ssp::playlist::Type playlist )
	{
		switch( playlist )
		{
		#ifdef SSP_SETTINGS_DEBUG_MMR_OUTPUT
			case ssp::playlist::Type::PLAYLIST_DUEL:
		#endif
				return "CasualSolo";
			case ssp::playlist::Type::PLAYLIST_RANKEDDUEL:
				return "Solo";
			case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES:
				return "Doubles";
			case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD:
				return "Standard";
			case ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD:
				return "SoloStandard";
			default:
				return "Unknown";
		}
	}
}

