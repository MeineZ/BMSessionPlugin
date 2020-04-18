#include "Match.h"

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>
#include <bakkesmod/wrappers/GameObject/CarWrapper.h>
#include <bakkesmod/wrappers/GameObject/TeamWrapper.h>

ssp::Match::Match() :
	type(ssp::playlist::Type::PLAYLIST_UNKOWN),
	goals(),
	currentTeam(-1),
	isActive(false)
{ 
	goals[0] = goals[1] = 0;
}

void ssp::Match::DetermineCurrentTeam( GameWrapper * gameWrapper )
{ 
	// Only determine the current team if it's unknown.
	// We expect the current team to be -1, when a new game started.
	if( currentTeam == -1 )
	{
		CarWrapper car = gameWrapper->GetLocalCar();
		if( !car.IsNull() )
		{
			currentTeam = car.GetTeamNum2();
		}
	}
}

void ssp::Match::MatchEndReset()
{
	// Soft reset data
	goals[0] = goals[1] = 0;
	currentTeam = -1;
	isActive = false;
}

void ssp::Match::FullReset()
{
	// Hard reset data
	type = ssp::playlist::Type::PLAYLIST_UNKOWN;
	goals[0] = goals[1] = 0;
	currentTeam = -1;
	isActive = false;
}

void ssp::Match::SetCurrentGameGoals(GameWrapper * gameWrapper)
{
	// We're assuming that we're in a valid game here! Guard this with with a valid game check!

	// Something must have gone wrong if the team number wasn't found
	// We just retry again (important to note is that the current team number is very important to figure out who won)
	

	// Get the current online game
	ServerWrapper serverWrapper = gameWrapper->GetOnlineGame();
	if( !serverWrapper.IsNull() )
	{
		// Get teams
		ArrayWrapper<TeamWrapper> teams = serverWrapper.GetTeams();
		if( teams.Count() == 2 )
		{
			// Get the score of both teams
			goals[0] = teams.Get( 0 ).GetScore();
			goals[1] = teams.Get( 1 ).GetScore();
		}
	}
}

void ssp::Match::SetWinOrLoss(ssp::playlist::Stats & stats)
{
	ssp::match::Result result = GetStanding();
	if( result == ssp::match::Result::WIN )
	{
		// You won! :D
		stats.wins++;
		if( stats.streak < 0 )
		{
			stats.streak = 1;
		}
		else
		{
			stats.streak++;
		}
	}
	else if(result == ssp::match::Result::LOSS )
	{
		// You lost! D:
		stats.losses++;
		if( stats.streak > 0 )
		{
			stats.streak = -1;
		}
		else
		{
			stats.streak--;
		}
	}
}

void ssp::Match::OnMatchStart(GameWrapper * gameWrapper)
{
	// Current game is active
	isActive = true;

	// Get car and team number
	currentTeam = -1;
	DetermineCurrentTeam(gameWrapper);
}
