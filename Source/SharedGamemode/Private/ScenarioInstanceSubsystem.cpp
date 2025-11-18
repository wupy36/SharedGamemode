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


#include "ScenarioInstanceSubsystem.h"
#include "Engine.h"

#include "GameplayScenario.h"
#include "GameplayScenarioAction.h"

#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/LevelStreamingDynamic.h"

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"


DEFINE_LOG_CATEGORY_STATIC(LogGameplayScenario, Log, All);

UScenarioInstanceSubsystem::UScenarioInstanceSubsystem()
	: Super()
{
	bBecomeListenServerFromStandalone = true;
	MapTransitionScenario = nullptr;
}

void UScenarioInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// ============================================================================
	// CONSOLE COMMAND: StartScenario
	// Purpose: Begin a scenario, changing maps if needed
	// Usage: StartScenario <PrimaryAssetId>
	// Example: StartScenario GameplayScenario'/Game/Scenarios/SC_MyScenario.SC_MyScenario'
	// ============================================================================
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("StartScenario"),
		TEXT("Begin a Scenario, Changing maps if needed"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				if (Args.Num() != 1)
				{
					Ar.Logf(TEXT("Error: Expected one parameter to StartScenario"));
					Ar.Logf(TEXT("Usage: StartScenario <PrimaryAssetId>"));
					Ar.Logf(TEXT("Example: StartScenario GameplayScenario'/Game/Scenarios/SC_MyScenario.SC_MyScenario'"));
					return;
				}

				//Parse the primary asset id
				FPrimaryAssetId ScenarioAsset = FPrimaryAssetId::FromString(Args[0]);

				if (!ScenarioAsset.IsValid())
				{
					Ar.Logf(TEXT("Error: Asset Id '%s' is not valid"), *Args[0]);
					Ar.Logf(TEXT("Expected format: GameplayScenario'/Path/To/Asset.AssetName'"));
					return;
				}
				UAssetManager& Manager = UAssetManager::Get();

				FSoftObjectPath Path = Manager.GetPrimaryAssetPath(ScenarioAsset);

				if (!Path.IsValid())
				{
					Ar.Logf(TEXT("Error: Scenario '%s' does not exist in asset registry"), *ScenarioAsset.ToString());
					Ar.Logf(TEXT("Tip: Use 'ListScenarios' to see available scenarios"));
					return;
				}

				Ar.Logf(TEXT("Loading scenario: %s..."), *ScenarioAsset.ToString());

				auto Handle = Manager.LoadPrimaryAsset(ScenarioAsset);

				if (Handle.IsValid())
				{
					Handle->WaitUntilComplete(10);
				}

				UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioAsset);

				if (!IsValid(Scenario))
				{
					Ar.Logf(TEXT("Error: Failed to load scenario '%s'"), *ScenarioAsset.ToString());
					return;
				}

				UE_LOG(LogGameplayScenario, Log, TEXT("ScenarioSubsystem: Starting scenario %s"), *GetNameSafe(Scenario));
				Ar.Logf(TEXT("Successfully loaded scenario: %s"), *GetNameSafe(Scenario));

				SetPendingScenario(Scenario);
				TransitionToPendingScenario(true);
			}),
		ECVF_Default
		);

	// ============================================================================
	// CONSOLE COMMAND: ListScenarios
	// Purpose: List all available scenarios in the asset registry
	// Usage: ListScenarios [Filter]
	// Example: ListScenarios Conquest
	// ============================================================================
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("ListScenarios"),
		TEXT("List all available scenarios. Optional filter parameter."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				UAssetManager& Manager = UAssetManager::Get();
				TArray<FPrimaryAssetId> ScenarioAssets;
				Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), ScenarioAssets);

				FString Filter;
				if (Args.Num() > 0)
				{
					Filter = Args[0];
					Ar.Logf(TEXT("Available Scenarios (Filter: %s):"), *Filter);
				}
				else
				{
					Ar.Logf(TEXT("Available Scenarios (%d total):"), ScenarioAssets.Num());
				}

				Ar.Logf(TEXT("----------------------------------------"));

				int32 DisplayCount = 0;
				for (const FPrimaryAssetId& AssetId : ScenarioAssets)
				{
					FString AssetName = AssetId.ToString();

					// Apply filter if provided
					if (!Filter.IsEmpty() && !AssetName.Contains(Filter))
					{
						continue;
					}

					// Try to get asset data from registry
					FAssetData AssetData;
					Manager.GetPrimaryAssetData(AssetId, AssetData);

					FString Description = TEXT("(No description)");
					FString Name = AssetId.PrimaryAssetName.ToString();

					if (AssetData.IsValid())
					{
						FString DescTag;
						if (AssetData.GetTagValue("Description", DescTag))
						{
							Description = DescTag;
						}

						FString NameTag;
						if (AssetData.GetTagValue("Name", NameTag))
						{
							Name = NameTag;
						}
					}

					Ar.Logf(TEXT("  [%d] %s"), DisplayCount + 1, *AssetName);
					Ar.Logf(TEXT("      Name: %s"), *Name);
					Ar.Logf(TEXT("      Desc: %s"), *Description);
					DisplayCount++;
				}

				if (DisplayCount == 0)
				{
					if (!Filter.IsEmpty())
					{
						Ar.Logf(TEXT("No scenarios found matching filter '%s'"), *Filter);
					}
					else
					{
						Ar.Logf(TEXT("No scenarios found. Make sure GameplayScenario assets are properly registered."));
					}
				}
				else
				{
					Ar.Logf(TEXT("----------------------------------------"));
					Ar.Logf(TEXT("Total displayed: %d"), DisplayCount);
				}
			}),
		ECVF_Default
		);

	// ============================================================================
	// CONSOLE COMMAND: GetActiveScenarios
	// Purpose: Show all currently active scenarios
	// Usage: GetActiveScenarios
	// ============================================================================
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("GetActiveScenarios"),
		TEXT("Show all currently active scenarios"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				if (ActiveScenarios.Num() == 0)
				{
					Ar.Logf(TEXT("No scenarios currently active"));
					return;
				}

				Ar.Logf(TEXT("Active Scenarios (%d):"), ActiveScenarios.Num());
				Ar.Logf(TEXT("----------------------------------------"));

				for (int32 i = 0; i < ActiveScenarios.Num(); i++)
				{
					UGameplayScenario* Scenario = ActiveScenarios[i];
					if (IsValid(Scenario))
					{
						FPrimaryAssetId AssetId = Scenario->GetPrimaryAssetId();
						Ar.Logf(TEXT("  [%d] %s"), i + 1, *GetNameSafe(Scenario));
						Ar.Logf(TEXT("      Asset: %s"), *AssetId.ToString());
						Ar.Logf(TEXT("      Actions: %d"), Scenario->ScenarioActions.Num());
					}
					else
					{
						Ar.Logf(TEXT("  [%d] (Invalid Scenario)"), i + 1);
					}
				}
				Ar.Logf(TEXT("----------------------------------------"));
			}),
		ECVF_Default
		);

	// ============================================================================
	// CONSOLE COMMAND: DeactivateScenario
	// Purpose: Deactivate a specific scenario by asset ID
	// Usage: DeactivateScenario <PrimaryAssetId>
	// Example: DeactivateScenario GameplayScenario'/Game/Scenarios/SC_MyScenario.SC_MyScenario'
	// ============================================================================
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("DeactivateScenario"),
		TEXT("Deactivate a specific scenario"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				if (Args.Num() != 1)
				{
					Ar.Logf(TEXT("Error: Expected one parameter"));
					Ar.Logf(TEXT("Usage: DeactivateScenario <PrimaryAssetId>"));
					Ar.Logf(TEXT("Tip: Use 'GetActiveScenarios' to see currently active scenarios"));
					return;
				}

				FPrimaryAssetId ScenarioAsset = FPrimaryAssetId::FromString(Args[0]);

				if (!ScenarioAsset.IsValid())
				{
					Ar.Logf(TEXT("Error: Asset Id '%s' is not valid"), *Args[0]);
					return;
				}

				UGameplayScenario** FoundScenario = ActiveScenarios.FindByPredicate([ScenarioAsset](UGameplayScenario* Scenario) {
					return IsValid(Scenario) && Scenario->GetPrimaryAssetId() == ScenarioAsset;
				});

				if (FoundScenario && IsValid(*FoundScenario))
				{
					UE_LOG(LogGameplayScenario, Log, TEXT("ScenarioSubsystem: Deactivating scenario %s via console command"), *GetNameSafe(*FoundScenario));
					Ar.Logf(TEXT("Deactivating scenario: %s"), *GetNameSafe(*FoundScenario));
					DeactivateScenario(*FoundScenario);
					Ar.Logf(TEXT("Successfully deactivated scenario"));
				}
				else
				{
					Ar.Logf(TEXT("Error: Scenario '%s' is not currently active"), *ScenarioAsset.ToString());
					Ar.Logf(TEXT("Tip: Use 'GetActiveScenarios' to see active scenarios"));
				}
			}),
		ECVF_Default
		);

	// ============================================================================
	// CONSOLE COMMAND: TearDownScenarios
	// Purpose: Deactivate all currently active scenarios
	// Usage: TearDownScenarios
	// ============================================================================
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("TearDownScenarios"),
		TEXT("Deactivate all currently active scenarios"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([this](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				if (ActiveScenarios.Num() == 0)
				{
					Ar.Logf(TEXT("No scenarios currently active"));
					return;
				}

				int32 NumScenarios = ActiveScenarios.Num();
				Ar.Logf(TEXT("Tearing down %d active scenario(s)..."), NumScenarios);

				UE_LOG(LogGameplayScenario, Log, TEXT("ScenarioSubsystem: Tearing down all scenarios via console command"));
				TearDownActiveScenarios();

				Ar.Logf(TEXT("Successfully deactivated %d scenario(s)"), NumScenarios);
			}),
		ECVF_Default
		);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
}

