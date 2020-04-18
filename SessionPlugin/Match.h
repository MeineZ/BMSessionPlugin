#pragma once

#include <Playlist.h>

class GameWrapper;

namespace ssp // SessionPlugin
{
	namespace match
	{
		enum class Result
		{
			LOSS = -1,
			TIE = 0,
			WIN = 1
		};
	}

	class Match
	{
	private:
		ssp::playlist::Type type;
		int goals[2];
		int currentTeam;
		bool isActive;

	public:
		Match();

		void DetermineCurrentTeam( GameWrapper *gameWrapper );

		void MatchEndReset();
		void FullReset();

		// Receive and set the current match score (ASSUMES WE ARE IN A VALID GAME! GUARD THIS FUNCTION WITH AN SessionPlugin::CheckValidGame()
		void SetCurrentGameGoals( GameWrapper *gameWrapper );

		void SetWinOrLoss( ssp::playlist::Stats &stats );

		void OnMatchStart( GameWrapper *gameWrapper );

		inline match::Result GetStanding();

		inline ssp::playlist::Type GetMatchType();
		inline bool IsActive();
		inline int GetCurrentTeam( );
		inline int GetTeamScore(int team);

		inline void SetMatchType(int playlistType);
		inline void Deactivate( );
	};
}

inline ssp::playlist::Type ssp::Match::GetMatchType()
{
	return type;
}

inline bool ssp::Match::IsActive()
{
	return isActive;
}

inline int ssp::Match::GetCurrentTeam()
{
	return currentTeam;
}

inline int ssp::Match::GetTeamScore( int team )
{
	return team >= 0 && team < 2 ? goals[team] : 0;
}

inline void ssp::Match::SetMatchType( int playlistType )
{ 
	type = ssp::playlist::FromInt( playlistType );
}

inline void ssp::Match::Deactivate()
{ 
	isActive = false;
}

inline ssp::match::Result ssp::Match::GetStanding()
{
	return (
		( goals[0] > goals[1] && currentTeam == 0 ) || ( goals[1] > goals[0] && currentTeam == 1 )
		? match::Result::WIN
		: (
			goals[0] == goals[1]
			? match::Result::TIE
			: match::Result::LOSS
		)
	);
}
