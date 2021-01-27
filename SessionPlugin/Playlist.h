#pragma once

#include <string>

#include <MMR.h>
#include <Loggable.h>

class CVarManagerWrapper;

namespace ssp // Session plugin
{
	namespace playlist
	{
		// Contains all stats that a playlist session can have.
		class Stats : public Loggable
		{
		public:
			ssp::MMR mmr;
			int wins;
			int losses;
			int streak;

			Stats( );
			Stats( float initialMmr );

			// Sets data of the stats. Only use for testing purposes as this may not work as intended!
			void SetTestData( );

			virtual void Log( CVarManagerWrapper *cvarManager );
		};

		// All supported types (types that are consider worth tracking).
		enum class Type
		{
			PLAYLIST_UNKNOWN			= -1,
			PLAYLIST_DUEL				=  1,
			PLAYLIST_DOUBLES			=  2,
			PLAYLIST_STANDARD			=  3,
			PLAYLIST_CHAOS				=  4,
			PLAYLIST_RANKEDDUEL			=  10,
			PLAYLIST_RANKEDDOUBLES		=  11,
			PLAYLIST_RANKEDSOLOSTANDARD =  12,
			PLAYLIST_RANKEDSTANDARD		=  13,
			PLAYLIST_RANKEDHOOPS		=  27,
			PLAYLIST_RANKEDRUMBLE		=  28,
			PLAYLIST_RANKEDDROPSHOT		=  29,
			PLAYLIST_RANKEDSNOWDAY		=  30,

			PLAYLIST_CASUAL				=  998 // Used to store the casual playlist at one place (unofficial playlist)
		};

		// Returns the playlist Type based on the given int (playlist enumeration value).
		Type FromInt(int type);

		// Returns the name of the given playlist. Empty string if unknown/unsupported.
		std::string GetName( Type type );

		// Checks if the playlist is a casual gamemode (all MMR should be the same there)
		bool IsCasualPlaylist(Type type);

		// If the given type is a casual gamemode, we'll return PLAYLIST_CASUAL instead of the given type
		inline Type ConvertToCasualType( Type type );

		// Returns true if the playlist Type is known (in the Type enumeration)
		inline bool IsKnown( Type type );


		inline int GetPlaylistGameSize( ssp::playlist::Type playlist );
		inline std::string GetPlaylistName( ssp::playlist::Type playlist );
	}
}

inline bool ssp::playlist::IsKnown( ssp::playlist::Type type )
{
	return type != ssp::playlist::Type::PLAYLIST_UNKNOWN;
}

inline ssp::playlist::Type ssp::playlist::ConvertToCasualType( ssp::playlist::Type type )
{
	return ssp::playlist::IsCasualPlaylist( type ) ? ssp::playlist::Type::PLAYLIST_CASUAL : type;
}

inline int ssp::playlist::GetPlaylistGameSize( ssp::playlist::Type playlist )
{
	switch( playlist )
	{
		case ssp::playlist::Type::PLAYLIST_DUEL:
		case ssp::playlist::Type::PLAYLIST_RANKEDDUEL:
			return 2;
		case ssp::playlist::Type::PLAYLIST_DOUBLES:
		case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES:
		case ssp::playlist::Type::PLAYLIST_RANKEDHOOPS:
			return 4;
		case ssp::playlist::Type::PLAYLIST_STANDARD:
		case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD:
		case ssp::playlist::Type::PLAYLIST_RANKEDDROPSHOT:
		case ssp::playlist::Type::PLAYLIST_RANKEDSNOWDAY:
		case ssp::playlist::Type::PLAYLIST_RANKEDRUMBLE:
			return 6;
		case Type::PLAYLIST_CHAOS:
			return 8;
		default:
			return -1;
	}
}
inline std::string ssp::playlist::GetPlaylistName( ssp::playlist::Type playlist )
{
	switch( playlist )
	{
		case ssp::playlist::Type::PLAYLIST_DUEL:
		case ssp::playlist::Type::PLAYLIST_DOUBLES:
		case ssp::playlist::Type::PLAYLIST_STANDARD:
		case ssp::playlist::Type::PLAYLIST_CHAOS:
			return "Casual";
		case ssp::playlist::Type::PLAYLIST_RANKEDDUEL:
			return "Solo";
		case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES:
			return "Doubles";
		case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD:
			return "Standard";
		case ssp::playlist::Type::PLAYLIST_RANKEDDROPSHOT:
			return "Dropshot";
		case ssp::playlist::Type::PLAYLIST_RANKEDHOOPS:
			return "Hoops";
		case ssp::playlist::Type::PLAYLIST_RANKEDSNOWDAY:
			return "Snowday";
		case ssp::playlist::Type::PLAYLIST_RANKEDRUMBLE:
			return "Rumble";
		default:
			return "Unknown";
	}
}