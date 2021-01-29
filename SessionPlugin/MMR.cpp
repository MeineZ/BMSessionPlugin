#include <MMR.h>

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>

#include <Playlist.h>

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
	streakMmrGain = 0.0f;
}

bool ssp::MMR::RequestMmrUpdate( GameWrapper *gameWrapper, UniqueIDWrapper &uniqueID, const ssp::playlist::Type matchType, bool force )
{

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if( force )
	{
		float mmr = mmrWrapper.GetPlayerMMR( uniqueID, static_cast<int>( matchType ) );
		if( current == mmr || std::floor( mmr ) < 101.f )
		{
			return false;
		}

		lastDiff = mmr - current;
		SetStreakMMRGain();
		current = mmr;
		return true;
	}
	else
	{
		if( mmrWrapper.IsSynced( uniqueID, static_cast<int>( matchType ) ) && !mmrWrapper.IsSyncing( uniqueID ) )
		{
			float mmr = mmrWrapper.GetPlayerMMR( uniqueID, static_cast<int>( matchType ) );
			if( current == mmr || std::floor( mmr ) < 101.f )
			{
				return false;
			}

			lastDiff = mmr - current;
			SetStreakMMRGain();
			current = mmr;
			return true;
		}
	}
	return false;
}

void ssp::MMR::SetStreakMMRGain()
{
	if( streakMmrGain < 0.0f )
	{
		streakMmrGain = lastDiff < 0.0f
			? ( streakMmrGain - lastDiff )
			: lastDiff;
	}
	else if( streakMmrGain > 0.0f )
	{
		streakMmrGain = lastDiff > 0.0f
			? ( streakMmrGain + lastDiff )
			: lastDiff;
	}
	else
		streakMmrGain = lastDiff;
}

void ssp::MMR::Log( CVarManagerWrapper *cvarManager )
{
	cvarManager->log( "Initial: " + std::to_string( initial) );
	cvarManager->log( "Current: " + std::to_string( current ) );
	cvarManager->log( "LastDiff: " + std::to_string( lastDiff ) );
}
