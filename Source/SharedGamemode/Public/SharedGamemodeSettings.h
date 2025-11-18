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

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "SharedGamemodeSettings.generated.h"

/**
 * Scenario whitelist/blacklist entry
 */
USTRUCT(BlueprintType)
struct FScenarioFilterEntry
{
	GENERATED_BODY()

	/** Scenario asset ID to filter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (AllowedTypes = "GameplayScenario"))
	FPrimaryAssetId ScenarioId;

	/** Optional reason for filtering (for admin reference) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FString Reason;

	FScenarioFilterEntry()
		: ScenarioId()
		, Reason(TEXT(""))
	{}
};

/**
 * Shared Gamemode Settings
 *
 * Project-level settings for the Shared Gamemode plugin.
 * These settings can be configured per-project in Project Settings
 * or overridden per-server via command line or config files.
 *
 * To override settings via command line:
 * -ScenarioVotingEnabled=true -DefaultScenario="GameplayScenario'/Game/..."
 *
 * To override settings in DefaultGame.ini:
 * [/Script/SharedGamemode.SharedGamemodeSettings]
 * bEnableScenarioVoting=true
 * DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_Default.SC_Default'"
 */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "Shared Gamemode Plugin"))
class SHAREDGAMEMODE_API USharedGamemodeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USharedGamemodeSettings();

	// ============================================================================
	// SCENARIO CONFIGURATION
	// ============================================================================

	/**
	 * Default scenario to load on server start
	 * If not set, server will wait for manual scenario activation
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios", meta = (AllowedTypes = "GameplayScenario"))
	FPrimaryAssetId DefaultScenarioAsset;

	/**
	 * Whether to automatically load the default scenario on server start
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios")
	bool bAutoLoadDefaultScenario = false;

	/**
	 * Scenarios that are explicitly allowed on this server
	 * If empty, all scenarios are allowed (unless blacklisted)
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios|Filtering")
	TArray<FScenarioFilterEntry> ScenarioWhitelist;

	/**
	 * Scenarios that are explicitly blocked on this server
	 * Takes priority over whitelist
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios|Filtering")
	TArray<FScenarioFilterEntry> ScenarioBlacklist;

	/**
	 * Gameplay tags required for a scenario to be selectable
	 * Scenarios must have ALL of these tags
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios|Filtering")
	FGameplayTagContainer RequiredScenarioTags;

	/**
	 * Gameplay tags that exclude a scenario from selection
	 * Scenarios must have NONE of these tags
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scenarios|Filtering")
	FGameplayTagContainer ExcludedScenarioTags;

	// ============================================================================
	// VOTING CONFIGURATION
	// ============================================================================

	/**
	 * Whether scenario voting is enabled on this server
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Voting")
	bool bEnableScenarioVoting = true;

	/**
	 * Whether clients can request scenario changes
	 * If false, only server console/admins can change scenarios
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Voting")
	bool bAllowClientScenarioRequests = true;

	/**
	 * Minimum number of players required to start voting
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Voting", meta = (ClampMin = "0"))
	int32 MinimumPlayersForVoting = 2;

	/**
	 * Default voting duration in seconds
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Voting", meta = (ClampMin = "5.0", ClampMax = "300.0"))
	float DefaultVotingDuration = 30.0f;

	/**
	 * Default number of scenario options to present during voting
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Voting", meta = (ClampMin = "2", ClampMax = "10"))
	int32 DefaultNumScenarioOptions = 3;

	// ============================================================================
	// PERSISTENCE CONFIGURATION
	// ============================================================================

	/**
	 * Whether to enable scenario statistics persistence
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Persistence")
	bool bEnableScenarioPersistence = true;

	/**
	 * Filename for scenario stats (relative to Saved directory)
	 * Can be customized per-server for multi-server setups
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Persistence")
	FString PersistenceFileName = TEXT("ScenarioStats.json");

	/**
	 * Whether to auto-save scenario stats periodically
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Persistence")
	bool bAutoSaveStats = true;

	/**
	 * Interval between auto-saves in seconds
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Persistence", meta = (ClampMin = "10.0", EditCondition = "bAutoSaveStats"))
	float AutoSaveInterval = 60.0f;

	// ============================================================================
	// SERVER CONFIGURATION
	// ============================================================================

	/**
	 * Whether to enable dedicated server console commands
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Server")
	bool bEnableConsoleCommands = true;

	/**
	 * Whether to allow seamless travel for scenario map transitions
	 * Seamless travel keeps players connected during map changes
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Server")
	bool bUseSeamlessTravel = false;

	/**
	 * Logging verbosity for scenario system
	 * 0 = Errors only, 1 = Warnings + Errors, 2 = Log + Warnings + Errors, 3 = Verbose
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Server", meta = (ClampMin = "0", ClampMax = "3"))
	int32 LogVerbosity = 2;

	// ============================================================================
	// UTILITY METHODS
	// ============================================================================

	/**
	 * Check if a scenario is allowed on this server
	 * @param ScenarioId The scenario to check
	 * @return True if the scenario can be activated
	 */
	UFUNCTION(BlueprintPure, Category = "Scenarios")
	bool IsScenarioAllowed(FPrimaryAssetId ScenarioId) const;

	/**
	 * Check if a scenario matches the tag requirements
	 * @param ScenarioTags The scenario's gameplay tags
	 * @return True if the scenario matches tag filters
	 */
	UFUNCTION(BlueprintPure, Category = "Scenarios")
	bool DoesScenarioMatchTagFilters(const FGameplayTagContainer& ScenarioTags) const;

	/**
	 * Get the singleton instance of settings
	 */
	static const USharedGamemodeSettings* Get();

	/**
	 * Get mutable settings (use carefully!)
	 */
	static USharedGamemodeSettings* GetMutable();

	// ============================================================================
	// DEVELOPER SETTINGS OVERRIDES
	// ============================================================================

	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
};
