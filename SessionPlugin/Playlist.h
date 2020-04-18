#pragma once

#include <string>

namespace ssp // Session plugin
{
	namespace playlist
	{
		struct Stats
		{
			float initialMmr;
			float currentMmr;
			int wins;
			int losses;
			int streak;
		};

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

		Type FromInt(int type);

		std::string GetName( Type type );

		inline bool IsKnown( Type type );
	}
}

inline bool ssp::playlist::IsKnown( ssp::playlist::Type type )
{
	return type != ssp::playlist::Type::PLAYLIST_UNKOWN;
}