void UScenarioInstanceSubsystem::SetPendingScenario(UGameplayScenario* Scenairo)
{
	PendingScenario = Scenairo;
}

void UScenarioInstanceSubsystem::TransitionToPendingScenario(bool bForce)
{
	if(!IsValid(PendingScenario))
	{
		UE_LOG(LogGameplayScenario, Warning, TEXT("ScenarioSubsystem: TransitionToPendingScenario called with no pending scenario"));
		return;
	}

	UGameplayScenario* Scenario = PendingScenario;
	PendingScenario = nullptr;

	StartActivatingScenario(Scenario, bForce);
}


void UScenarioInstanceSubsystem::PreActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce)
{
	UAssetManager& Manager = UAssetManager::Get();

	FSoftObjectPath Path = Manager.GetPrimaryAssetPath(ScenarioAsset);

	if (!ensure(Path.IsValid()))
	{
		return;
	}

	UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioAsset);

	if (!IsValid(Scenario))
	{
		auto LoadHandle = Manager.LoadPrimaryAsset(ScenarioAsset);
		//TODO (when?): Async This.  We need some future/await behavior here.
		if (LoadHandle.IsValid())
		{
			LoadHandle->WaitUntilComplete();
		}

		Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioAsset);
	}

	if (IsValid(Scenario))
	{
		PreActivateScenario(Scenario, bForce);
	}
}

