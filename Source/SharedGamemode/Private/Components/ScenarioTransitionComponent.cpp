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

#include "Components/ScenarioTransitionComponent.h"
#include "ScenarioInstanceSubsystem.h"
#include "GameplayScenario.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenarioTransition, Log, All);

UScenarioTransitionComponent::UScenarioTransitionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UScenarioTransitionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UScenarioTransitionComponent, bVotingActive);
	DOREPLIFETIME(UScenarioTransitionComponent, VotingTimeRemaining);
	DOREPLIFETIME(UScenarioTransitionComponent, VoteOptions);
}

void UScenarioTransitionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache the scenario subsystem
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		CachedScenarioSubsystem = GameInstance->GetSubsystem<UScenarioInstanceSubsystem>();
	}
}

void UScenarioTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only tick on server during voting
	if (GetOwnerRole() != ROLE_Authority || !bVotingActive)
	{
		return;
	}

	VotingTimeRemaining -= DeltaTime;

	if (VotingTimeRemaining <= 0.0f)
	{
		CompleteVoting();
	}
}

// ============================================================================
// VOTING PUBLIC INTERFACE
// ============================================================================

bool UScenarioTransitionComponent::StartVoting()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("StartVoting called on client - must be called on server"));
		return false;
	}

	if (bVotingActive)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("Voting already active"));
		return false;
	}

	// Select scenario options
	TArray<FPrimaryAssetId> SelectedScenarios;
	SelectScenarioOptions(SelectedScenarios);

	if (SelectedScenarios.Num() == 0)
	{
		UE_LOG(LogScenarioTransition, Error, TEXT("No scenarios available for voting"));
		return false;
	}

	// Initialize vote options
	VoteOptions.Empty();
	for (const FPrimaryAssetId& ScenarioId : SelectedScenarios)
	{
		VoteOptions.Add(FScenarioVoteOption(ScenarioId));
	}

	// Reset state
	PlayerVotes.Empty();
	VotingTimeRemaining = VotingDuration;
	bVotingActive = true;

	// Start ticking
	SetComponentTickEnabled(true);

	UE_LOG(LogScenarioTransition, Log, TEXT("Voting started with %d options for %.1f seconds"), VoteOptions.Num(), VotingDuration);

	// Broadcast state change
	OnVotingStateChanged.Broadcast(true);

	return true;
}

void UScenarioTransitionComponent::CancelVoting()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("CancelVoting called on client - must be called on server"));
		return;
	}

	if (!bVotingActive)
	{
		return;
	}

	bVotingActive = false;
	VotingTimeRemaining = 0.0f;
	VoteOptions.Empty();
	PlayerVotes.Empty();

	SetComponentTickEnabled(false);

	UE_LOG(LogScenarioTransition, Log, TEXT("Voting cancelled"));

	OnVotingStateChanged.Broadcast(false);
}

void UScenarioTransitionComponent::CastVote(FPrimaryAssetId ScenarioId)
{
	// Get the local player state
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("CastVote: No local player controller"));
		return;
	}

	APlayerState* LocalPlayerState = PC->GetPlayerState<APlayerState>();
	if (!LocalPlayerState)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("CastVote: No local player state"));
		return;
	}

	// Send RPC to server
	ServerCastVote(LocalPlayerState, ScenarioId);
}

bool UScenarioTransitionComponent::GetWinningScenario(FPrimaryAssetId& OutScenarioId) const
{
	if (VoteOptions.Num() == 0)
	{
		return false;
	}

	const FScenarioVoteOption* Winner = &VoteOptions[0];
	for (const FScenarioVoteOption& Option : VoteOptions)
	{
		if (Option.VoteTally > Winner->VoteTally)
		{
			Winner = &Option;
		}
	}

	if (Winner->VoteTally > 0.0f)
	{
		OutScenarioId = Winner->ScenarioId;
		return true;
	}

	// If no votes, return first option
	OutScenarioId = VoteOptions[0].ScenarioId;
	return true;
}

bool UScenarioTransitionComponent::GetScenarioVotes(FPrimaryAssetId ScenarioId, float& OutVoteTally, int32& OutVoteCount) const
{
	for (const FScenarioVoteOption& Option : VoteOptions)
	{
		if (Option.ScenarioId == ScenarioId)
		{
			OutVoteTally = Option.VoteTally;
			OutVoteCount = Option.VoteCount;
			return true;
		}
	}

	return false;
}

// ============================================================================
// EXTENSIBILITY POINTS - Default Implementations
// ============================================================================

float UScenarioTransitionComponent::GetVoteWeight_Implementation(APlayerState* Voter) const
{
	// Default: All votes have equal weight
	return 1.0f;
}

void UScenarioTransitionComponent::SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios)
{
	// Default: Get random scenarios from the asset manager
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AllScenarios;
	Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

	// Filter by tags if query is set
	if (!ScenarioFilterQuery.IsEmpty())
	{
		TArray<FPrimaryAssetId> FilteredScenarios;
		for (const FPrimaryAssetId& ScenarioId : AllScenarios)
		{
			// Try to load scenario to check tags
			UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);
			if (Scenario && ScenarioFilterQuery.Matches(Scenario->ScenarioTags))
			{
				FilteredScenarios.Add(ScenarioId);
			}
		}
		AllScenarios = FilteredScenarios;
	}

	if (AllScenarios.Num() == 0)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("No scenarios found for voting"));
		return;
	}

	// Randomly select NumScenarioOptions scenarios
	int32 NumToSelect = FMath::Min(NumScenarioOptions, AllScenarios.Num());

	// Shuffle and take first N
	for (int32 i = 0; i < NumToSelect; i++)
	{
		int32 RandomIndex = FMath::RandRange(i, AllScenarios.Num() - 1);
		AllScenarios.Swap(i, RandomIndex);
	}

	OutScenarios.Append(AllScenarios.GetData(), NumToSelect);

	UE_LOG(LogScenarioTransition, Verbose, TEXT("Selected %d scenario options for voting"), OutScenarios.Num());
}

