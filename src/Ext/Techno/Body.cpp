#include "Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/House/Body.h>
#include <Utilities/AresFunctions.h>

TechnoExt::ExtContainer TechnoExt::ExtMap;

TechnoExt::ExtData::~ExtData()
{
	auto const pTypeExt = this->TypeExtData;
	auto const pType = pTypeExt->OwnerObject();
	auto pThis = this->OwnerObject();

	if (pTypeExt->AutoDeath_Behavior.isset())
	{
		auto& vec = ScenarioExt::Global()->AutoDeathObjects;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (RulesExt::Global()->ExpandBuildingPlace && pThis->WhatAmI() == AbstractType::Unit && pType->DeploysInto)
	{
		auto& vec = HouseExt::ExtMap.Find(pThis->Owner)->OwnedDeployingUnits;
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());
	}

	if (pThis->WhatAmI() != AbstractType::Aircraft && pThis->WhatAmI() != AbstractType::Building
		&& pType->Ammo > 0 && pTypeExt->ReloadInTransport)
	{
		auto& vec = ScenarioExt::Global()->TransportReloaders;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	AnimExt::InvalidateTechnoPointers(pThis);
}

bool TechnoExt::IsActive(TechnoClass* pThis)
{
	return pThis
		&& pThis->IsAlive
		&& pThis->Health > 0
		&& !pThis->InLimbo
		&& !pThis->TemporalTargetingMe
		&& !pThis->BeingWarpedOut
		&& !pThis->IsUnderEMP()
		;
}

bool TechnoExt::IsHarvesting(TechnoClass* pThis)
{
	if (!TechnoExt::IsActive(pThis))
		return false;

	auto slave = pThis->SlaveManager;
	if (slave && slave->State != SlaveManagerStatus::Ready)
		return true;

	if (pThis->WhatAmI() == AbstractType::Building)
		return pThis->IsPowerOnline();

	if (TechnoExt::HasAvailableDock(pThis))
	{
		switch (pThis->GetCurrentMission())
		{
		case Mission::Harvest:
		case Mission::Unload:
		case Mission::Enter:
			return true;
		case Mission::Guard: // issue#603: not exactly correct, but idk how to do better
			if (auto pUnit = abstract_cast<UnitClass*>(pThis))
				return pUnit->IsHarvesting || pUnit->Locomotor->Is_Really_Moving_Now() || pUnit->HasAnyLink();
		default:
			return false;
		}
	}

	return false;
}

bool TechnoExt::HasAvailableDock(TechnoClass* pThis)
{
	for (auto pBld : pThis->GetTechnoType()->Dock)
	{
		if (pThis->Owner->CountOwnedAndPresent(pBld))
			return true;
	}

	return false;
}

// Syncs Iron Curtain or Force Shield timer to another techno.
void TechnoExt::SyncInvulnerability(TechnoClass* pFrom, TechnoClass* pTo)
{
	if (pFrom->IsIronCurtained())
	{
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pFrom->GetTechnoType());
		bool isForceShielded = pFrom->ForceShielded;
		bool allowSyncing = !isForceShielded ? pTypeExt->IronCurtain_KeptOnDeploy.Get(RulesExt::Global()->IronCurtain_KeptOnDeploy) :
			pTypeExt->ForceShield_KeptOnDeploy.Get(RulesExt::Global()->ForceShield_KeptOnDeploy);

		if (allowSyncing)
		{
			pTo->IronCurtainTimer = pFrom->IronCurtainTimer;
			pTo->IronTintStage = pFrom->IronTintStage;
			pTo->ForceShielded = isForceShielded;
		}
	}
}

