#include "Playlist.h"

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>

namespace ssp // SessionPlugin
{
	namespace playlist
	{
		Type FromInt( int type )
		{
			switch( type )
			{
				case int( Type::PLAYLIST_DUEL ):					return Type::PLAYLIST_DUEL;
				case int( Type::PLAYLIST_DOUBLES ):					return Type::PLAYLIST_DOUBLES;
				case int( Type::PLAYLIST_STANDARD ) :				return Type::PLAYLIST_STANDARD;
				case int( Type::PLAYLIST_CHAOS ) :					return Type::PLAYLIST_CHAOS;
				case int( Type::PLAYLIST_RANKEDDUEL ) :				return Type::PLAYLIST_RANKEDDUEL;
				case int( Type::PLAYLIST_RANKEDDOUBLES ) :			return Type::PLAYLIST_RANKEDDOUBLES;
				case int( Type::PLAYLIST_RANKEDSOLOSTANDARD ) :		return Type::PLAYLIST_RANKEDSOLOSTANDARD;
				case int( Type::PLAYLIST_RANKEDSTANDARD ) :			return Type::PLAYLIST_RANKEDSTANDARD;
				case int( Type::PLAYLIST_RANKEDHOOPS ) :			return Type::PLAYLIST_RANKEDHOOPS;
				case int( Type::PLAYLIST_RANKEDRUMBLE ) :			return Type::PLAYLIST_RANKEDRUMBLE;
				case int( Type::PLAYLIST_RANKEDDROPSHOT ) :			return Type::PLAYLIST_RANKEDDROPSHOT;
				case int( Type::PLAYLIST_RANKEDSNOWDAY ) :			return Type::PLAYLIST_RANKEDSNOWDAY;
				default:											return Type::PLAYLIST_UNKNOWN;
			}
		}

		std::string GetName(Type type)
		{
			switch( type )
			{
				case Type::PLAYLIST_DUEL:
				case Type::PLAYLIST_DOUBLES:
				case Type::PLAYLIST_STANDARD:
				case Type::PLAYLIST_CHAOS:					return "Casual";
				case Type::PLAYLIST_RANKEDDUEL:				return "Ranked Duel";
				case Type::PLAYLIST_RANKEDDOUBLES:			return "Ranked Doubles";
				case Type::PLAYLIST_RANKEDSOLOSTANDARD:		return "Ranked Solo Standard";
				case Type::PLAYLIST_RANKEDSTANDARD:			return "Ranked Standard";
				case Type::PLAYLIST_RANKEDHOOPS:			return "Ranked Hoops";
				case Type::PLAYLIST_RANKEDRUMBLE:			return "Ranked Rumble";
				case Type::PLAYLIST_RANKEDDROPSHOT:			return "Ranked Dropshot";
				case Type::PLAYLIST_RANKEDSNOWDAY:			return "Ranked Snowday";
				case Type::PLAYLIST_UNKNOWN:
				default:									return "";
			}
		}

		bool IsCasualPlaylist( Type type )
		{
			switch( type )
			{
				case Type::PLAYLIST_DUEL:
				case Type::PLAYLIST_DOUBLES:
				case Type::PLAYLIST_STANDARD:
				case Type::PLAYLIST_CHAOS:		return true;
				default:						return false;
			}
		}

		Stats::Stats():
			Loggable(),
			mmr( 0.0f ),
			wins( 0 ),
			losses( 0 ),
			streak( 0 )
		{ }

		Stats::Stats( float initialMmr ):
			Loggable(),
			mmr(initialMmr),
			wins(0),
			losses(0),
			streak(0)
		{
		}

		void Stats::SetTestData()
		{
			wins = 5;
			losses = 2;
			streak = 3;

			mmr = ssp::MMR( 1743.00f );
			mmr.current = 1772.43f;
			mmr.lastDiffDisplay = 9.64f;
			mmr.streakMmrGain = 28.92f;

		}

		void Stats::Log( CVarManagerWrapper *cvarManager )
		{
			mmr.Log( cvarManager );
			cvarManager->log( "Wins: " + std::to_string(wins));
			cvarManager->log( "Losses: " + std::to_string( losses ) );
			cvarManager->log( "Streak: " + std::to_string( streak ) );
		}
	}
}

