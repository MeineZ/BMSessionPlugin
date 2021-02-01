#pragma once

#include <Playlist.h>

class CVarManager;

namespace ssp // SessionPlugin
{
	class SessionPlugin;

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

		enum class State
		{
			IDLE = 0,
			ONGOING = 1,
			ENDED = 2
		};
	}

	class Match : public Loggable
	{
	private:
		ssp::playlist::Type type; // The match type
		ssp::match::State currentState; // Current state of the match
		int goals[2]; // Amount of goals scored for each team
		int currentTeam; // The team that the player is in

	public:
		// Construct Match
		Match();

		// Resets to default values
		void Reset();

		// Should be called on match start. Initializes the current match data
		void OnMatchStart(GameWrapper* gameWrapper);
		// Should be called when a team scored. Updates the current score
		void OnGoalScored(GameWrapper* gameWrapper);
		// Should be called when a match ended. Handles the end of a game, also handles when the player left the match before it actually ended.
		void OnMatchEnded(GameWrapper* gameWrapper);

		// Finds the current team of the player
		void FindCurrentTeam( GameWrapper *gameWrapper );

		// Receive and set the current match score
		void SetCurrentGameGoals( GameWrapper *gameWrapper );

		virtual void Log( CVarManagerWrapper *cvarManager );

		// Returns the current standing based on the currently known score.
		inline match::Result GetStanding();

		inline ssp::playlist::Type GetMatchType(); // Returns the current match type
		inline void SetMatchType(ssp::playlist::Type playlistType); // Sets the playlist type from a number

		inline ssp::match::State GetMatchState(); // Returns the current match type
		inline void SetMatchState(ssp::match::State state); // Sets the match state
	};
}

inline ssp::playlist::Type ssp::Match::GetMatchType()
{
	return type;
}

inline void ssp::Match::SetMatchType(ssp::playlist::Type playlistType)
{
	type = playlistType;
}

inline ssp::match::State ssp::Match::GetMatchState()
{
	return currentState;
}

inline void ssp::Match::SetMatchState( ssp::match::State state )
{ 
	currentState = state;
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
