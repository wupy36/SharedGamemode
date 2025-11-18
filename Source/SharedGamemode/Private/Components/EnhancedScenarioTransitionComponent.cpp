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

#include "Components/EnhancedScenarioTransitionComponent.h"
#include "ScenarioPersistenceManager.h"
#include "SharedGamemodeSettings.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnhancedScenarioTransition, Log, All);

UEnhancedScenarioTransitionComponent::UEnhancedScenarioTransitionComponent()
{
	// Set defaults from settings if available
	if (const USharedGamemodeSettings* Settings = USharedGamemodeSettings::Get())
	{
		VotingDuration = Settings->DefaultVotingDuration;
		NumScenarioOptions = Settings->DefaultNumScenarioOptions;
	}
}

// ============================================================================
// EXTENSIBILITY OVERRIDES
// ============================================================================

float UEnhancedScenarioTransitionComponent::GetVoteWeight_Implementation(APlayerState* Voter) const
{
	if (!Voter)
	{
		return 1.0f;
	}

	// Get player performance score
	float PerformanceScore = GetPlayerPerformanceScore(Voter);

	// Calculate weight based on performance
	// Formula: BaseWeight (1.0) + (PerformanceScore * Multiplier)
	float Weight = 1.0f + (PerformanceScore * PerformanceWeightMultiplier);

	// Clamp to min/max range
	Weight = FMath::Clamp(Weight, MinimumVoteWeight, MaximumVoteWeight);

	UE_LOG(LogEnhancedScenarioTransition, Verbose, TEXT("Player %s vote weight: %.2f (Performance: %.2f)"),
		*Voter->GetPlayerName(), Weight, PerformanceScore);

	return Weight;
}

void UEnhancedScenarioTransitionComponent::SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios)
{
	if (bUseRotationLogic)
	{
		// Use persistence manager for intelligent rotation
		if (UScenarioPersistenceManager* PersistenceManager = GetPersistenceManager())
		{
			OutScenarios = PersistenceManager->SelectScenariosWeighted(NumScenarioOptions, ScenarioFilterQuery);

			if (OutScenarios.Num() > 0)
			{
				UE_LOG(LogEnhancedScenarioTransition, Log, TEXT("Selected %d scenarios using rotation logic"), OutScenarios.Num());
				return;
			}
			else
			{
				UE_LOG(LogEnhancedScenarioTransition, Warning, TEXT("Rotation logic returned no scenarios, falling back to random selection"));
			}
		}
	}

	// Fall back to base implementation (random selection)
	Super::SelectScenarioOptions_Implementation(OutScenarios);
}

void UEnhancedScenarioTransitionComponent::OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId)
{
	// Call parent implementation
	Super::OnVotingComplete_Implementation(WinningScenarioId);

	// Track vote statistics if enabled
	if (bTrackVoteStatistics)
	{
		if (UScenarioPersistenceManager* PersistenceManager = GetPersistenceManager())
		{
			// Record votes for all options
			for (const FScenarioVoteOption& Option : VoteOptions)
			{
				if (Option.VoteCount > 0)
				{
					PersistenceManager->RecordScenarioVotes(Option.ScenarioId, Option.VoteCount);
				}
			}

			UE_LOG(LogEnhancedScenarioTransition, Verbose, TEXT("Recorded votes for %d scenarios"), VoteOptions.Num());
		}
	}
}

// ============================================================================
// ADDITIONAL FEATURES
// ============================================================================

float UEnhancedScenarioTransitionComponent::GetPlayerPerformanceScore_Implementation(APlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return 0.0f;
	}

	// Default implementation: Use player score from PlayerState
	// Normalize to 0-1 range based on all players
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return 0.0f;
	}

	// Find max score among all players
	float MaxScore = 1.0f; // Minimum to avoid divide by zero
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS && PS->GetScore() > MaxScore)
		{
			MaxScore = PS->GetScore();
		}
	}

	// Normalize player's score
	float NormalizedScore = PlayerState->GetScore() / MaxScore;

	return NormalizedScore;
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

UScenarioPersistenceManager* UEnhancedScenarioTransitionComponent::GetPersistenceManager() const
{
	if (CachedPersistenceManager)
	{
		return CachedPersistenceManager;
	}

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		CachedPersistenceManager = GameInstance->GetSubsystem<UScenarioPersistenceManager>();
		return CachedPersistenceManager;
	}

	return nullptr;
}
