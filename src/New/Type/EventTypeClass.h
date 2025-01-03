#pragma once
#include <set>
#include <unordered_map>

#include <Utilities/Enumerable.h>
#include <Utilities/TemplateDef.h>

class EventTypeClass final : public Enumerable<EventTypeClass>
{
public:

	EventTypeClass(const char* pTitle = NONE_STR) : Enumerable<EventTypeClass>(pTitle)
	{ }

	static void AddDefaults();

	// public static pointers of event types invoked by Phobos
	static EventTypeClass* WhenCreated;
	static EventTypeClass* WhenCaptured;
	static EventTypeClass* WhenCrush;
	static EventTypeClass* WhenCrushed;
	static EventTypeClass* WhenInfiltrate;
	static EventTypeClass* WhenInfiltrated;
	static EventTypeClass* BeforeLoad;
	static EventTypeClass* AfterLoad;
	static EventTypeClass* WhenUnload;
	static EventTypeClass* BeforeBoard;
	static EventTypeClass* AfterBoard;
	static EventTypeClass* WhenUnboard;

	void LoadFromINI(CCINIClass* pINI);
	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};