void UScenarioInstanceSubsystem::PreActivateScenario(UGameplayScenario* Scenario, bool bForce)
{
	if (!IsValid(Scenario))
	{
		return;
	}
	if (!bForce && IsScenarioActive(Scenario))
	{
		return;
	}

	UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: PreActivating Scenario %s"), *GetNameSafe(Scenario));

	Scenario->PreActivateScenario(this);
}

void UScenarioInstanceSubsystem::ActivateScenario(UGameplayScenario* Scenario, bool bForce)
{
	if (!IsValid(Scenario))
	{
		return;
	}
	if (!bForce && IsScenarioActive(Scenario))
	{
		return;
	}

	UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: Activating Scenario %s"), *GetNameSafe(Scenario));

	ActiveScenarios.Add(Scenario);

	//Activate the game actions
	for (UGameplayScenarioAction* Action : Scenario->ScenarioActions)
	{
		Action->OnScenarioActivated(this);
	}
	OnScenarioActivated.Broadcast(Scenario);
}

void UScenarioInstanceSubsystem::ActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce)
{
	UAssetManager& Manager = UAssetManager::Get();

	FSoftObjectPath Path = Manager.GetPrimaryAssetPath(ScenarioAsset);

	if (!ensure(Path.IsValid()))
	{
		return;
	}

	UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioAsset);

	if (!IsValid(Scenario))
	{
		auto LoadHandle = Manager.LoadPrimaryAsset(ScenarioAsset);
		//TODO (when?): Async This.  We need some future/await behavior here.
		if (LoadHandle.IsValid())
		{
			LoadHandle->WaitUntilComplete();
		}

		Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioAsset);
	}

	if (IsValid(Scenario))
	{
		ActivateScenario(Scenario, bForce);
	}
}

