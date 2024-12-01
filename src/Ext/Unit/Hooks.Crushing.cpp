#include <DriveLocomotionClass.h>
#include <ShipLocomotionClass.h>
#include <UnitClass.h>
#include <OverlayTypeClass.h>

#include <Ext/TechnoType/Body.h>
#include <Utilities/Macro.h>
#include <Utilities/TemplateDef.h>

DEFINE_HOOK(0x073B05B, UnitClass_PerCellProcess_TiltWhenCrushes, 0x6)
{
	enum { SkipGameCode = 0x73B074 };

	GET(UnitClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->TiltsWhenCrushes_Overlays.Get(pThis->Type->TiltsWhenCrushes))
		return SkipGameCode;

	pThis->RockingForwardsPerFrame += static_cast<float>(pTypeExt->CrushOverlayExtraForwardTilt);

	return SkipGameCode;
}

DEFINE_HOOK(0x0741941, UnitClass_OverrunSquare_TiltWhenCrushes, 0x6)
{
	enum { SkipGameCode = 0x74195E };

	GET(UnitClass*, pThis, EDI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->TiltsWhenCrushes_Vehicles.Get(pThis->Type->TiltsWhenCrushes))
		return SkipGameCode;

	pThis->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get(-0.050000001));

	return SkipGameCode;
}

DEFINE_HOOK(0x4B1150, DriveLocomotionClass_WhileMoving_CrushSlowdown, 0x9)
{
	enum { SkipGameCode = 0x4B116B };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->LinkedTo->GetTechnoType());
	auto slowdownCoefficient = pThis->movementspeed_50;

	if (slowdownCoefficient > pTypeExt->CrushSlowdownMultiplier)
		slowdownCoefficient = pTypeExt->CrushSlowdownMultiplier;

	__asm { fld slowdownCoefficient };

	return SkipGameCode;

}

DEFINE_HOOK(0x5F6CE0, FootClass_CanGetCrushed_Hook, 6)
{
	enum { CanCrush = 0x5F6D85, CannotCrush = 0x5F6D8C };

	GET(FootClass* const, pCrusher, EDI);
	GET(FootClass* const, pVictim, ESI);

	// An eligible crusher must be a unit with "Crusher=yes".
	// An eligible victim must be either an infantry, a unit, or an overlay.
	// Otherwise, fallback to unmodded behavior.
	if (RulesExt::Global()->CrusherLevelEnabled &&
		pCrusher && pCrusher->GetTechnoType()->Crusher && abstract_cast<UnitTypeClass*>(pCrusher->GetTechnoType()) &&
		pVictim && (abstract_cast<InfantryTypeClass*>(pVictim->GetTechnoType()) ||
			abstract_cast<UnitTypeClass*>(pVictim->GetTechnoType()) ||
			abstract_cast<OverlayTypeClass*>(pVictim->GetTechnoType())))
	{
		auto pCrusherExt = TechnoTypeExt::ExtMap.Find(pCrusher->GetTechnoType());
		auto pVictimExt = TechnoTypeExt::ExtMap.Find(pVictim->GetTechnoType());
		int crusherLevel = pCrusherExt->GetCrusherLevel(pCrusher);
		int crushableLevel = pVictimExt->GetCrushableLevel(pVictim);
		return crusherLevel > crushableLevel ? CanCrush : CannotCrush;
	}

	return 0;
}

DEFINE_HOOK(0x7418AA, UnitClass_CrushCell_WhenCrushed, 6)
{
	GET(UnitClass* const, pThis, EDI);
	GET(ObjectClass* const, pVictim, ESI);

	if (auto const pVictimTechno = abstract_cast<TechnoClass*>(pVictim))
	{
		auto pVictimExt = TechnoTypeExt::ExtMap.Find(pVictim->GetTechnoType());
		pVictimExt->WhenCrushedBy(pThis, pVictimTechno);
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x4B1A4B, DriveLocomotionClass_WhileMoving_CrushTilt, 0xD)
DEFINE_HOOK(0x4B19F7, DriveLocomotionClass_WhileMoving_CrushTilt, 0xD)
{
	enum { SkipGameCode1 = 0x4B1A04, SkipGameCode2 = 0x4B1A58 };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pLinkedTo = pThis->LinkedTo;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());
	pLinkedTo->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get(-0.050000001));

	return R->Origin() == 0x4B19F7 ? SkipGameCode1 : SkipGameCode2;
}

DEFINE_HOOK(0x6A0813, ShipLocomotionClass_WhileMoving_CrushSlowdown, 0xB)
{
	enum { SkipGameCode = 0x6A082E };

	GET(ShipLocomotionClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->LinkedTo->GetTechnoType());
	auto slowdownCoefficient = pThis->movementspeed_50;

	if (slowdownCoefficient > pTypeExt->CrushSlowdownMultiplier)
		slowdownCoefficient = pTypeExt->CrushSlowdownMultiplier;

	__asm { fld slowdownCoefficient };

	return SkipGameCode;
}

DEFINE_HOOK(0x6A108D, ShipLocomotionClass_WhileMoving_CrushTilt, 0xD)
{
	enum { SkipGameCode = 0x6A109A };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pLinkedTo = pThis->LinkedTo;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());
	pLinkedTo->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get(-0.02));

	return SkipGameCode;
}
