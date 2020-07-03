#include "Renderer.h"

#include <sstream>

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/canvaswrapper.h>

#define BOX_FILL_COLOR 0, 0, 0, 75
#define TITLE_COLOR 255, 255, 255, 127
#define LABEL_COLOR 255, 255, 255, 150
#define GREEN_COLOR 95, 232, 95, 127
#define RED_COLOR 232, 95, 95, 127

ssp::Renderer::Renderer() :
	posX(std::make_shared<int>(420)),
	posY(std::make_shared<int>(0))
{ }

void ssp::Renderer::RenderStats( CanvasWrapper *canvas, ssp::playlist::Stats &stats, ssp::playlist::Type type )
{
	std::stringstream stringStream;

	Vector2 position = {
		*posX,
		*posY
	};

	// DRAW BOX
	canvas->SetColor( BOX_FILL_COLOR );
	canvas->SetPosition( position );
	canvas->FillBox( GetStatsDisplaySize( type ) );


	// DRAW CURRENT PLAYLIST
	canvas->SetColor( TITLE_COLOR );
	canvas->SetPosition( Vector2{ position.X + 10, position.Y + 5 } );

	stringStream << ssp::playlist::GetName( type ) << " (";
	stats.mmr.SetCurrentSStream( stringStream );
	stringStream << ")";
	canvas->DrawString( stringStream.str() );


	// DRAW MMR
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 47, position.Y + 21 } );
	canvas->DrawString( "MMR:" );

	float mmrGain = stats.mmr.GetDiff();
	SetColorByValue( canvas, mmrGain );
	canvas->SetPosition( Vector2{ position.X + 88, position.Y + 21 } );

	stringStream.str( "" );
	stats.mmr.SetDiffSStream( stringStream );
	canvas->DrawString( stringStream.str() );

	// DRAW LAST GAME MMR GAIN
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 10, position.Y + 37 } );
	canvas->DrawString( "Last game:" );

	float lastGameGain = stats.mmr.lastDiffDisplay;
	SetColorByValue( canvas, lastGameGain );
	canvas->SetPosition( Vector2{ position.X + 88, position.Y + 37 } );

	stringStream.str( "" );
	stats.mmr.SetLastGameSStream( stringStream );
	canvas->DrawString( stringStream.str() );


	// DRAW WINS
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 46, position.Y + 53 } );
	canvas->DrawString( "Wins:" );

	canvas->SetColor( GREEN_COLOR );
	canvas->SetPosition( Vector2{ position.X + 88, position.Y + 53 } );
	canvas->DrawString( std::to_string( stats.wins ) );


	// DRAW LOSSES
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 33, position.Y + 69 } );
	canvas->DrawString( "Losses:" );

	canvas->SetColor( RED_COLOR );
	canvas->SetPosition( Vector2{ position.X + 88, position.Y + 69 } );
	canvas->DrawString( std::to_string( stats.losses ) );


	// DRAW STREAK
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 34, position.Y + 85 } );
	canvas->DrawString( "Streak:" );

	int streak = stats.streak;
	SetColorByValue( canvas, static_cast<float>(streak));
	canvas->SetPosition( Vector2{ position.X + 88, position.Y + 85 } );
	stringStream.str( "" );
	stringStream << ( streak > 0 ? "+" : "" ) << streak;
	if( stats.mmr.streakMmrGain != 0.0f )
	{
		stringStream << " (" << ( stats.mmr.streakMmrGain > 0 ? "+" : "" ) << stats.mmr.streakMmrGain << ")";
	}
	canvas->DrawString( stringStream.str() );
}

Vector2 ssp::Renderer::GetStatsDisplaySize( ssp::playlist::Type type )
{
	switch( type )
	{
		case ssp::playlist::Type::PLAYLIST_RANKEDDUEL: return Vector2{ 175, 104 };
		case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES: return Vector2{ 200, 104 };
		case ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD: return Vector2{ 240, 104 };
		case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD: return Vector2{ 210, 104 };
		default: return Vector2{ 220, 104 };
	}
}

void ssp::Renderer::SetColorByValue( CanvasWrapper *canvasWrapper, float value )
{ 
	if( value > 0.0f )
	{
		canvasWrapper->SetColor( GREEN_COLOR );
	}
	else if( value < 0.0f )
	{
		canvasWrapper->SetColor( RED_COLOR );
	}
	else
	{
		canvasWrapper->SetColor( LABEL_COLOR );
	}
}
