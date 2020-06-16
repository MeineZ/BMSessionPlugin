#include <MMR.h>

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>

#include <Playlist.h>

ssp::MMR::MMR( float initialMmr ) :
	Loggable(),
	initial(initialMmr),
	current(initialMmr),
	lastDiffDisplay(0.0f),
	lastDiff(0.0f),
	streakMmrGain(0.0f)
{ }

bool ssp::MMR::RequestMmrUpdate( GameWrapper *gameWrapper, SteamID &steamID, const ssp::playlist::Type const *matchType, bool force )
{

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if( force )
	{
		float mmr = mmrWrapper.GetPlayerMMR( steamID, static_cast<int>( *matchType ) );
		if( current == mmr || std::floor( mmr ) < 101.f )
		{
			return false;
		}

		// Only set last difference if a new game was started
		// Which is detected by last diff being set to 0
		if( lastDiff == 0.0f )
		{
			lastDiff = mmr - current;
		}
		lastDiffDisplay = lastDiff;
		current = mmr;
		return true;
	}
	else
	{
		if( mmrWrapper.IsSynced( steamID, static_cast<int>( *matchType ) ) && !mmrWrapper.IsSyncing( steamID ) )
		{
			float mmr = mmrWrapper.GetPlayerMMR( steamID, static_cast<int>( *matchType ) );
			if( current == mmr || std::floor( mmr ) < 101.f )
			{
				return false;
			}

			// Only set last difference if a new game was started
			// Which is detected by last diff being set to 0
			if( lastDiff == 0.0f )
			{
				lastDiff = mmr - current;
			}
			lastDiffDisplay = lastDiff;
			current = mmr;
			return true;
		}
	}
	return false;
}

void ssp::MMR::Log( CVarManagerWrapper *cvarManager )
{
	cvarManager->log( "Initial: " + std::to_string( initial) );
	cvarManager->log( "Current: " + std::to_string( current ) );
	cvarManager->log( "LastDiff: " + std::to_string( lastDiff ) );
}
