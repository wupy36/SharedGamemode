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
#include "Components/ScenarioTransitionComponent.h"
#include "EnhancedScenarioTransitionComponent.generated.h"

class UScenarioPersistenceManager;

/**
 * Enhanced Scenario Transition Component
 *
 * Example implementation showing how to extend the base ScenarioTransitionComponent.
 * This component adds:
 * - Performance-based vote weighting
 * - Integration with ScenarioPersistenceManager for rotation
 * - Custom scenario selection using least-recently-played logic
 * - Vote statistics tracking
 *
 * Use this as a reference for creating your own custom transition components.
 * You can:
 * - Copy and modify this class for your game
 * - Create a Blueprint child class and override event functions
 * - Use this directly if the features match your needs
 */
UCLASS(ClassGroup = (GameplayScenario), meta = (BlueprintSpawnableComponent), Blueprintable)
class SHAREDGAMEMODE_API UEnhancedScenarioTransitionComponent : public UScenarioTransitionComponent
{
	GENERATED_BODY()

public:
	UEnhancedScenarioTransitionComponent();

	// ============================================================================
	// ENHANCED VOTING CONFIGURATION
	// ============================================================================

	/**
	 * Minimum vote weight for any player
	 * Lower-performing players will still have at least this weight
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Enhanced")
	float MinimumVoteWeight = 0.5f;

	/**
	 * Maximum vote weight for any player
	 * Higher-performing players will have at most this weight
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Enhanced")
	float MaximumVoteWeight = 2.0f;

	/**
	 * Multiplier for performance-based weighting
	 * Higher values increase the impact of performance on vote weight
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Enhanced")
	float PerformanceWeightMultiplier = 0.1f;

	/**
	 * Whether to use persistence manager for scenario selection
	 * If true, uses rotation logic to avoid repeating scenarios
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Enhanced")
	bool bUseRotationLogic = true;

	/**
	 * Whether to track vote statistics in persistence manager
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Enhanced")
	bool bTrackVoteStatistics = true;

	// ============================================================================
	// EXTENSIBILITY OVERRIDES
	// ============================================================================

	/**
	 * Override: Calculate vote weight based on player performance
	 * This implementation uses player score to weight votes
	 */
	virtual float GetVoteWeight_Implementation(APlayerState* Voter) const override;

	/**
	 * Override: Select scenarios using persistence manager rotation logic
	 * Prioritizes least-recently-played scenarios
	 */
	virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios) override;

	/**
	 * Override: Track voting results in persistence manager
	 */
	virtual void OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId) override;

	// ============================================================================
	// ADDITIONAL FEATURES
	// ============================================================================

	/**
	 * Get the current player's performance score
	 * Override this in Blueprint or subclass to use your game's scoring system
	 * @param PlayerState The player to evaluate
	 * @return Performance score (higher = better player)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Voting|Enhanced")
	float GetPlayerPerformanceScore(APlayerState* PlayerState) const;
	virtual float GetPlayerPerformanceScore_Implementation(APlayerState* PlayerState) const;

protected:
	/**
	 * Get the persistence manager subsystem
	 */
	UScenarioPersistenceManager* GetPersistenceManager() const;

private:
	/** Cached persistence manager */
	UPROPERTY(Transient)
	TObjectPtr<UScenarioPersistenceManager> CachedPersistenceManager;
};