double TechnoExt::GetCurrentSpeedMultiplier(FootClass* pThis)
{
	double houseMultiplier = 1.0;

	if (pThis->WhatAmI() == AbstractType::Aircraft)
		houseMultiplier = pThis->Owner->Type->SpeedAircraftMult;
	else if (pThis->WhatAmI() == AbstractType::Infantry)
		houseMultiplier = pThis->Owner->Type->SpeedInfantryMult;
	else
		houseMultiplier = pThis->Owner->Type->SpeedUnitsMult;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pThis->SpeedMultiplier * houseMultiplier * pExt->AE.SpeedMultiplier *
		(pThis->HasAbility(Ability::Faster) ? RulesClass::Instance->VeteranSpeed : 1.0);
}

CoordStruct TechnoExt::PassengerKickOutLocation(TechnoClass* pThis, FootClass* pPassenger, int maxAttempts = 1)
{
	if (!pThis || !pPassenger)
		return CoordStruct::Empty;

	if (maxAttempts < 1)
		maxAttempts = 1;

	CellClass* pCell;
	CellStruct placeCoords = CellStruct::Empty;
	auto pTypePassenger = pPassenger->GetTechnoType();
	CoordStruct finalLocation = CoordStruct::Empty;
	short extraDistanceX = 1;
	short extraDistanceY = 1;
	SpeedType speedType = pTypePassenger->SpeedType;
	MovementZone movementZone = pTypePassenger->MovementZone;

	if (pTypePassenger->WhatAmI() == AbstractType::AircraftType)
	{
		speedType = SpeedType::Track;
		movementZone = MovementZone::Normal;
	}
	do
	{
		placeCoords = pThis->GetCell()->MapCoords - CellStruct { (short)(extraDistanceX / 2), (short)(extraDistanceY / 2) };
		placeCoords = MapClass::Instance->NearByLocation(placeCoords, speedType, -1, movementZone, false, extraDistanceX, extraDistanceY, true, false, false, false, CellStruct::Empty, false, false);

		pCell = MapClass::Instance->GetCellAt(placeCoords);
		extraDistanceX += 1;
		extraDistanceY += 1;
	}
	while (extraDistanceX < maxAttempts && (pThis->IsCellOccupied(pCell, FacingType::None, -1, nullptr, false) != Move::OK) && pCell->MapCoords != CellStruct::Empty);

	pCell = MapClass::Instance->TryGetCellAt(placeCoords);
	if (pCell)
		finalLocation = pCell->GetCoordsWithBridge();

	return finalLocation;
}

bool TechnoExt::AllowedTargetByZone(TechnoClass* pThis, TechnoClass* pTarget, TargetZoneScanType zoneScanType, WeaponTypeClass* pWeapon, bool useZone, int zone)
{
	if (!pThis || !pTarget)
		return false;

	if (pThis->WhatAmI() == AbstractType::Aircraft)
		return true;

	MovementZone mZone = pThis->GetTechnoType()->MovementZone;
	int currentZone = useZone ? zone : MapClass::Instance->GetMovementZoneType(pThis->GetMapCoords(), mZone, pThis->OnBridge);

	if (currentZone != -1)
	{
		if (zoneScanType == TargetZoneScanType::Any)
			return true;

		int targetZone = MapClass::Instance->GetMovementZoneType(pTarget->GetMapCoords(), mZone, pTarget->OnBridge);

		if (zoneScanType == TargetZoneScanType::Same)
		{
			if (currentZone != targetZone)
				return false;
		}
		else
		{
			if (currentZone == targetZone)
				return true;

			auto const speedType = pThis->GetTechnoType()->SpeedType;
			auto cellStruct = MapClass::Instance->NearByLocation(CellClass::Coord2Cell(pTarget->Location),
				speedType, -1, mZone, false, 1, 1, true,
				false, false, speedType != SpeedType::Float, CellStruct::Empty, false, false);
			auto const pCell = MapClass::Instance->GetCellAt(cellStruct);

			if (!pCell)
				return false;

			double distance = pCell->GetCoordsWithBridge().DistanceFrom(pTarget->GetCenterCoords());

			if (!pWeapon)
			{
				int weaponIndex = pThis->SelectWeapon(pTarget);

				if (weaponIndex < 0)
					return false;

				pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType;
			}

			if (distance > pWeapon->Range)
				return false;
		}
	}

	return true;
}