bool UScenarioTransitionComponent::ShouldAllowVote_Implementation(APlayerState* Voter) const
{
	// Default: Allow all players to vote
	return Voter != nullptr;
}

void UScenarioTransitionComponent::OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId)
{
	// Default: Log the result
	UE_LOG(LogScenarioTransition, Log, TEXT("Voting complete. Winner: %s"), *WinningScenarioId.ToString());
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void UScenarioTransitionComponent::ServerCastVote_Implementation(APlayerState* Voter, FPrimaryAssetId ScenarioId)
{
	if (!bVotingActive)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("Vote rejected: Voting not active"));
		return;
	}

	if (!ShouldAllowVote(Voter))
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("Vote rejected: Player not allowed to vote"));
		return;
	}

	// Verify scenario is a valid option
	FScenarioVoteOption* TargetOption = VoteOptions.FindByPredicate([ScenarioId](const FScenarioVoteOption& Option) {
		return Option.ScenarioId == ScenarioId;
	});

	if (!TargetOption)
	{
		UE_LOG(LogScenarioTransition, Warning, TEXT("Vote rejected: Invalid scenario ID"));
		return;
	}

	// Get vote weight
	float VoteWeight = GetVoteWeight(Voter);

	// Check if player already voted
	if (FScenarioVoteEntry* ExistingVote = PlayerVotes.Find(Voter))
	{
		// Remove previous vote
		FScenarioVoteOption* PrevOption = VoteOptions.FindByPredicate([ExistingVote](const FScenarioVoteOption& Option) {
			return Option.ScenarioId == ExistingVote->ScenarioId;
		});

		if (PrevOption)
		{
			PrevOption->VoteTally -= ExistingVote->VoteWeight;
			PrevOption->VoteCount--;
		}
	}

	// Add new vote
	TargetOption->VoteTally += VoteWeight;
	TargetOption->VoteCount++;
	PlayerVotes.Add(Voter, FScenarioVoteEntry(Voter, ScenarioId, VoteWeight));

	UE_LOG(LogScenarioTransition, Verbose, TEXT("Vote cast: %s voted for %s (weight: %.2f)"),
		*Voter->GetPlayerName(), *ScenarioId.ToString(), VoteWeight);

	// Broadcast event
	OnVoteCast.Broadcast(Voter, ScenarioId, VoteWeight);
}

bool UScenarioTransitionComponent::ServerCastVote_Validate(APlayerState* Voter, FPrimaryAssetId ScenarioId)
{
	return Voter != nullptr && ScenarioId.IsValid();
}

void UScenarioTransitionComponent::OnRep_VotingActive()
{
	OnVotingStateChanged.Broadcast(bVotingActive);
}

void UScenarioTransitionComponent::CompleteVoting()
{
	if (!bVotingActive)
	{
		return;
	}

	FPrimaryAssetId WinningScenarioId;
	if (!GetWinningScenario(WinningScenarioId))
	{
		UE_LOG(LogScenarioTransition, Error, TEXT("Voting complete but no winner found"));
		CancelVoting();
		return;
	}

	UE_LOG(LogScenarioTransition, Log, TEXT("Voting complete. Winner: %s with %.1f votes"),
		*WinningScenarioId.ToString(),
		VoteOptions.FindByPredicate([WinningScenarioId](const FScenarioVoteOption& Option) {
			return Option.ScenarioId == WinningScenarioId;
		})->VoteTally);

	// Deactivate voting
	bVotingActive = false;
	SetComponentTickEnabled(false);

	// Call extensibility point
	OnVotingComplete(WinningScenarioId);

	// Broadcast delegate
	OnVotingCompleteDelegate.Broadcast(WinningScenarioId);

	// Auto-transition if enabled
	if (bAutoTransitionOnVoteComplete)
	{
		if (UScenarioInstanceSubsystem* ScenarioSubsystem = GetScenarioSubsystem())
		{
			UAssetManager& Manager = UAssetManager::Get();
			UGameplayScenario* WinningScenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(WinningScenarioId);

			if (!WinningScenario)
			{
				// Load the scenario if not already loaded
				auto Handle = Manager.LoadPrimaryAsset(WinningScenarioId);
				if (Handle.IsValid())
				{
					Handle->WaitUntilComplete();
				}
				WinningScenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(WinningScenarioId);
			}

			if (WinningScenario)
			{
				ScenarioSubsystem->SetPendingScenario(WinningScenario);
				ScenarioSubsystem->TransitionToPendingScenario(true);
			}
			else
			{
				UE_LOG(LogScenarioTransition, Error, TEXT("Failed to load winning scenario: %s"), *WinningScenarioId.ToString());
			}
		}
	}
}

UScenarioInstanceSubsystem* UScenarioTransitionComponent::GetScenarioSubsystem() const
{
	if (CachedScenarioSubsystem)
	{
		return CachedScenarioSubsystem;
	}

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UScenarioInstanceSubsystem>();
	}

	return nullptr;
}
