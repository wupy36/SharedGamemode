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
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScenarioPersistenceManager.generated.h"

class UGameplayScenario;

/**
 * Persistent statistics for a single scenario
 */
USTRUCT(BlueprintType)
struct FScenarioStats
{
	GENERATED_BODY()

	/** Scenario asset ID */
	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId ScenarioId;

	/** Number of times this scenario has been played */
	UPROPERTY(BlueprintReadOnly)
	int32 TimesPlayed = 0;

	/** Total votes received across all voting sessions */
	UPROPERTY(BlueprintReadOnly)
	int32 TotalVotes = 0;

	/** Last time this scenario was activated (Unix timestamp) */
	UPROPERTY(BlueprintReadOnly)
	int64 LastPlayedTimestamp = 0;

	/** Average player count when this scenario is active */
	UPROPERTY(BlueprintReadOnly)
	float AveragePlayerCount = 0.0f;

	/** Custom rotation weight (higher = more likely to be selected) */
	UPROPERTY(BlueprintReadWrite)
	float RotationWeight = 1.0f;

	FScenarioStats()
		: ScenarioId()
		, TimesPlayed(0)
		, TotalVotes(0)
		, LastPlayedTimestamp(0)
		, AveragePlayerCount(0.0f)
		, RotationWeight(1.0f)
	{}

	explicit FScenarioStats(FPrimaryAssetId InScenarioId)
		: ScenarioId(InScenarioId)
		, TimesPlayed(0)
		, TotalVotes(0)
		, LastPlayedTimestamp(0)
		, AveragePlayerCount(0.0f)
		, RotationWeight(1.0f)
	{}
};

/**
 * Scenario Persistence Manager
 *
 * Game Instance Subsystem that tracks and persists scenario statistics.
 * Designed to support map rotation, popularity tracking, and custom
 * scenario selection algorithms.
 *
 * Features:
 * - Automatic stat tracking (play count, votes, player count)
 * - JSON-based persistence to disk
 * - Rotation weight system for balanced map rotation
 * - Extensible for custom selection algorithms
 *
 * To extend this class:
 * 1. Override CalculateRotationWeight() for custom weighting logic
 * 2. Override SelectScenariosWeighted() for custom selection algorithms
 * 3. Override GetPersistencePath() to customize save location
 */
UCLASS(Blueprintable)
class SHAREDGAMEMODE_API UScenarioPersistenceManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UScenarioPersistenceManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ============================================================================
	// PERSISTENCE CONFIGURATION
	// ============================================================================

	/** Filename for persistent storage (relative to Saved directory) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Persistence|Config")
	FString PersistenceFileName = TEXT("ScenarioStats.json");

	/** Whether to auto-save stats when scenarios activate/deactivate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Persistence|Config")
	bool bAutoSave = true;

	/** Minimum time between auto-saves (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Persistence|Config")
	float AutoSaveInterval = 60.0f;

	// ============================================================================
	// ROTATION CONFIGURATION
	// ============================================================================

	/** Penalty multiplier for recently played scenarios (0.0 - 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rotation|Config")
	float RecencyPenalty = 0.5f;

	/** Time window for recency penalty (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rotation|Config")
	float RecencyPenaltyWindow = 3600.0f; // 1 hour

	/** Bonus multiplier for popular scenarios (based on votes) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rotation|Config")
	float PopularityBonus = 1.2f;

	/** Minimum votes to be considered "popular" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rotation|Config")
	int32 PopularityThreshold = 10;

	// ============================================================================
	// STAT TRACKING - Public Interface
	// ============================================================================

	/**
	 * Record that a scenario was activated
	 * @param ScenarioId The scenario that was activated
	 * @param NumPlayers Current number of players
	 */
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	void RecordScenarioPlayed(FPrimaryAssetId ScenarioId, int32 NumPlayers);

	/**
	 * Record votes for a scenario
	 * @param ScenarioId The scenario that received votes
	 * @param NumVotes Number of votes to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	void RecordScenarioVotes(FPrimaryAssetId ScenarioId, int32 NumVotes);

	/**
	 * Get stats for a specific scenario
	 * @param ScenarioId The scenario to query
	 * @param OutStats The stats structure (only valid if function returns true)
	 * @return True if stats exist for this scenario
	 */
	UFUNCTION(BlueprintPure, Category = "Persistence")
	bool GetScenarioStats(FPrimaryAssetId ScenarioId, FScenarioStats& OutStats) const;

	/**
	 * Get all tracked scenario stats
	 * @return Array of all scenario stats
	 */
	UFUNCTION(BlueprintPure, Category = "Persistence")
	TArray<FScenarioStats> GetAllStats() const;

	/**
	 * Manually save stats to disk
	 * @return True if save was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	bool SaveStats();

	/**
	 * Manually load stats from disk
	 * @return True if load was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	bool LoadStats();

	/**
	 * Reset all stats (does not save automatically)
	 */
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	void ResetStats();

	// ============================================================================
	// SCENARIO SELECTION
	// ============================================================================

	/**
	 * Select scenarios using weighted random selection
	 * @param NumScenarios Number of scenarios to select
	 * @param FilterTags Optional gameplay tag query to filter scenarios
	 * @return Array of selected scenario IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Rotation")
	TArray<FPrimaryAssetId> SelectScenariosWeighted(int32 NumScenarios, const FGameplayTagQuery& FilterTags);

	/**
	 * Get the least recently played scenarios
	 * @param NumScenarios Number of scenarios to return
	 * @param FilterTags Optional gameplay tag query to filter scenarios
	 * @return Array of scenario IDs sorted by last played time (oldest first)
	 */
	UFUNCTION(BlueprintCallable, Category = "Rotation")
	TArray<FPrimaryAssetId> GetLeastRecentlyPlayed(int32 NumScenarios, const FGameplayTagQuery& FilterTags);

	/**
	 * Get the most popular scenarios (by vote count)
	 * @param NumScenarios Number of scenarios to return
	 * @param FilterTags Optional gameplay tag query to filter scenarios
	 * @return Array of scenario IDs sorted by total votes (highest first)
	 */
	UFUNCTION(BlueprintCallable, Category = "Rotation")
	TArray<FPrimaryAssetId> GetMostPopular(int32 NumScenarios, const FGameplayTagQuery& FilterTags);

	// ============================================================================
	// EXTENSIBILITY POINTS
	// ============================================================================

	/**
	 * Calculate rotation weight for a scenario
	 * Override this to implement custom weighting algorithms
	 * @param ScenarioId The scenario to calculate weight for
	 * @return Weight value (higher = more likely to be selected)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Rotation|Extensibility")
	float CalculateRotationWeight(FPrimaryAssetId ScenarioId) const;
	virtual float CalculateRotationWeight_Implementation(FPrimaryAssetId ScenarioId) const;

	/**
	 * Get the full path for persistence file
	 * Override this to customize save location (e.g., per-server configs)
	 * @return Full file path
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Persistence|Extensibility")
	FString GetPersistencePath() const;
	virtual FString GetPersistencePath_Implementation() const;

protected:
	/**
	 * Callback when a scenario is activated
	 */
	void OnScenarioActivated(UGameplayScenario* Scenario);

	/**
	 * Get or create stats entry for a scenario
	 */
	FScenarioStats& GetOrCreateStats(FPrimaryAssetId ScenarioId);

	/**
	 * Check if auto-save should trigger
	 */
	void CheckAutoSave();

private:
	/** Map of scenario ID to stats */
	UPROPERTY()
	TMap<FPrimaryAssetId, FScenarioStats> ScenarioStatsMap;

	/** Time of last save */
	double LastSaveTime = 0.0;
};