// Feature for common usage : TechnoType conversion -- Trsdy
// BTW, who said it was merely a Type pointer replacement and he could make a better one than Ares?
bool TechnoExt::ConvertToType(FootClass* pThis, TechnoTypeClass* pToType)
{
	if (AresFunctions::ConvertTypeTo)
		return AresFunctions::ConvertTypeTo(pThis, pToType);
	// In case not using Ares 3.0. Only update necessary vanilla properties
	AbstractType rtti;
	TechnoTypeClass** nowTypePtr;

	// Different types prohibited
	switch (pThis->WhatAmI())
	{
	case AbstractType::Infantry:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<InfantryClass*>(pThis)->Type));
		rtti = AbstractType::InfantryType;
		break;
	case AbstractType::Unit:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<UnitClass*>(pThis)->Type));
		rtti = AbstractType::UnitType;
		break;
	case AbstractType::Aircraft:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<AircraftClass*>(pThis)->Type));
		rtti = AbstractType::AircraftType;
		break;
	default:
		Debug::Log("%s is not FootClass, conversion not allowed\n", pToType->get_ID());
		return false;
	}

	if (pToType->WhatAmI() != rtti)
	{
		Debug::Log("Incompatible types between %s and %s\n", pThis->get_ID(), pToType->get_ID());
		return false;
	}

	// Detach CLEG targeting
	auto tempUsing = pThis->TemporalImUsing;
	if (tempUsing && tempUsing->Target)
		tempUsing->Detach();

	HouseClass* const pOwner = pThis->Owner;

	// Remove tracking of old techno
	if (!pThis->InLimbo)
		pOwner->RegisterLoss(pThis, false);
	pOwner->RemoveTracking(pThis);

	int oldHealth = pThis->Health;

	// Generic type-conversion
	TechnoTypeClass* prevType = *nowTypePtr;
	*nowTypePtr = pToType;

	// Readjust health according to percentage
	pThis->SetHealthPercentage((double)(oldHealth) / (double)prevType->Strength);
	pThis->EstimatedHealth = pThis->Health;

	// Add tracking of new techno
	pOwner->AddTracking(pThis);
	if (!pThis->InLimbo)
		pOwner->RegisterGain(pThis, false);
	pOwner->RecheckTechTree = true;

	// Update Ares AttachEffects -- skipped
	// Ares RecalculateStats -- skipped

	// Adjust ammo
	pThis->Ammo = Math::min(pThis->Ammo, pToType->Ammo);
	// Ares ResetSpotlights -- skipped

	// Adjust ROT
	if (rtti == AbstractType::AircraftType)
		pThis->SecondaryFacing.SetROT(pToType->ROT);
	else
		pThis->PrimaryFacing.SetROT(pToType->ROT);
	// Adjust Ares TurretROT -- skipped
	//  pThis->SecondaryFacing.SetROT(TechnoTypeExt::ExtMap.Find(pToType)->TurretROT.Get(pToType->ROT));

	// Locomotor change, referenced from Ares 0.A's abduction code, not sure if correct, untested
	CLSID nowLocoID;
	ILocomotion* iloco = pThis->Locomotor;
	const auto& toLoco = pToType->Locomotor;
	if ((SUCCEEDED(static_cast<LocomotionClass*>(iloco)->GetClassID(&nowLocoID)) && nowLocoID != toLoco))
	{
		// because we are throwing away the locomotor in a split second, piggybacking
		// has to be stopped. otherwise the object might remain in a weird state.
		while (LocomotionClass::End_Piggyback(pThis->Locomotor));
		// throw away the current locomotor and instantiate
		// a new one of the default type for this unit.
		if (auto newLoco = LocomotionClass::CreateInstance(toLoco))
		{
			newLoco->Link_To_Object(pThis);
			pThis->Locomotor = std::move(newLoco);
		}
	}

	// TODO : Jumpjet locomotor special treatement, some brainfart, must be uncorrect, HELP ME!
	const auto& jjLoco = LocomotionClass::CLSIDs::Jumpjet();
	if (pToType->BalloonHover && pToType->DeployToLand && prevType->Locomotor != jjLoco && toLoco == jjLoco)
		pThis->Locomotor->Move_To(pThis->Location);

	return true;
}

