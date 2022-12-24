#include "Renderer.h"

#include <sstream>

#include <bakkesmod/wrappers/cvarmanagerwrapper.h>
#include <bakkesmod/wrappers/canvaswrapper.h>

#include <SessionPlugin.h>

ssp::Renderer::Renderer() :
	posX(std::make_shared<int>(420)),
	posY(std::make_shared<int>(0)),
	scale(std::make_shared<float>(1.0f)),
	colorBackground( std::make_shared<LinearColor>( LinearColor() ) ),
	colorTitle( std::make_shared<LinearColor>( LinearColor() ) ),
	colorLabel( std::make_shared<LinearColor>( LinearColor() ) ),
	colorPositive( std::make_shared<LinearColor>( LinearColor() ) ),
	colorNegative( std::make_shared<LinearColor>( LinearColor() ) )
{ }

void ssp::Renderer::RenderStats( CanvasWrapper *canvas, ssp::playlist::Stats &stats, ssp::playlist::Type type )
{
	std::stringstream stringStream;

	Vector2 position = {
		*posX,
		*posY
	};

	// DRAW BOX
	canvas->SetColor( *colorBackground );
	canvas->SetPosition( position );
	canvas->FillBox(GetStatsDisplaySize(type));

	// DRAW CURRENT PLAYLIST
	canvas->SetColor( *colorTitle );
	canvas->SetPosition( Vector2{ position.X + 10, position.Y + (int)(5.0f * *scale) } );

	stringStream << ssp::playlist::GetName( type ) << " (";
	stats.mmr.SetCurrentSStream( stringStream );
	stringStream << ")";
	canvas->DrawString(stringStream.str(), *scale, *scale );


	// DRAW MMR
	canvas->SetColor( *colorLabel );
	canvas->SetPosition( Vector2{ position.X + (int)(47.0f * *scale), position.Y + (int)(21.0f * *scale) } );
	canvas->DrawString( "MMR:", *scale, *scale);

	float mmrGain = stats.mmr.GetDiff();
	SetColorByValue( canvas, mmrGain );
	canvas->SetPosition( Vector2{ position.X + (int)(88.0f * *scale), position.Y + (int)(21.0f * *scale) } );

	stringStream.str( "" );
	stats.mmr.SetDiffSStream( stringStream );
	canvas->DrawString( stringStream.str(), *scale, *scale);

	// DRAW LAST GAME MMR GAIN
	canvas->SetColor( *colorLabel );
	canvas->SetPosition(Vector2{ position.X + (int)(10.0f * *scale), position.Y + (int)(37.0f * *scale) } );
	canvas->DrawString( "Last game:", *scale, *scale);

	float lastGameGain = stats.mmr.lastDiff;
	SetColorByValue( canvas, lastGameGain );
	canvas->SetPosition( Vector2{ position.X + (int)(88.0f * *scale), position.Y + (int)(37.0f * *scale) } );

	stringStream.str( "" );
	stats.mmr.SetLastGameSStream( stringStream );
	canvas->DrawString( stringStream.str(), *scale, *scale);


	// DRAW WINS
	canvas->SetColor( *colorLabel );
	canvas->SetPosition( Vector2{ position.X + (int)(46.0f * *scale), position.Y + (int)(53.0f * *scale) } );
	canvas->DrawString( "Wins:", *scale, *scale);

	canvas->SetColor( *colorPositive );
	canvas->SetPosition( Vector2{ position.X + (int)(88.0f * *scale), position.Y + (int)(53.0f * *scale) } );
	canvas->DrawString( std::to_string( stats.wins ), *scale, *scale);


	// DRAW LOSSES
	canvas->SetColor( *colorLabel );
	canvas->SetPosition( Vector2{ position.X + (int)(33.0f * *scale), position.Y + (int)(69.0f * *scale) } );
	canvas->DrawString( "Losses:", *scale, *scale);

	canvas->SetColor( *colorNegative );
	canvas->SetPosition( Vector2{ position.X + (int)(88.0f * *scale), position.Y + (int)(69.0f * *scale) } );
	canvas->DrawString( std::to_string( stats.losses ), *scale, *scale);


	// DRAW STREAK
	canvas->SetColor( *colorLabel );
	canvas->SetPosition( Vector2{ position.X + (int)(34.0f * *scale), position.Y + (int)(85.0f * *scale) });
	canvas->DrawString( "Streak:", *scale, *scale);

	int streak = stats.streak;
	SetColorByValue(canvas, static_cast<float>(streak));
	canvas->SetPosition( Vector2{ position.X + (int)(88.0f * *scale), position.Y + (int)(85.0f * *scale) } );
	stringStream.str( "" );
	stringStream << ( streak > 0 ? "+" : "" ) << streak;
	float streakDiff = stats.mmr.current - stats.mmr.streakMmrStamp;
	if( stats.mmr.current - stats.mmr.streakMmrStamp != 0.0f )
	{
		stringStream << " (" << ( streakDiff > 0 ? "+" : "" ) << streakDiff << ")";
	}
	canvas->DrawString( stringStream.str(), *scale, *scale);
}

Vector2 ssp::Renderer::GetStatsDisplaySize( ssp::playlist::Type type )
{
	switch( type )
	{
		case ssp::playlist::Type::PLAYLIST_RANKEDDUEL: return Vector2{ (int)(175.0f * *scale), (int)(104.0f * *scale) };
		case ssp::playlist::Type::PLAYLIST_RANKEDDOUBLES: return Vector2{ (int)(200.0f * *scale), (int)(104.0f * *scale) };
		case ssp::playlist::Type::PLAYLIST_RANKEDSOLOSTANDARD: return Vector2{ (int)(240.0f * *scale), (int)(104.0f * *scale) };
		case ssp::playlist::Type::PLAYLIST_RANKEDSTANDARD: return Vector2{ (int)(210.0f * *scale), (int)(104.0f * *scale) };
		default: return Vector2{ (int)(220.0f * *scale), (int)(104.0f * *scale) };
	}
}

void ssp::Renderer::SetColorByValue(CanvasWrapper *canvasWrapper, float value )
{ 
	if( value > 0.0f )
	{
		canvasWrapper->SetColor( *colorPositive );
	}
	else if( value < 0.0f )
	{
		canvasWrapper->SetColor( *colorNegative );
	}
	else
	{
		canvasWrapper->SetColor( *colorLabel );
	}
}
