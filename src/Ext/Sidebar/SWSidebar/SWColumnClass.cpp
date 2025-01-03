#include "SWColumnClass.h"
#include "SWSidebarClass.h"

#include <Ext/SWType/Body.h>
#include <Ext/Side/Body.h>

SWColumnClass::SWColumnClass(unsigned int id, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, static_cast<GadgetFlag>(0), true)
{
	auto& columns = SWSidebarClass::Instance.Columns;
	columns.emplace_back(this);

	this->MaxButtons = Phobos::UI::SuperWeaponSidebar_Max - (static_cast<int>(columns.size()) - 1);
}

bool SWColumnClass::Draw(bool forced)
{
	if (!SWSidebarClass::IsEnabled())
		return false;

	const auto pSurface = DSurface::Composite();
	auto bounds = pSurface->GetRect();

	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array->Items[ScenarioClass::Instance->PlayerSideIndex]);

	if (const auto centerShape = pSideExt->SuperWeaponSidebar_CenterShape.Get())
	{
		for (const auto button : this->Buttons)
		{
			Point2D drawPoint = { this->X, button->Y };
			pSurface->DrawSHP(FileSystem::SIDEBAR_PAL, centerShape, 0, &drawPoint, &bounds, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
		}
	}

	if (const auto topShape = pSideExt->SuperWeaponSidebar_TopShape.Get())
	{
		Point2D drawPoint = { this->X, this->Y - topShape->Height };
		pSurface->DrawSHP(FileSystem::SIDEBAR_PAL, topShape, 0, &drawPoint, &bounds, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}

	if (const auto bottomShape = pSideExt->SuperWeaponSidebar_BottomShape.Get())
	{
		Point2D drawPoint = { this->X, this->Y + this->Height };
		pSurface->DrawSHP(FileSystem::SIDEBAR_PAL, bottomShape, 0, &drawPoint, &bounds, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}

	for (const auto button : this->Buttons)
		button->Draw(true);

	return true;
}

void SWColumnClass::OnMouseEnter()
{
	if (!SWSidebarClass::IsEnabled())
		return;

	SWSidebarClass::Instance.CurrentColumn = this;
}

void SWColumnClass::OnMouseLeave()
{
	SWSidebarClass::Instance.CurrentColumn = nullptr;
}

bool SWColumnClass::Clicked(DWORD* pKey, GadgetFlag flags, int x, int y, KeyModifier modifier)
{
	for (const auto button : this->Buttons)
	{
		if (button->Clicked(pKey, flags, x, y, modifier))
			return true;
	}

	return false;
}

bool SWColumnClass::AddButton(int superIdx)
{
	if (const auto pSWType = SuperWeaponTypeClass::Array->GetItemOrDefault(superIdx))
	{
		const auto pSWExt = SWTypeExt::ExtMap.Find(pSWType);

		if (!pSWExt->SW_ShowCameo)
			return true;

		if (!Phobos::UI::SuperWeaponSidebar)
			return false;

		if (!pSWExt->SuperWeaponSidebar_Allow)
			return false;

		const unsigned int ownerBits = 1u << HouseClass::CurrentPlayer->Type->ArrayIndex;

		if ((pSWExt->SuperWeaponSidebar_RequiredHouses & ownerBits) == 0)
			return false;
	}
	else
	{
		return true;
	}

	auto& buttons = this->Buttons;

	if (static_cast<int>(buttons.size()) >= this->MaxButtons && !SWSidebarClass::Instance.AddColumn())
		return false;

	const int cameoWidth = 60;
	const auto button = DLLCreate<SWButtonClass>(SWButtonClass::StartID + superIdx, superIdx, 0, 0, cameoWidth, Phobos::UI::SuperWeaponSidebar_CameoHeight);

	if (!button)
		return false;

	button->Zap();
	GScreenClass::Instance->AddButton(button);
	SWSidebarClass::Instance.SortButtons();
	return true;
}

bool SWColumnClass::RemoveButton(int superIdx)
{
	auto& buttons = this->Buttons;

	const auto it = std::find_if(buttons.begin(), buttons.end(), [superIdx](SWButtonClass* const button) { return button->SuperIndex == superIdx; });

	if (it == buttons.end())
		return false;

	AnnounceInvalidPointer(SWSidebarClass::Instance.CurrentButton, *it);
	GScreenClass::Instance->RemoveButton(*it);

	DLLDelete(*it);
	buttons.erase(it);
	SWSidebarClass::Instance.SortButtons();
	return true;
}

void SWColumnClass::ClearButtons(bool remove)
{
	auto& buttons = this->Buttons;

	if (remove)
	{
		for (const auto button : buttons)
			GScreenClass::Instance->RemoveButton(button);
	}

	buttons.clear();
}

void SWColumnClass::SetHeight(int height)
{
	this->Height = height;
}