// Checks if vehicle can deploy into a building at its current location. If unit has no DeploysInto set returns noDeploysIntoDefaultValue (def = false) instead.
bool TechnoExt::CanDeployIntoBuilding(UnitClass* pThis, bool noDeploysIntoDefaultValue)
{
	if (!pThis)
		return false;

	auto const pDeployType = pThis->Type->DeploysInto;

	if (!pDeployType)
		return noDeploysIntoDefaultValue;

	bool canDeploy = true;
	auto mapCoords = CellClass::Coord2Cell(pThis->GetCoords());

	if (pDeployType->GetFoundationWidth() > 2 || pDeployType->GetFoundationHeight(false) > 2)
		mapCoords += CellStruct { -1, -1 };

	pThis->Mark(MarkType::Up);

	pThis->Locomotor->Mark_All_Occupation_Bits(MarkType::Up);

	if (!pDeployType->CanCreateHere(mapCoords, pThis->Owner))
		canDeploy = false;

	pThis->Locomotor->Mark_All_Occupation_Bits(MarkType::Down);
	pThis->Mark(MarkType::Down);

	return canDeploy;
}

bool TechnoExt::IsTypeImmune(TechnoClass* pThis, TechnoClass* pSource)
{
	if (!pThis || !pSource)
		return false;

	auto const pType = pThis->GetTechnoType();

	if (!pType->TypeImmune)
		return false;

	if (pType == pSource->GetTechnoType() && pThis->Owner == pSource->Owner)
		return true;

	return false;
}

/// <summary>
/// Gets whether or not techno has listed AttachEffect types active on it
/// </summary>
/// <param name="attachEffectTypes">Attacheffect types.</param>
/// <param name="requireAll">Whether or not to require all listed types to be present or if only one will satisfy the check.</param>
/// <param name="ignoreSameSource">Ignore AttachEffects that come from set invoker and source.</param>
/// <param name="pInvoker">Invoker Techno used for same source check.</param>
/// <param name="pSource">Source AbstractClass instance used for same source check.</param>
/// <returns>True if techno has active AttachEffects that satisfy the source, false if not.</returns>
bool TechnoExt::ExtData::HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource,
	TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const
{
	unsigned int foundCount = 0;
	unsigned int typeCounter = 1;

	for (auto const& type : attachEffectTypes)
	{
		for (auto const& attachEffect : this->AttachedEffects)
		{
			if (attachEffect->GetType() == type && attachEffect->IsActive())
			{
				if (ignoreSameSource && pInvoker && pSource && attachEffect->IsFromSource(pInvoker, pSource))
					continue;

				unsigned int minSize = minCounts ? minCounts->size() : 0;
				unsigned int maxSize = maxCounts ? maxCounts->size() : 0;

				if (type->Cumulative && (minSize > 0 || maxSize > 0))
				{
					int cumulativeCount = this->GetAttachedEffectCumulativeCount(type, ignoreSameSource, pInvoker, pSource);

					if (minSize > 0)
					{
						if (cumulativeCount < minCounts->at(typeCounter - 1 >= minSize ? minSize - 1 : typeCounter - 1))
							continue;
					}
					if (maxSize > 0)
					{
						if (cumulativeCount > maxCounts->at(typeCounter - 1 >= maxSize ? maxSize - 1 : typeCounter - 1))
							continue;
					}
				}

				// Only need to find one match, can stop here.
				if (!requireAll)
					return true;

				foundCount++;
				break;
			}
		}

		// One of the required types was not found, can stop here.
		if (requireAll && foundCount < typeCounter)
			return false;

		typeCounter++;
	}

	if (requireAll && foundCount == attachEffectTypes.size())
		return true;

	return false;
}

