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
#include "Components/GameStateComponent.h"
#include "GameplayTagContainer.h"
#include "ScenarioTransitionComponent.generated.h"

class UGameplayScenario;
class UScenarioInstanceSubsystem;

/**
 * Vote entry structure
 * Tracks individual player votes with optional weighting
 */
USTRUCT(BlueprintType)
struct FScenarioVoteEntry
{
	GENERATED_BODY()

	/** The player state that cast this vote */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> Voter;

	/** The scenario asset ID being voted for */
	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId ScenarioId;

	/** Vote weight (default 1.0, can be modified for performance-based voting) */
	UPROPERTY(BlueprintReadOnly)
	float VoteWeight = 1.0f;

	FScenarioVoteEntry()
		: Voter(nullptr)
		, ScenarioId()
		, VoteWeight(1.0f)
	{}

	FScenarioVoteEntry(APlayerState* InVoter, FPrimaryAssetId InScenarioId, float InWeight = 1.0f)
		: Voter(InVoter)
		, ScenarioId(InScenarioId)
		, VoteWeight(InWeight)
	{}
};

/**
 * Scenario option for voting
 * Represents a single scenario that can be voted on
 */
USTRUCT(BlueprintType)
struct FScenarioVoteOption
{
	GENERATED_BODY()

	/** The scenario asset ID */
	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId ScenarioId;

	/** Current vote tally (includes weighting) */
	UPROPERTY(BlueprintReadOnly)
	float VoteTally = 0.0f;

	/** Number of individual votes (not weighted) */
	UPROPERTY(BlueprintReadOnly)
	int32 VoteCount = 0;

	FScenarioVoteOption()
		: ScenarioId()
		, VoteTally(0.0f)
		, VoteCount(0)
	{}

	explicit FScenarioVoteOption(FPrimaryAssetId InScenarioId)
		: ScenarioId(InScenarioId)
		, VoteTally(0.0f)
		, VoteCount(0)
	{}
};

/**
 * Delegate called when voting state changes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVotingStateChanged, bool, bIsVoting);

/**
 * Delegate called when a vote is cast
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVoteCast, APlayerState*, Voter, FPrimaryAssetId, ScenarioId, float, VoteWeight);

/**
 * Delegate called when voting completes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVotingComplete, FPrimaryAssetId, WinningScenarioId);

/**
 * Base Scenario Transition Component
 *
 * This component handles voting and transitioning between scenarios.
 * It is designed to be subclassed to add custom voting logic, weighting,
 * and transition behavior.
 *
 * Design Philosophy:
 * - Provides core voting infrastructure
 * - Delegates game-specific logic to subclasses
 * - Server-authoritative with client RPC support
 * - Modular and extensible
 *
 * To extend this class:
 * 1. Override GetVoteWeight() to implement custom vote weighting
 * 2. Override SelectScenarioOptions() to customize scenario selection
 * 3. Override ShouldAllowVote() to add vote validation logic
 * 4. Override OnVotingComplete_Internal() to add post-vote behavior
 */
