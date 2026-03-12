#include "Hooks.h"
#include "../Data/Data.h"
#include "../Utilities/Utilities.h"
#include "RE/B/BSAnimationGraphEvent.h"

namespace Hooks
{
	MenuWatcher* MenuWatcherInstance;

	RE::PlayerCharacter* Player;
	RE::TESGlobal* WZ_IsDoingIdle;
	RE::TESGlobal* WZ_PlatesHealth;
	RE::TESGlobal* WZ_PlatesMaxHealth;
	RE::TESGlobal* WZ_PlatesHealthToAdd;
	RE::TESGlobal* WZ_PlatesProtectHealth;
	RE::TESGlobal* WZ_PlatesProtectLimbs;
	RE::TESGlobal* WZ_PlatesIgnoreArmor;
	RE::TESGlobal* WZ_PlatesLoot_Chance;
	RE::TESGlobal* WZ_PlatesLoot_MaxQuantity;
	RE::TESGlobal* WZ_PlatesLoot_MinQuantity;
	RE::AlchemyItem* WZ_PlatesBreakPotion;
	RE::AlchemyItem* WZ_PlatesTakeDamagePotion;
	RE::TESObjectMISC* WZ_Plates;
	RE::BGSPerk* PowerArmorPerk;

	static uintptr_t ProcessHitDataOriginal;

	void ProcessPlates(RE::Actor* actor, RE::HitData& hitData)
	{
		if (hitData.target->formID != 0x14) {
			return;
		}

		if (WZ_PlatesHealth->value > WZ_PlatesMaxHealth->value) {
			WZ_PlatesHealth->value = WZ_PlatesMaxHealth->value;
		}

		float startingPlatesHealth = WZ_PlatesHealth->value;
		float newPlatesHealth = startingPlatesHealth;

		/*
		REX::INFO("FORMID: [{}]", actor->formID);
		REX::INFO("Plates Health: [{}]", WZ_PlatesHealth->value);
		REX::INFO("Total Damage: [{}]", hitData.totalDamage);
		REX::INFO("Health Damage: [{}]", hitData.healthDamage);
		REX::INFO("Physical Damage: [{}]", hitData.physicalDamage);
		REX::INFO("Limb Damage: [{}]", hitData.targetedLimbDamage);
		REX::INFO("Resisted Physical Damage: [{}]", hitData.resistedPhysicalDamage);
		REX::INFO("Resisted Typed Damage: [{}]", hitData.resistedTypedDamage);
		*/

		if (startingPlatesHealth <= 0.0f || actor->GetPerkRank(PowerArmorPerk) != 0) {
			return;
		}

		if (WZ_PlatesIgnoreArmor->value > 0.0f && (WZ_PlatesProtectHealth->value > 0.0f || WZ_PlatesProtectLimbs->value > 0.0f)) {
			float calculatedResistedDamage = hitData.resistedPhysicalDamage + hitData.resistedTypedDamage;
			newPlatesHealth = Utilities::Max(0.0f, WZ_PlatesHealth->value - calculatedResistedDamage);
			WZ_PlatesHealth->value = newPlatesHealth;
		}

		if (WZ_PlatesProtectHealth->value > 0.0f) {
			float newHealthDamage = Utilities::Max(0.0f, hitData.healthDamage - WZ_PlatesHealth->value);
			newPlatesHealth = Utilities::Max(0.0f, WZ_PlatesHealth->value - hitData.healthDamage);
			WZ_PlatesHealth->value = newPlatesHealth;
			hitData.healthDamage = newHealthDamage;
		}

		if (WZ_PlatesProtectLimbs->value > 0.0f) {
			float newLimbDamage = Utilities::Max(0.0f, hitData.targetedLimbDamage - WZ_PlatesHealth->value);
			newPlatesHealth = Utilities::Max(0.0f, WZ_PlatesHealth->value - hitData.targetedLimbDamage);
			WZ_PlatesHealth->value = newPlatesHealth;
			hitData.targetedLimbDamage = newLimbDamage;
		}

		if (newPlatesHealth == 0.0f) {
			Player->DrinkPotion(WZ_PlatesBreakPotion, 0);
		}

		if (newPlatesHealth > 0.0f && newPlatesHealth < startingPlatesHealth) {
			Player->DrinkPotion(WZ_PlatesTakeDamagePotion, 0);
		}
	}

