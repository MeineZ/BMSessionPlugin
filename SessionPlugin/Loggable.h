#pragma once

class CVarManagerWrapper;

namespace ssp
{
	class Loggable
	{
		virtual void Log( CVarManagerWrapper *cvarManager ) = 0;
	};
}