/// <summary>
/// Gets how many counts of same cumulative AttachEffect type instance techno has active on it.
/// </summary>
/// <param name="pAttachEffectType">AttachEffect type.</param>
/// <param name="ignoreSameSource">Ignore AttachEffects that come from set invoker and source.</param>
/// <param name="pInvoker">Invoker Techno used for same source check.</param>
/// <param name="pSource">Source AbstractClass instance used for same source check.</param>
/// <returns>Number of active cumulative AttachEffect type instances on the techno. 0 if the AttachEffect type is not cumulative.</returns>
int TechnoExt::ExtData::GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource) const
{
	if (!pAttachEffectType->Cumulative)
		return 0;

	unsigned int foundCount = 0;

	for (auto const& attachEffect : this->AttachedEffects)
	{
		if (attachEffect->GetType() == pAttachEffectType && attachEffect->IsActive())
		{
			if (ignoreSameSource && pInvoker && pSource && attachEffect->IsFromSource(pInvoker, pSource))
				continue;

			foundCount++;
		}
	}

	return foundCount;
}

int TechnoExt::ExtData::GetCrusherLevel() const
{
	auto const pTypeExt = this->TypeExtData;
	if (!pTypeExt->CrusherLevel.isset())
	{
		auto const pType = pTypeExt->OwnerObject();
		// `CrusherLevel` is not set, we take a default value based on `Crusher` and `OmniCrusher`.
		if (pType->Crusher)
		{
			if (pType->OmniCrusher)
				pTypeExt->CrusherLevel = RulesExt::Global()->CrusherLevel_Defaults_OmniCrusher;
			else
				pTypeExt->CrusherLevel = RulesExt::Global()->CrusherLevel_Defaults_Crusher;
		}
		else
		{
			pTypeExt->CrusherLevel = 0;
		}
	}

	return pTypeExt->CrusherLevel.Get();
}

int TechnoExt::ExtData::GetCrushableLevel() const
{
	auto const pTypeExt = this->TypeExtData;
	if (!pTypeExt->CrushableLevel.isset())
	{
		auto const pType = pTypeExt->OwnerObject();
		// `CrushableLevel` is not set, we take a default value based on `Crushable`, `OmniCrushResistant`, and the techno's type.
		if (pType->Crushable)
		{
			pTypeExt->CrushableLevel = 0;
		}
		else if (pType->OmniCrushResistant)
		{
			pTypeExt->CrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_OmniCrushResistant;
		}
		else
		{
			// The techno types are infantry types, unit types, aircraft types, and building types.
			// Overlay types are not techno types.
			switch (pType->WhatAmI())
			{
			case AbstractType::InfantryType:
				pTypeExt->CrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_Uncrushable_Infantry;
				break;
			case AbstractType::UnitType:
			case AbstractType::AircraftType:
				pTypeExt->CrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_Uncrushable_Unit;
				break;
			case AbstractType::BuildingType:
				pTypeExt->CrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_Uncrushable_Building;
				break;
			}
		}

		// `CrushableLevel` is not set, we take a default value for `DeployedCrushableLevel` based on `DeployedCrushable` and `OmniCrushResistant`.
		// To avoid complexity of all possible combos, we ignore explicit `DeployedCrushableLevel` if `CrushableLevel` is not set.
		if (auto pInfantryType = abstract_cast<InfantryTypeClass*>(pType))
		{
			// We only consider case 1, since in any other case, the infantry's
			// default `DeployedCrushableLevel` will be equal to its default `CrushableLevel`.
			// 1. `Crushable=true`, `DeployedCrushable=false`: uncrushable only when deployed;
			// 2. `Crushable=false`, `DeployedCrushable=true`: uncrushable always.
			if (pInfantryType->Crushable && !pInfantryType->DeployedCrushable)
			{
				if (pInfantryType->OmniCrushResistant)
					pTypeExt->DeployedCrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_OmniCrushResistant;
				else
					pTypeExt->DeployedCrushableLevel = RulesExt::Global()->CrushableLevel_Defaults_Uncrushable_Infantry;
			}
			else
			{
				pTypeExt->DeployedCrushableLevel = pTypeExt->CrushableLevel;
			}
		}
	}

	if (auto pVictimInfantry = abstract_cast<InfantryClass*>(this->OwnerObject()))
	{
		if (pVictimInfantry->IsDeployed())
			return this->TypeExtData->DeployedCrushableLevel.Get(this->TypeExtData->CrushableLevel.Get(0));
	}
	return this->TypeExtData->CrushableLevel.Get(0);
}

