#include "Renderer.h"

#include <sstream>
#include <iomanip>

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

void ssp::Renderer::RenderStats( CanvasWrapper * canvas, ssp::playlist::Stats & stats, ssp::playlist::Type type )
{
	std::stringstream stringStream;

	Vector2 position = {
		*posX,
		*posY
	};

	// DRAW BOX
	canvas->SetColor( BOX_FILL_COLOR );
	canvas->SetPosition( position );
	canvas->FillBox( GetStatsDisplaySize(type) );


	// DRAW CURRENT PLAYLIST
	canvas->SetColor( TITLE_COLOR );
	canvas->SetPosition( Vector2{ position.X + 10, position.Y + 5 } );

	float currentMmr = stats.currentMmr;
	stringStream.clear();
	stringStream << ssp::playlist::GetName( type ) << " (" << std::setprecision( 2 ) << std::showpoint << std::fixed << currentMmr << ")";
	canvas->DrawString( stringStream.str() );


	// DRAW MMR
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 24, position.Y + 21 } );
	canvas->DrawString( "MMR:" );

	float mmrGain = stats.currentMmr - stats.initialMmr;
	if( mmrGain > 0 )
	{
		canvas->SetColor( GREEN_COLOR );
	}
	else if( mmrGain < 0 )
	{
		canvas->SetColor( RED_COLOR );
	}
	canvas->SetPosition( Vector2{ position.X + 65, position.Y + 21 } );

	stringStream.str(""); 
	stringStream << std::fixed << std::setprecision( 2 ) << std::showpoint << std::fixed << mmrGain;
	canvas->DrawString( ( mmrGain > 0 ? "+" : "" ) + stringStream.str() );


	// DRAW WINS
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 23, position.Y + 37 } );
	canvas->DrawString( "Wins:" );

	canvas->SetColor( GREEN_COLOR );
	canvas->SetPosition( Vector2{ position.X + 65, position.Y + 37 } );
	canvas->DrawString( std::to_string( stats.wins ) );


	// DRAW LOSSES
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 10, position.Y + 53 } );
	canvas->DrawString( "Losses:" );

	canvas->SetColor( RED_COLOR );
	canvas->SetPosition( Vector2{ position.X + 65, position.Y + 53 } );
	canvas->DrawString( std::to_string( stats.losses ) );


	// DRAW STREAK
	canvas->SetColor( LABEL_COLOR );
	canvas->SetPosition( Vector2{ position.X + 11, position.Y + 69 } );
	canvas->DrawString( "Streak:" );

	int streak = stats.streak;
	if( streak > 0 )
	{
		canvas->SetColor( GREEN_COLOR );
	}
	else if( streak < 0 )
	{
		canvas->SetColor( RED_COLOR );
	}
	canvas->SetPosition( Vector2{ position.X + 65, position.Y + 69 } );
	canvas->DrawString( ( streak > 0 ? "+" : "" ) + std::to_string( streak ) );
}

Vector2 ssp::Renderer::GetStatsDisplaySize( ssp::playlist::Type type )
{
	switch( type )
	{
		case ssp::playlist::Type::PLAYLIST_RANKEDDUEL: return Vector2{ 175, 88 };
		case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES: return Vector2{ 200, 88 };
		case ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD: return Vector2{ 240, 88 };
		case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD: return Vector2{ 210, 88 };
		default: return Vector2{ 220, 88 };
	}
}
