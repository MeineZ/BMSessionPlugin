#include <MMR.h>

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>

#include <Playlist.h>
#include <Settings.h>

ssp::MMR::MMR( float initialMmr ):
	Loggable()
{
	Reset( initialMmr );
}

void ssp::MMR::Reset(float initialMmr )
{
	initial = initialMmr;
	current = initialMmr;
	lastDiff = 0.0f;
	streakMmrStamp = initialMmr;
}

ssp::mmr::RequestResult ssp::MMR::RequestMmrUpdate( GameWrapper *gameWrapper, UniqueIDWrapper &uniqueID, const ssp::playlist::Type matchType, bool force )
{
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if( force )
	{
		float mmr = mmrWrapper.GetPlayerMMR( uniqueID, static_cast<int>( matchType ) );
		if( current == mmr)
			return ssp::mmr::RequestResult::SAME_VALUE;

		if( std::floor( mmr ) >= 100.f && std::floor( mmr ) < 101.f )
			return ssp::mmr::RequestResult::INVALID_VALUE;

		lastDiff = mmr - current;
		SetStreakMMRGain( );
		current = mmr;
		return ssp::mmr::RequestResult::SUCCESS;
	}
	else
	{
		if( mmrWrapper.IsSynced( uniqueID, static_cast<int>( matchType ) ) && !mmrWrapper.IsSyncing( uniqueID ) )
		{
			float mmr = mmrWrapper.GetPlayerMMR( uniqueID, static_cast<int>( matchType ) );
			if( current == mmr )
				return ssp::mmr::RequestResult::SAME_VALUE;

			if( std::floor( mmr ) >= 100.f && std::floor( mmr ) < 101.f )
				return ssp::mmr::RequestResult::INVALID_VALUE;

			lastDiff = mmr - current;
			SetStreakMMRGain( );
			current = mmr;
			return ssp::mmr::RequestResult::SUCCESS;
		}
	}
	return ssp::mmr::RequestResult::NOT_SYNCED;
}

void ssp::MMR::SetStreakMMRGain()
{
	float streakDiff = current - streakMmrStamp;
	if( streakDiff < 0.0f )
	{
		streakMmrStamp = lastDiff < 0.0f
			? streakMmrStamp
			: current;
	}
	else if( streakDiff > 0.0f )
	{
		streakMmrStamp = lastDiff > 0.0f
			? streakMmrStamp
			: current;
	}
	else
		streakMmrStamp = current;
}

void ssp::MMR::Log( CVarManagerWrapper *cvarManager )
{
	SSP_NO_PLUGIN_LOG( "Initial: " + std::to_string( initial) );
	SSP_NO_PLUGIN_LOG( "Current: " + std::to_string( current ) );
	SSP_NO_PLUGIN_LOG( "LastDiff: " + std::to_string( lastDiff ) );
	SSP_NO_PLUGIN_LOG( "Streak: " + std::to_string( current - streakMmrStamp ) );
}
