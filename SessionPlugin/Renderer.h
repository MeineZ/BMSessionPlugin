#pragma once

#include <memory>

#include <bakkesmod/plugin/bakkesmodplugin.h>

#include <Playlist.h>

class CVarManagerWrapper;
class CanvasWrapper;

namespace ssp
{
	class SessionPlugin;

	class Renderer
	{
	public:
		std::shared_ptr<int> posX;
		std::shared_ptr<int> posY;

		std::shared_ptr<LinearColor> colorBackground; // The color used for the background
		std::shared_ptr<LinearColor> colorTitle; // The color used for title texts
		std::shared_ptr<LinearColor> colorLabel; // The color used for label texts
		std::shared_ptr<LinearColor> colorPositive; // The color used for positive texts
		std::shared_ptr<LinearColor> colorNegative; // The color used for negative texts

		Renderer();

		// Renders given stats with the given playlist name at the specified position (posX, posY)
		void RenderStats( CanvasWrapper *canvasWrapper, ssp::playlist::Stats & stats, ssp::playlist::Type type );

		Vector2 GetStatsDisplaySize( ssp::playlist::Type type );

	private:
		void SetColorByValue( CanvasWrapper *canvasWrapper, float value);
	};
};

