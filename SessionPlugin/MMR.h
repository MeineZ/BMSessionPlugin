#pragma once

#include <sstream>
#include <string>
#include <iomanip>

#include <bakkesmod/wrappers/wrapperstructs.h>

#include <Loggable.h>

class GameWrapper;
class CVarManagerWrapper;

namespace ssp // SessionPlugin
{
	namespace playlist
	{
		enum class Type;
	}

	class MMR : public Loggable
	{
	public:
		float initial = 0;
		float current = 0;

		float lastDiff = 0;

		MMR( float initialMmr );

		bool RequestMmrUpdate(GameWrapper * gameWrapper, SteamID & steamId, const ssp::playlist::Type const * matchType, bool force = true);

		virtual void Log( CVarManagerWrapper *cvarManager );

		inline float GetDiff();

		inline void SetDiffSStream( std::stringstream &stringStream );
		inline void SetCurrentSStream( std::stringstream &stringStream );
	};
}

inline float ssp::MMR::GetDiff()
{
	return current - initial;
}

inline void ssp::MMR::SetDiffSStream(std::stringstream &stringStream)
{
	stringStream << ( GetDiff() > 0 ? "+" : "") << std::setprecision( 2 ) << std::showpoint << std::fixed << GetDiff();
}

inline void ssp::MMR::SetCurrentSStream( std::stringstream &stringStream )
{
	stringStream << std::setprecision( 2 ) << std::showpoint << std::fixed << current;
}
