#include "EventTypeClass.h"

template<>

const char* Enumerable<EventTypeClass>::GetMainSection()
{
	return "EventTypes";
}

EventTypeClass* EventTypeClass::WhenCreated = nullptr;
EventTypeClass* EventTypeClass::WhenCaptured = nullptr;
EventTypeClass* EventTypeClass::WhenCrush = nullptr;
EventTypeClass* EventTypeClass::WhenCrushed = nullptr;
EventTypeClass* EventTypeClass::WhenInfiltrate = nullptr;
EventTypeClass* EventTypeClass::WhenInfiltrated = nullptr;
EventTypeClass* EventTypeClass::BeforeLoad = nullptr;
EventTypeClass* EventTypeClass::AfterLoad = nullptr;
EventTypeClass* EventTypeClass::WhenUnload = nullptr;
EventTypeClass* EventTypeClass::BeforeBoard = nullptr;
EventTypeClass* EventTypeClass::AfterBoard = nullptr;
EventTypeClass* EventTypeClass::WhenUnboard = nullptr;

void EventTypeClass::AddDefaults()
{
	EventTypeClass::WhenCreated = FindOrAllocate("WhenCreated");
	EventTypeClass::WhenCaptured = FindOrAllocate("WhenCaptured");
	EventTypeClass::WhenCrush = FindOrAllocate("WhenCrush");
	EventTypeClass::WhenCrushed = FindOrAllocate("WhenCrushed");
	EventTypeClass::WhenInfiltrate = FindOrAllocate("WhenInfiltrate");
	EventTypeClass::WhenInfiltrated = FindOrAllocate("WhenInfiltrated");
	EventTypeClass::BeforeLoad = FindOrAllocate("BeforeLoad");
	EventTypeClass::AfterLoad = FindOrAllocate("AfterLoad");
	EventTypeClass::WhenUnload = FindOrAllocate("WhenUnload");
	EventTypeClass::BeforeBoard = FindOrAllocate("BeforeBoard");
	EventTypeClass::AfterBoard = FindOrAllocate("AfterBoard");
	EventTypeClass::WhenUnboard = FindOrAllocate("WhenUnboard");
}

void EventTypeClass::LoadFromINI(CCINIClass* pINI)
{
}

template <typename T>
void EventTypeClass::Serialize(T& Stm)
{
}

void EventTypeClass::LoadFromStream(PhobosStreamReader& Stm)
{
	this->Serialize(Stm);
}

void EventTypeClass::SaveToStream(PhobosStreamWriter& Stm)
{
	this->Serialize(Stm);
}