#pragma once

#include <Playlist.h>

class GameWrapper;

namespace ssp // SessionPlugin
{
	namespace match
	{
		// Contains match results
		// Tie is not really a result in RL, but when the player leaves, it may happen to be a tie.
		// Ties are ignored in the session, to punish the player and lazyness from mr. programmer
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
		ssp::playlist::Type type; // The match type
		int goals[2]; // Amount of goals scored for each team
		int currentTeam; // The team that the player is in
		bool isActive; // Whether the match is currently active or if it ended

	public:
		// Construct Match
		Match();

		// Reset on match end
		void MatchEndReset();
		// Fully reset current game stats. Basically Match:::MatchEndReset() but with a playlist type reset.
		void FullReset();

		// Should be called on match start. Initializes the current match data
		void OnMatchStart( GameWrapper *gameWrapper );

		// Determine the team that the current player is in
		// ASSUMES WE ARE IN A VALID GAME! GUARD THIS FUNCTION WITH AN SessionPlugin::CheckValidGame()
		void DetermineCurrentTeam( GameWrapper *gameWrapper );

		// Receive and set the current match score 
		// ASSUMES WE ARE IN A VALID GAME! GUARD THIS FUNCTION WITH AN SessionPlugin::CheckValidGame()
		void SetCurrentGameGoals( GameWrapper *gameWrapper );

		// Detemine if the player lost or not based on the current known score.
		// Also updates the session data, as we assume that the game is over.
		void SetWinOrLoss( ssp::playlist::Stats &stats );

		// Returns the current standing based on the currently known score.
		inline match::Result GetStanding();

		inline ssp::playlist::Type GetMatchType(); // Returns the current match type
		inline bool IsActive(); // Returns if a match is being tracked/active
		inline int GetCurrentTeam(); // Returns the current team the player is in
		inline int GetTeamScore(int team); // Returns the score of the specified team

		inline void SetMatchType(int playlistType); // Sets the playlist type from a number
		inline void Deactivate( ); // Deactivates the current match and stops tracking
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
