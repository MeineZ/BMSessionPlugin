#pragma once

#include <memory>

#include <Playlist.h>

class CVarManagerWrapper;
class CanvasWrapper;

namespace ssp
{
	class Renderer
	{
	public:
		std::shared_ptr<int> posX;
		std::shared_ptr<int> posY;

		Renderer();

		// Renders given stats with the given playlist name at the specified position (posX, posY)
		void RenderStats( CanvasWrapper *canvasWrapper, ssp::playlist::Stats & stats, std::string & playlistName);
	};
};