	void ProcessPlatesLoot(RE::Actor* actor, RE::HitData& hitData)
	{
		RE::Actor* player = nullptr;
		RE::Actor* targetToReceiveLoot = nullptr;

		if (hitData.target->formID == 0x14 && hitData.aggressor && hitData.aggressor->formID != 0x14) {
			targetToReceiveLoot = hitData.aggressor->As<RE::Actor>();
			player = hitData.target->As<RE::Actor>();
		}

		if (hitData.aggressor && hitData.aggressor->formID == 0x14 && hitData.target->formID != 0x14) {
			targetToReceiveLoot = hitData.target->As<RE::Actor>();
			player = hitData.aggressor->As<RE::Actor>();
		}

		if (!player || !targetToReceiveLoot || WZ_PlatesLoot_Chance->value <= 0.0f) {
			return;
		}

		REX::INFO("Target to receive loot name: [{}]", targetToReceiveLoot->GetDisplayFullName());

		bool isHostile = targetToReceiveLoot->GetHostileToActor(player);
		uint32_t platesInInventory = targetToReceiveLoot->GetInventoryObjectCount(WZ_Plates);

		if (!isHostile || platesInInventory > 0) {
			return;
		}

		REX::INFO("Target has no plates in inventory, loot chance will be rolled.");

		int chance = static_cast<int>(WZ_PlatesLoot_Chance->value);
		bool rolledSuccessfully = Utilities::RollChanceFast(chance);

		if (!rolledSuccessfully) {
			return;
		}

		REX::INFO("Loot chance successful, rolling for quantity.");

		int maxQuantity = static_cast<int>(WZ_PlatesLoot_MaxQuantity->value);
		int minQuantity = static_cast<int>(WZ_PlatesLoot_MinQuantity->value);

		maxQuantity = Utilities::Max(1, maxQuantity);
		minQuantity = Utilities::Max(1, minQuantity);

		if (minQuantity > maxQuantity) {
			std::swap(minQuantity, maxQuantity);
		}

		int quantityRolled = Utilities::RollIntRangeFast(minQuantity, maxQuantity);
		if (quantityRolled > 0) {
			REX::INFO("Rolled quantity: [{}], adding to aggressor inventory.", quantityRolled);
			targetToReceiveLoot->AddInventoryItem(WZ_Plates, nullptr, quantityRolled, nullptr, nullptr, nullptr);
		}
	}

	void Hook_ProcessHitData(RE::Actor* actor, RE::HitData& hitData)
	{
		ProcessPlates(actor, hitData);
		ProcessPlatesLoot(actor, hitData);
		
		typedef void (*FnProcessHitData)(RE::Actor* actor, RE::HitData& hitData);
		FnProcessHitData fn = (FnProcessHitData)ProcessHitDataOriginal;
		return fn ? (*fn)(actor, hitData) : void();
	}

	RE::BSEventNotifyControl MenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source)
	{
		if ((a_event.menuName == "LoadingMenu" || a_event.menuName == "PipboyMenu") && a_event.opening) {
			WZ_IsDoingIdle->value = 0.0f;
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void MenuWatcher::Initialize()
	{
		RE::UI::GetSingleton()->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(this);
	}

	std::unordered_map<uintptr_t, SubgraphWatcher::OriginalProcessEventFunction> SubgraphWatcher::FunctionMap;

	RE::BSEventNotifyControl SubgraphWatcher::ProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		if (a_event.tag == "sneakStateExit" && WZ_IsDoingIdle->value > 0.0f) {
			WZ_IsDoingIdle->value = 0.0f;
		}

		OriginalProcessEventFunction _function = FunctionMap.at(*(uintptr_t*)this);
		return _function ? (this->*_function)(a_event, a_source) : RE::BSEventNotifyControl::kContinue;
	}

	void SubgraphWatcher::Sink()
	{
		uintptr_t vtable = *(uintptr_t*)this;
		auto it = FunctionMap.find(vtable);

		if (it == FunctionMap.end()) {
			OriginalProcessEventFunction _function = Utilities::SafeWrite64Function(vtable + 0x8, &SubgraphWatcher::ProcessEvent);
			FunctionMap.insert(std::pair<uintptr_t, OriginalProcessEventFunction>(vtable, _function));
		}
	}

	void Initialize()
	{
		Player = RE::PlayerCharacter::GetSingleton();

		MenuWatcherInstance = new MenuWatcher();
		MenuWatcherInstance->Initialize();

		WZ_IsDoingIdle = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x16);
		WZ_PlatesHealth = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0xF99);
		WZ_PlatesMaxHealth = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0xF9A);
		WZ_PlatesHealthToAdd = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x11);
		WZ_PlatesProtectHealth = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x18);
		WZ_PlatesProtectLimbs = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x17);
		WZ_PlatesIgnoreArmor = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x19);
		WZ_PlatesLoot_Chance = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x22);
		WZ_PlatesLoot_MaxQuantity = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x23);
		WZ_PlatesLoot_MinQuantity = (RE::TESGlobal*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x24);
		WZ_PlatesBreakPotion = (RE::AlchemyItem*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x12);
		WZ_PlatesTakeDamagePotion = (RE::AlchemyItem*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x1B);
		WZ_Plates = (RE::TESObjectMISC*)Utilities::GetFormFromMod("Warzone - Armor Plates System.esp", 0x5);
		PowerArmorPerk = (RE::BGSPerk*)Utilities::GetFormFromMod("Fallout4.esm", 0x1F8A9);

		((SubgraphWatcher*)((uint64_t)Player + 0x38))->SubgraphWatcher::Sink();
	}

	void InitializeOnLaunch()
	{
		// AE = E8 ?? ?? ?? ?? 48 85 FF 0F 84 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? BA ?? ?? ?? ?? -> "Fallout4.exe"+0xC4D6F7
		// OG = E8 ?? ?? ?? ?? 48 85 FF 74 36 48 8B CF -> "Fallout4.exe"+0xD60D61

		uintptr_t ProcessHitData = REL::Relocation<uintptr_t>{ Utilities::GetFallout4BaseAddress() + 0xC4D6F7 }.address();

		REL::Trampoline& trampoline = REL::GetTrampoline();
		ProcessHitDataOriginal = trampoline.write_call<5>(ProcessHitData, &Hook_ProcessHitData);
	}
}