UCLASS(ClassGroup = (GameplayScenario), meta = (BlueprintSpawnableComponent), Blueprintable)
class SHAREDGAMEMODE_API UScenarioTransitionComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UScenarioTransitionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================================
	// VOTING CONFIGURATION
	// ============================================================================

	/** Duration of voting period in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Config")
	float VotingDuration = 30.0f;

	/** Number of scenario options to present during voting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Config")
	int32 NumScenarioOptions = 3;

	/** Gameplay tags to filter available scenarios */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Config")
	FGameplayTagQuery ScenarioFilterQuery;

	/** Whether to automatically transition when voting completes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting|Config")
	bool bAutoTransitionOnVoteComplete = true;

	// ============================================================================
	// VOTING STATE (Replicated)
	// ============================================================================

	/** Whether voting is currently active */
	UPROPERTY(ReplicatedUsing = OnRep_VotingActive, BlueprintReadOnly, Category = "Voting|State")
	bool bVotingActive = false;

	/** Time remaining in current vote */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Voting|State")
	float VotingTimeRemaining = 0.0f;

	/** Available scenarios to vote on */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Voting|State")
	TArray<FScenarioVoteOption> VoteOptions;

	// ============================================================================
	// VOTING METHODS - Public Interface
	// ============================================================================

	/**
	 * Start a new voting session (Server only)
	 * @return True if voting started successfully
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Voting")
	virtual bool StartVoting();

	/**
	 * Cancel the current voting session (Server only)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Voting")
	virtual void CancelVoting();

	/**
	 * Client RPC to cast a vote
	 * @param ScenarioId The scenario to vote for
	 */
	UFUNCTION(BlueprintCallable, Category = "Voting")
	void CastVote(FPrimaryAssetId ScenarioId);

	/**
	 * Get the current winning scenario
	 * @param OutScenarioId The scenario ID with the most votes
	 * @return True if there is a valid winner
	 */
	UFUNCTION(BlueprintPure, Category = "Voting")
	bool GetWinningScenario(FPrimaryAssetId& OutScenarioId) const;

	/**
	 * Get vote count for a specific scenario
	 * @param ScenarioId The scenario to query
	 * @param OutVoteTally The weighted vote tally
	 * @param OutVoteCount The raw vote count
	 * @return True if the scenario is a valid vote option
	 */
	UFUNCTION(BlueprintPure, Category = "Voting")
	bool GetScenarioVotes(FPrimaryAssetId ScenarioId, float& OutVoteTally, int32& OutVoteCount) const;

	// ============================================================================
	// EXTENSIBILITY POINTS - Override these in subclasses
	// ============================================================================

	/**
	 * Get the vote weight for a specific player
	 * Override this to implement performance-based or role-based weighting
	 * @param Voter The player state casting the vote
	 * @return The weight multiplier for this player's vote (1.0 = normal)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Voting|Extensibility")
	float GetVoteWeight(APlayerState* Voter) const;
	virtual float GetVoteWeight_Implementation(APlayerState* Voter) const;

	/**
	 * Select which scenarios should be available for voting
	 * Override this to implement custom selection logic (rotation, popularity, etc.)
	 * @param OutScenarios The selected scenario IDs
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Voting|Extensibility")
	void SelectScenarioOptions(TArray<FPrimaryAssetId>& OutScenarios);
	virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios);

	/**
	 * Determine if a player is allowed to vote
	 * Override this to add custom validation (e.g., check if player is spectator)
	 * @param Voter The player state attempting to vote
	 * @return True if the player can vote
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Voting|Extensibility")
	bool ShouldAllowVote(APlayerState* Voter) const;
	virtual bool ShouldAllowVote_Implementation(APlayerState* Voter) const;

	/**
	 * Called when voting completes (before transition)
	 * Override this to add custom behavior (e.g., save stats, notify players)
	 * @param WinningScenarioId The scenario that won the vote
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Voting|Extensibility")
	void OnVotingComplete(FPrimaryAssetId WinningScenarioId);
	virtual void OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId);

	// ============================================================================
	// DELEGATES
	// ============================================================================

	/** Broadcast when voting state changes */
	UPROPERTY(BlueprintAssignable, Category = "Voting|Events")
	FOnVotingStateChanged OnVotingStateChanged;

	/** Broadcast when a vote is cast */
	UPROPERTY(BlueprintAssignable, Category = "Voting|Events")
	FOnVoteCast OnVoteCast;

	/** Broadcast when voting completes */
	UPROPERTY(BlueprintAssignable, Category = "Voting|Events")
	FOnVotingComplete OnVotingCompleteDelegate;

protected:
	// ============================================================================
	// INTERNAL METHODS
	// ============================================================================

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Server RPC for casting votes */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCastVote(APlayerState* Voter, FPrimaryAssetId ScenarioId);
	void ServerCastVote_Implementation(APlayerState* Voter, FPrimaryAssetId ScenarioId);
	bool ServerCastVote_Validate(APlayerState* Voter, FPrimaryAssetId ScenarioId);

	/** Replication notification for voting state */
	UFUNCTION()
	void OnRep_VotingActive();

	/** Handle vote completion */
	virtual void CompleteVoting();

	/** Get the scenario subsystem */
	UScenarioInstanceSubsystem* GetScenarioSubsystem() const;

private:
	// ============================================================================
	// INTERNAL STATE
	// ============================================================================

	/** Map of player votes (server only) */
	TMap<APlayerState*, FScenarioVoteEntry> PlayerVotes;

	/** Cached scenario subsystem */
	UPROPERTY(Transient)
	TObjectPtr<UScenarioInstanceSubsystem> CachedScenarioSubsystem;
};
