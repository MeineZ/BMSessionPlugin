#pragma once

#include <string>

namespace ssp // Session plugin
{
	namespace playlist
	{
		// Contains all stats that a playlist session can have.
		struct Stats
		{
			float initialMmr;
			float currentMmr;
			int wins;
			int losses;
			int streak;
		};

		// All supported types (types that are consider worth tracking).
		enum class Type
		{
			PLAYLIST_UNKOWN				= -1,
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
			PLAYLIST_RANKEDSNOWDAY		=  30
		};

		// Returns the playlist Type based on the given int (playlist enumeration value).
		Type FromInt(int type);

		// Returns the name of the given playlist. Empty string if unknown/unsupported.
		std::string GetName( Type type );

		// Returns true if the playlist Type is known (in the Type enumeration)
		inline bool IsKnown( Type type );
	}
}

inline bool ssp::playlist::IsKnown( ssp::playlist::Type type )
{
	return type != ssp::playlist::Type::PLAYLIST_UNKOWN;
}