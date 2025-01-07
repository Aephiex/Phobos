#pragma once

#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>
#include "EventTypeClass.h"
#include "Affiliated/HandlerCompClass.h"

class EventHandlerTypeClass final : public Enumerable<EventHandlerTypeClass>
{
public:
	Valueable<bool> loaded;
	ValueableVector<EventTypeClass*> EventTypes;
	std::vector<std::unique_ptr<HandlerCompClass>> HandlerComps;

	EventHandlerTypeClass(const char* pTitle = NONE_STR) : Enumerable<EventHandlerTypeClass>(pTitle)
		, loaded { false }
		, EventTypes {}
		, HandlerComps {}
	{};

	void LoadFromINI(CCINIClass* pINI);
	void LoadFromINI(INI_EX& exINI);
	void LoadForScope(INI_EX& exINI, const char* pSection, const EventScopeType scopeType, const char* scopeName);
	void LoadForExtendedScope(INI_EX& exINI, const char* pSection, const EventScopeType scopeType, const EventExtendedScopeType extendedScopeType, const char* scopeName, const char* extendedScopeName);
	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);

	static void LoadTypeListFromINI(INI_EX& exINI, const char* pSection, const char* pHeader, ValueableVector<EventHandlerTypeClass*>* vec);
	static void LoadTypeMapFromINI(INI_EX& exINI, const char* pSection, const char* pHeader, PhobosMap<EventTypeClass*, std::vector<EventHandlerTypeClass*>>* map);

	void HandleEvent(std::map<EventScopeType, TechnoClass*>* pParticipants);
private:
	template <typename T>
	void Serialize(T& Stm);
	void LoadFromINIPrivate(INI_EX& exINI, const char* pSection);

	bool CheckFilters(std::map<EventScopeType, TechnoClass*>* pParticipants, EventScopeType scopeType) const;
	void ExecuteEffects(std::map<EventScopeType, TechnoClass*>* pParticipants, EventScopeType scopeType) const;
};