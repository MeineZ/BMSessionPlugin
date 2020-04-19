#pragma once

#include <sstream>
#include <string>
#include <iomanip>

namespace ssp // SessionPlugin
{
	struct MMR
	{
		float initial = 0;
		float current = 0;

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
