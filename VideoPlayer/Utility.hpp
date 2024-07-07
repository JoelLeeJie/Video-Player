/*
	File Name: Utility.hpp

	Brief: Declares various utility functions, like tracking delta time.
*/

#ifndef UTILITY_HPP
#define UTILITY_HPP
namespace Utility
{
	//REQUIRES UpdateDeltaTime to be called every frame. Tracks time between frames.
	extern double deltaTime;

	

	/*------------------------
	Functions
	*/

	/*
		Updates deltaTime.
	*/
	void UpdateDeltaTime();
}

#endif