// Checks if this transport can load a passenger.
// Note that this function only checks the size limit and Ares passenger whitelist and blacklist,
// it doesn't check if this transport is actually a transport or not.
bool TechnoExt::ExtData::CanLoadPassenger(TechnoClass* pPassenger) const
{
	auto const pTransport = this->OwnerObject();
	auto const pTransportTypeExt = this->TypeExtData;
	auto const pTransportType = pTransportTypeExt->OwnerObject();
	auto const pPassengerType = pPassenger->GetTechnoType();
	auto const sizeLimit = static_cast<int>(pTransportType->SizeLimit);
	auto const size = static_cast<int>(pPassengerType->Size);
	return (sizeLimit <= 0 || sizeLimit >= size)
		&& (!pTransportTypeExt->Passengers_BySize || (pTransport->Passengers.GetTotalSize() + size) <= pTransportType->Passengers)
		&& (pTransportTypeExt->PassengersWhitelist.empty() || pTransportTypeExt->PassengersWhitelist.Contains(pPassengerType))
		&& !pTransportTypeExt->PassengersBlacklist.Contains(pPassengerType);
}

// Checks if this transport can load any of the passengers inside a vector.
// Note that this function only checks the size limit and Ares passenger whitelist and blacklist,
// it doesn't check if this transport is actually a transport or not.
bool TechnoExt::ExtData::CanLoadAny(std::vector<TechnoClass*> pPassengerList) const
{
	for (auto pPassenger : pPassengerList)
		if (this->CanLoadPassenger(pPassenger))
			return true;
	return false;
}

// Checks if a transport can load any of the passengers inside a map.
bool TechnoExt::ExtData::CanLoadAny(std::map<int, std::vector<TechnoClass*>> passengerMap) const
{
	auto const pTransportTypeExt = this->TypeExtData;
	auto const pTransportType = pTransportTypeExt->OwnerObject();
	auto const sizeLimit = static_cast<int>(pTransportType->SizeLimit);
	for (auto const& passengerMapPair : passengerMap)
	{
		auto const passengerSize = passengerMapPair.first;
		// there is no passenger small enough it can load, or the passenger list of this size is somehow empty
		if ((sizeLimit > 0 && passengerSize > sizeLimit) || passengerMapPair.second.empty())
			continue;
		// then check the function for list
		if (this->CanLoadAny(passengerMapPair.second))
			return true;
	}
	return false;
}

