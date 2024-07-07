/*
	File Name: Utility.cpp

	Brief: Defines various utility functions, like tracking delta time.
*/
#include "Utility.hpp"
#include <chrono>

namespace Utility
{
	/*-------------------
	Shared Variables */

	double deltaTime = 0;

	/*
		Updates deltaTime.
	*/
	void UpdateDeltaTime()
	{
		//Stores time of last frame, and compares difference with time of current frame.
		static std::chrono::system_clock::time_point lastCalledTime = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - lastCalledTime;
		lastCalledTime = std::chrono::system_clock::now();
		deltaTime = elapsed_seconds.count();
	}
}
