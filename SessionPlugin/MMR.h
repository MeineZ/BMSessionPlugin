#pragma once

#include <sstream>
#include <string>
#include <iomanip>

#include <bakkesmod/plugin/bakkesmodsdk.h>
#include <bakkesmod/wrappers/wrapperstructs.h>
#include <bakkesmod/wrappers/UniqueIDWrapper.h>

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
		float initial;
		float current;

		float lastDiff;

		float streakMmrGain;

		MMR( float initialMmr );

		void Reset( float initialMmr = 0.0f);

		bool RequestMmrUpdate(GameWrapper * gameWrapper, UniqueIDWrapper &uniqueID, const ssp::playlist::Type matchType, bool force = true);

		virtual void Log( CVarManagerWrapper *cvarManager );

		inline float GetDiff();

		inline void SetDiffSStream( std::stringstream &stringStream );
		inline void SetCurrentSStream( std::stringstream &stringStream );
		inline void SetLastGameSStream( std::stringstream &stringStream );

	private:
		void SetStreakMMRGain();
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

inline void ssp::MMR::SetLastGameSStream( std::stringstream &stringStream )
{
	stringStream << ( lastDiff > 0 ? "+" : "" ) << std::setprecision( 2 ) << std::showpoint << std::fixed << lastDiff;
}

inline void ssp::MMR::SetCurrentSStream( std::stringstream &stringStream )
{
	stringStream << std::setprecision( 2 ) << std::showpoint << std::fixed << current;
}