void UScenarioInstanceSubsystem::DeactivateScenario(UGameplayScenario* Scenario)
{
	if (!IsScenarioActive(Scenario))
	{
		return;
	}

	UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: Deactivating Scenario %s"), *GetNameSafe(Scenario));

	Scenario->DeactivateScenario(this);

	ActiveScenarios.RemoveSwap(Scenario);

	OnScenarioDeactivated.Broadcast(Scenario);
}

void UScenarioInstanceSubsystem::DeactivateScenario(FPrimaryAssetId ScenarioAsset)
{
	UGameplayScenario** SearchedScenario = ActiveScenarios.FindByPredicate([ScenarioAsset](UGameplayScenario* Scenario) {
		if (Scenario->GetPrimaryAssetId() == ScenarioAsset)
		{
			return true;
		}
		return false;
	});

	if (SearchedScenario)
	{
		DeactivateScenario(*SearchedScenario);
	}
}

void UScenarioInstanceSubsystem::TearDownActiveScenarios()
{
	UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: Tearing Down all active scenarios"));
	for(UGameplayScenario* Scenario : ActiveScenarios)
	{
		if (IsValid(Scenario))
		{
			Scenario->DeactivateScenario(this, true);
			OnScenarioDeactivated.Broadcast(Scenario);
		}
	}
	ActiveScenarios.Empty();
}

bool UScenarioInstanceSubsystem::IsScenarioActive(UGameplayScenario* Scenario) const
{
	if (ActiveScenarios.Contains(Scenario))
	{
		return true;
	}

	return false;
}

void UScenarioInstanceSubsystem::OnPostLoadMap(UWorld* World)
{
	if (IsValid(MapTransitionScenario))
	{
		UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: After Map Load, Finishing Activating %s"), *GetNameSafe(PendingScenario));

		FinishActivatingScenario(MapTransitionScenario, true);
		MapTransitionScenario = nullptr;
	}

	if (IsValid(PendingScenario))
	{
		UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem: After Map Load, Transiting to pending Scenario %s"), *GetNameSafe(PendingScenario));

		TransitionToPendingScenario();
	}
	
}

void UScenarioInstanceSubsystem::OnPreLoadMap(const FString& MapName)
{
	//If we're about to transition maps, deactivate all scenarios
	TearDownActiveScenarios();
}
void UScenarioInstanceSubsystem::StartActivatingScenario(UGameplayScenario* Scenario, bool bForce)
{
	if (Scenario->Map.IsValid())
	{
		TearDownActiveScenarios();
	}

	PreActivateScenario(Scenario, bForce);

	if (Scenario->Map.IsValid())
	{
		UE_LOG(LogGameplayScenario, Verbose, TEXT("ScenarioSubsystem:Transiting to world %s for scenario %s"), *Scenario->Map.ToString(), *GetNameSafe(Scenario));

		TransitionToWorld(Scenario->Map);

		//Store off the pending scenario so we can activate it once the map is loaded
		MapTransitionScenario = Scenario;

		//Await the level change to try to transition again
		return;
	}

	FinishActivatingScenario(Scenario, bForce);
}

void UScenarioInstanceSubsystem::FinishActivatingScenario(UGameplayScenario* Scenario, bool bForce)
{
	ActivateScenario(Scenario, bForce);
}

void UScenarioInstanceSubsystem::TransitionToWorld(FPrimaryAssetId WorldAsset)
{
	const UWorld* const World = GetGameInstance()->GetWorld();


	const bool bIsClient = World->GetNetMode() == NM_Client;

	//Don't transition if we're the client.  We're probably at this world
	if (bIsClient)
	{
		return;
	}

	const bool bIsDedicatedServer = IsRunningDedicatedServer();
	const bool bIsListenServer = World->GetNetMode() == NM_ListenServer; 
	const bool bIsStandalone = World->GetNetMode() == NM_Standalone;

	FURL NewMapURL = FURL(*WorldAsset.PrimaryAssetName.ToString());

	if (bIsStandalone || bIsListenServer)
	{
		NewMapURL.AddOption(TEXT("listen"));
	}

	//Travel to the new scenario world
	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		if (GameMode->CanServerTravel(NewMapURL.ToString(), false))
		{
			GameMode->ProcessServerTravel(NewMapURL.ToString(), false);
		}
	}
}

