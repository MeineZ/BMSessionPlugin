#include "Match.h"

#include <bakkesmod/wrappers/gamewrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>
#include <bakkesmod/wrappers/GameObject/CarWrapper.h>
#include <bakkesmod/wrappers/GameObject/TeamWrapper.h>

#include <SessionPlugin.h>

ssp::Match::Match() :
	type(ssp::playlist::Type::PLAYLIST_UNKNOWN),
	currentState(ssp::match::State::IDLE),
	goals(),
	currentTeam(-1)
{ 
	goals[0] = goals[1] = 0;
}

void ssp::Match::Reset()
{
	currentState = match::State::IDLE;
	currentTeam = TEAM_INVALID_VALUE;
	goals[0] = goals[1] = 0;
}

void ssp::Match::OnMatchStart(GameWrapper* gameWrapper)
{
	// Current game is active
	currentState = match::State::ONGOING;

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	type = ssp::playlist::FromInt(mmrWrapper.GetCurrentPlaylist());

	// Get car and team number
	currentTeam = TEAM_INVALID_VALUE;
	FindCurrentTeam(gameWrapper);

	// Reset goals
	SetCurrentGameGoals(gameWrapper);
}

void ssp::Match::OnGoalScored(GameWrapper* gameWrapper)
{
	SetCurrentGameGoals( gameWrapper );
}

void ssp::Match::OnMatchEnded(GameWrapper* gameWrapper)
{
	if(currentState == match::State::ONGOING)
		currentState = match::State::ENDED;
}

void ssp::Match::FindCurrentTeam( GameWrapper * gameWrapper )
{
	if( !ssp::playlist::IsKnown( type ) || currentState != match::State::ONGOING )
	{
		currentTeam = TEAM_INVALID_VALUE;
		return;
	}

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

void ssp::Match::SetCurrentGameGoals(GameWrapper * gameWrapper)
{
	if( !ssp::playlist::IsKnown( type ) || currentState != match::State::ONGOING )
	{
		// Shouldn't even get here
		goals[0] = goals[1] = 0;
		return;
	}

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
	else
	{
		goals[0] = goals[1] = 0;
	}
}

void ssp::Match::Log( CVarManagerWrapper *cvarManager )
{ 
	SSP_NO_PLUGIN_LOG("Match type: "+ std::to_string(static_cast<int>( type )));
	SSP_NO_PLUGIN_LOG( "Current state: " + std::to_string( static_cast<int>( currentState ) ) );
	SSP_NO_PLUGIN_LOG( "Goals: " + std::to_string( static_cast<int>( goals[0] ) ) + " - " + std::to_string( static_cast<int>( goals[1] ) ) );
	SSP_NO_PLUGIN_LOG( "Current team: " + std::to_string( static_cast<int>( currentTeam ) ) );
}