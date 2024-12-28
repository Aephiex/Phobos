#pragma once

#include <Networking.h>
#include <vector>

class PhobosNetEvent
{
public:
	enum class Events : unsigned char
	{
		ToggleAggressiveStance = 0x80,

		First = ToggleAggressiveStance,
		Last = ToggleAggressiveStance
	};

	class Handlers
	{
	public:
		static void RaiseToggleAggressiveStance(TechnoClass* pTechno);
		static void RespondToToggleAggressiveStance(NetworkEvent* Event);
	};
};