#include <MMR.h>

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/MMRWrapper.h>

#include <Playlist.h>

bool ssp::MMR::RequestMmrUpdate( GameWrapper *gameWrapper, SteamID &steamID, const ssp::playlist::Type const *matchType, bool force )
{
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if( force )
	{
		float mmr = mmrWrapper.GetPlayerMMR( steamID, static_cast<int>( *matchType ) );
		if( current == mmr && std::floor( mmr ) != 100.f )
		{
			return false;
		}
		current = mmr;
		return true;
	}
	else
	{
		if( mmrWrapper.IsSynced( steamID, static_cast<int>( *matchType ) ) && !mmrWrapper.IsSyncing( steamID ) )
		{
			float mmr = mmrWrapper.GetPlayerMMR( steamID, static_cast<int>( *matchType ) );
			if( current == mmr )
			{
				return false;
			}
			current = mmr;
			return true; 
		}
	}
	return false;
}