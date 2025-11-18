/*
Copyright 2021 Empires Team

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "SharedGamemodeSettings.h"
#include "GameplayScenario.h"
#include "Engine/AssetManager.h"

USharedGamemodeSettings::USharedGamemodeSettings()
{
	CategoryName = TEXT("Plugins");
}

bool USharedGamemodeSettings::IsScenarioAllowed(FPrimaryAssetId ScenarioId) const
{
	// Check blacklist first (takes priority)
	for (const FScenarioFilterEntry& Entry : ScenarioBlacklist)
	{
		if (Entry.ScenarioId == ScenarioId)
		{
			return false;
		}
	}

	// If whitelist exists, check if scenario is in it
	if (ScenarioWhitelist.Num() > 0)
	{
		bool bFoundInWhitelist = false;
		for (const FScenarioFilterEntry& Entry : ScenarioWhitelist)
		{
			if (Entry.ScenarioId == ScenarioId)
			{
				bFoundInWhitelist = true;
				break;
			}
		}

		if (!bFoundInWhitelist)
		{
			return false;
		}
	}

	// Check tag filters if scenario is loaded
	UAssetManager& Manager = UAssetManager::Get();
	UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);
	if (Scenario)
	{
		return DoesScenarioMatchTagFilters(Scenario->ScenarioTags);
	}

	// If scenario is not loaded, allow it (tag check will happen on activation)
	return true;
}

bool USharedGamemodeSettings::DoesScenarioMatchTagFilters(const FGameplayTagContainer& ScenarioTags) const
{
	// Check required tags
	if (RequiredScenarioTags.Num() > 0)
	{
		if (!ScenarioTags.HasAll(RequiredScenarioTags))
		{
			return false;
		}
	}

	// Check excluded tags
	if (ExcludedScenarioTags.Num() > 0)
	{
		if (ScenarioTags.HasAny(ExcludedScenarioTags))
		{
			return false;
		}
	}

	return true;
}

const USharedGamemodeSettings* USharedGamemodeSettings::Get()
{
	return GetDefault<USharedGamemodeSettings>();
}

USharedGamemodeSettings* USharedGamemodeSettings::GetMutable()
{
	return GetMutableDefault<USharedGamemodeSettings>();
}

FName USharedGamemodeSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

#if WITH_EDITOR
FText USharedGamemodeSettings::GetSectionText() const
{
	return NSLOCTEXT("SharedGamemode", "SharedGamemodeSettingsSection", "Shared Gamemode");
}

FText USharedGamemodeSettings::GetSectionDescription() const
{
	return NSLOCTEXT("SharedGamemode", "SharedGamemodeSettingsDescription",
		"Configure the Shared Gamemode plugin for scenarios, voting, and dedicated server settings.");
}
#endif