// Ares 0.2 source
// Checks if a garrisonable structure can garrison a unit.
// It is assumed that "whom" is an infantry.
// This function only checks about the Ares occupier whitelist, it does not check if this is a garrisonable structure or not.
bool TechnoExt::ExtData::CanBeOccupiedBy(TechnoClass* whom) const
{
	return this->TypeExtData->AllowedOccupiers.empty()
		|| this->TypeExtData->AllowedOccupiers.Contains(whom->GetTechnoType());
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->Shield)
		.Process(this->LaserTrails)
		.Process(this->AttachedEffects)
		.Process(this->AE)
		.Process(this->ReceiveDamage)
		.Process(this->PassengerDeletionTimer)
		.Process(this->CurrentShieldType)
		.Process(this->LastWarpDistance)
		.Process(this->ChargeTurretTimer)
		.Process(this->AutoDeathTimer)
		.Process(this->MindControlRingAnimType)
		.Process(this->Strafe_BombsDroppedThisRound)
		.Process(this->CurrentAircraftWeaponIndex)
		.Process(this->IsInTunnel)
		.Process(this->IsBurrowed)
		.Process(this->HasBeenPlacedOnMap)
		.Process(this->DeployFireTimer)
		.Process(this->SkipTargetChangeResetSequence)
		.Process(this->ForceFullRearmDelay)
		.Process(this->LastRearmWasFullDelay)
		.Process(this->CanCloakDuringRearm)
		.Process(this->WHAnimRemainingCreationInterval)
		.Process(this->UnitIdleIsSelected)
		.Process(this->UnitIdleActionTimer)
		.Process(this->UnitIdleActionGapTimer)
		.Process(this->FiringObstacleCell)
		.Process(this->UnitAutoDeployTimer)
		.Process(this->IsDetachingForCloak)
		.Process(this->LastBeControlledFrame)
		.Process(this->OriginalPassengerOwner)
		.Process(this->HasRemainingWarpInDelay)
		.Process(this->LastWarpInDelay)
		.Process(this->IsBeingChronoSphered)
		.Process(this->AggressiveStance)
		.Process(this->HousingMe)
		.Process(this->WhenCreatedEventFired)
		;
}

void TechnoExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::InitAggressiveStance()
{
	this->AggressiveStance = this->TypeExtData->AggressiveStance.Get();
}

bool TechnoExt::ExtData::GetAggressiveStance() const
{
	// if this is a passenger then obey the configuration of the transport
	if (auto pTransport = this->OwnerObject()->Transporter)
	{
		auto pTransportExt = TechnoExt::ExtMap.Find(pTransport);
		return pTransportExt->GetAggressiveStance();
	}

	return this->AggressiveStance;
}

void TechnoExt::ExtData::ToggleAggressiveStance()
{
	
	if (this->AggressiveStance)
	{
		// toggle off aggressive stance
		this->AggressiveStance = false;
		// stop current target
		this->OwnerObject()->QueueMission(Mission::Guard, true);
		this->OwnerObject()->SetTarget(nullptr);
	}
	else
	{
		// toggle on aggressive stance
		this->AggressiveStance = true;
	}
}

bool TechnoExt::ExtData::CanToggleAggressiveStance()
{
	if (!this->TypeExtData->AggressiveStance_Togglable.isset())
	{
		// Only techno that are armed and open-topped can be aggressive stance.
		if (!(this->OwnerObject()->IsArmed() || this->TypeExtData->OwnerObject()->OpenTopped))
		{
			this->TypeExtData->AggressiveStance_Togglable = false;
			return false;
		}

		// Engineers and Agents are default to not allow aggressive stance.
		if (auto pInfantryTypeClass = abstract_cast<InfantryTypeClass*>(this->TypeExtData->OwnerObject()))
		{
			if (pInfantryTypeClass->Engineer || pInfantryTypeClass->Agent)
			{
				this->TypeExtData->AggressiveStance_Togglable = false;
				return false;
			}
		}

		this->TypeExtData->AggressiveStance_Togglable = true;
		return true;
	}

	return this->TypeExtData->AggressiveStance_Togglable.Get(true);
}

bool TechnoExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") { }

TechnoExt::ExtContainer::~ExtContainer() = default;


// =============================
// container hooks

DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
{
	GET(TechnoClass*, pItem, ESI);

	TechnoExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	TechnoExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TechnoClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
{
	TechnoExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
{
	TechnoExt::ExtMap.SaveStatic();

	return 0;
}
