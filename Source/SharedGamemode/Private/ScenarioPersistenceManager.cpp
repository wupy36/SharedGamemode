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

#include "ScenarioPersistenceManager.h"
#include "ScenarioInstanceSubsystem.h"
#include "GameplayScenario.h"
#include "Engine/AssetManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenarioPersistence, Log, All);

UScenarioPersistenceManager::UScenarioPersistenceManager()
{
}

void UScenarioPersistenceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load existing stats
	LoadStats();

	// Register for scenario events
	if (UScenarioInstanceSubsystem* ScenarioSubsystem = GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>())
	{
		ScenarioSubsystem->OnScenarioActivated.AddUObject(this, &UScenarioPersistenceManager::OnScenarioActivated);
	}

	UE_LOG(LogScenarioPersistence, Log, TEXT("ScenarioPersistenceManager initialized. Loaded %d scenario stats."), ScenarioStatsMap.Num());
}

void UScenarioPersistenceManager::Deinitialize()
{
	// Auto-save on shutdown
	if (bAutoSave)
	{
		SaveStats();
	}

	Super::Deinitialize();
}

// ============================================================================
// STAT TRACKING
// ============================================================================

void UScenarioPersistenceManager::RecordScenarioPlayed(FPrimaryAssetId ScenarioId, int32 NumPlayers)
{
	if (!ScenarioId.IsValid())
	{
		return;
	}

	FScenarioStats& Stats = GetOrCreateStats(ScenarioId);

	// Update play count
	Stats.TimesPlayed++;

	// Update average player count
	if (Stats.TimesPlayed == 1)
	{
		Stats.AveragePlayerCount = NumPlayers;
	}
	else
	{
		Stats.AveragePlayerCount = (Stats.AveragePlayerCount * (Stats.TimesPlayed - 1) + NumPlayers) / Stats.TimesPlayed;
	}

	// Update timestamp
	Stats.LastPlayedTimestamp = FDateTime::UtcNow().ToUnixTimestamp();

	UE_LOG(LogScenarioPersistence, Verbose, TEXT("Recorded play for %s (Total: %d plays, Avg Players: %.1f)"),
		*ScenarioId.ToString(), Stats.TimesPlayed, Stats.AveragePlayerCount);

	CheckAutoSave();
}

void UScenarioPersistenceManager::RecordScenarioVotes(FPrimaryAssetId ScenarioId, int32 NumVotes)
{
	if (!ScenarioId.IsValid() || NumVotes <= 0)
	{
		return;
	}

	FScenarioStats& Stats = GetOrCreateStats(ScenarioId);
	Stats.TotalVotes += NumVotes;

	UE_LOG(LogScenarioPersistence, Verbose, TEXT("Recorded %d votes for %s (Total: %d)"),
		NumVotes, *ScenarioId.ToString(), Stats.TotalVotes);

	CheckAutoSave();
}

bool UScenarioPersistenceManager::GetScenarioStats(FPrimaryAssetId ScenarioId, FScenarioStats& OutStats) const
{
	if (const FScenarioStats* Found = ScenarioStatsMap.Find(ScenarioId))
	{
		OutStats = *Found;
		return true;
	}

	return false;
}

TArray<FScenarioStats> UScenarioPersistenceManager::GetAllStats() const
{
	TArray<FScenarioStats> AllStats;
	ScenarioStatsMap.GenerateValueArray(AllStats);
	return AllStats;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

bool UScenarioPersistenceManager::SaveStats()
{
	FString FilePath = GetPersistencePath();

	// Create JSON object
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> StatsArray;

	for (const auto& Pair : ScenarioStatsMap)
	{
		const FScenarioStats& Stats = Pair.Value;

		TSharedPtr<FJsonObject> StatsObject = MakeShared<FJsonObject>();
		StatsObject->SetStringField(TEXT("ScenarioId"), Stats.ScenarioId.ToString());
		StatsObject->SetNumberField(TEXT("TimesPlayed"), Stats.TimesPlayed);
		StatsObject->SetNumberField(TEXT("TotalVotes"), Stats.TotalVotes);
		StatsObject->SetNumberField(TEXT("LastPlayedTimestamp"), Stats.LastPlayedTimestamp);
		StatsObject->SetNumberField(TEXT("AveragePlayerCount"), Stats.AveragePlayerCount);
		StatsObject->SetNumberField(TEXT("RotationWeight"), Stats.RotationWeight);

		StatsArray.Add(MakeShared<FJsonValueObject>(StatsObject));
	}

	RootObject->SetArrayField(TEXT("ScenarioStats"), StatsArray);
	RootObject->SetStringField(TEXT("Version"), TEXT("1.0"));
	RootObject->SetNumberField(TEXT("SaveTimestamp"), FDateTime::UtcNow().ToUnixTimestamp());

	// Serialize to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogScenarioPersistence, Error, TEXT("Failed to serialize scenario stats to JSON"));
		return false;
	}

	// Write to file
	if (!FFileHelper::SaveStringToFile(OutputString, *FilePath))
	{
		UE_LOG(LogScenarioPersistence, Error, TEXT("Failed to save scenario stats to file: %s"), *FilePath);
		return false;
	}

	LastSaveTime = FPlatformTime::Seconds();
	UE_LOG(LogScenarioPersistence, Log, TEXT("Saved %d scenario stats to: %s"), ScenarioStatsMap.Num(), *FilePath);
	return true;
}

bool UScenarioPersistenceManager::LoadStats()
{
	FString FilePath = GetPersistencePath();

	// Check if file exists
	if (!FPaths::FileExists(FilePath))
	{
		UE_LOG(LogScenarioPersistence, Log, TEXT("No existing stats file found at: %s"), *FilePath);
		return false;
	}

	// Load file
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		UE_LOG(LogScenarioPersistence, Error, TEXT("Failed to load scenario stats from file: %s"), *FilePath);
		return false;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogScenarioPersistence, Error, TEXT("Failed to parse scenario stats JSON"));
		return false;
	}

	// Clear existing stats
	ScenarioStatsMap.Empty();

	// Load stats array
	const TArray<TSharedPtr<FJsonValue>>* StatsArray;
	if (!RootObject->TryGetArrayField(TEXT("ScenarioStats"), StatsArray))
	{
		UE_LOG(LogScenarioPersistence, Error, TEXT("Failed to find ScenarioStats array in JSON"));
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *StatsArray)
	{
		const TSharedPtr<FJsonObject>& StatsObject = Value->AsObject();
		if (!StatsObject.IsValid())
		{
			continue;
		}

		FScenarioStats Stats;

		FString ScenarioIdStr;
		if (StatsObject->TryGetStringField(TEXT("ScenarioId"), ScenarioIdStr))
		{
			Stats.ScenarioId = FPrimaryAssetId::FromString(ScenarioIdStr);
		}

		StatsObject->TryGetNumberField(TEXT("TimesPlayed"), Stats.TimesPlayed);
		StatsObject->TryGetNumberField(TEXT("TotalVotes"), Stats.TotalVotes);

		int64 Timestamp;
		if (StatsObject->TryGetNumberField(TEXT("LastPlayedTimestamp"), Timestamp))
		{
			Stats.LastPlayedTimestamp = Timestamp;
		}

		double AvgPlayers;
		if (StatsObject->TryGetNumberField(TEXT("AveragePlayerCount"), AvgPlayers))
		{
			Stats.AveragePlayerCount = AvgPlayers;
		}

		double RotWeight;
		if (StatsObject->TryGetNumberField(TEXT("RotationWeight"), RotWeight))
		{
			Stats.RotationWeight = RotWeight;
		}

		if (Stats.ScenarioId.IsValid())
		{
			ScenarioStatsMap.Add(Stats.ScenarioId, Stats);
		}
	}

	UE_LOG(LogScenarioPersistence, Log, TEXT("Loaded %d scenario stats from: %s"), ScenarioStatsMap.Num(), *FilePath);
	return true;
}

void UScenarioPersistenceManager::ResetStats()
{
	ScenarioStatsMap.Empty();
	UE_LOG(LogScenarioPersistence, Log, TEXT("Reset all scenario stats"));
}

// ============================================================================
// SCENARIO SELECTION
// ============================================================================

TArray<FPrimaryAssetId> UScenarioPersistenceManager::SelectScenariosWeighted(int32 NumScenarios, const FGameplayTagQuery& FilterTags)
{
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AllScenarios;
	Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

	// Filter by tags if query is valid
	if (!FilterTags.IsEmpty())
	{
		TArray<FPrimaryAssetId> FilteredScenarios;
		for (const FPrimaryAssetId& ScenarioId : AllScenarios)
		{
			UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);
			if (Scenario && FilterTags.Matches(Scenario->ScenarioTags))
			{
				FilteredScenarios.Add(ScenarioId);
			}
		}
		AllScenarios = FilteredScenarios;
	}

	if (AllScenarios.Num() == 0)
	{
		UE_LOG(LogScenarioPersistence, Warning, TEXT("No scenarios available for weighted selection"));
		return TArray<FPrimaryAssetId>();
	}

	// Calculate weights
	TArray<float> Weights;
	float TotalWeight = 0.0f;

	for (const FPrimaryAssetId& ScenarioId : AllScenarios)
	{
		float Weight = CalculateRotationWeight(ScenarioId);
		Weights.Add(Weight);
		TotalWeight += Weight;
	}

	// Select scenarios using weighted random
	TArray<FPrimaryAssetId> SelectedScenarios;
	TArray<bool> Selected;
	Selected.SetNumZeroed(AllScenarios.Num());

	int32 NumToSelect = FMath::Min(NumScenarios, AllScenarios.Num());

	for (int32 i = 0; i < NumToSelect; i++)
	{
		// Random weighted selection
		float Random = FMath::FRand() * TotalWeight;
		float Accumulated = 0.0f;

		for (int32 j = 0; j < AllScenarios.Num(); j++)
		{
			if (Selected[j])
			{
				continue;
			}

			Accumulated += Weights[j];
			if (Random <= Accumulated)
			{
				SelectedScenarios.Add(AllScenarios[j]);
				Selected[j] = true;
				TotalWeight -= Weights[j];
				break;
			}
		}
	}

	UE_LOG(LogScenarioPersistence, Verbose, TEXT("Selected %d scenarios using weighted selection"), SelectedScenarios.Num());
	return SelectedScenarios;
}

TArray<FPrimaryAssetId> UScenarioPersistenceManager::GetLeastRecentlyPlayed(int32 NumScenarios, const FGameplayTagQuery& FilterTags)
{
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AllScenarios;
	Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

	// Filter by tags if query is valid
	if (!FilterTags.IsEmpty())
	{
		TArray<FPrimaryAssetId> FilteredScenarios;
		for (const FPrimaryAssetId& ScenarioId : AllScenarios)
		{
			UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);
			if (Scenario && FilterTags.Matches(Scenario->ScenarioTags))
			{
				FilteredScenarios.Add(ScenarioId);
			}
		}
		AllScenarios = FilteredScenarios;
	}

	// Sort by last played timestamp (ascending = oldest first)
	AllScenarios.Sort([this](const FPrimaryAssetId& A, const FPrimaryAssetId& B) {
		int64 TimeA = 0;
		int64 TimeB = 0;

		if (const FScenarioStats* StatsA = ScenarioStatsMap.Find(A))
		{
			TimeA = StatsA->LastPlayedTimestamp;
		}
		if (const FScenarioStats* StatsB = ScenarioStatsMap.Find(B))
		{
			TimeB = StatsB->LastPlayedTimestamp;
		}

		return TimeA < TimeB;
	});

	// Return first N
	int32 NumToReturn = FMath::Min(NumScenarios, AllScenarios.Num());
	TArray<FPrimaryAssetId> Result;
	Result.Append(AllScenarios.GetData(), NumToReturn);

	return Result;
}

TArray<FPrimaryAssetId> UScenarioPersistenceManager::GetMostPopular(int32 NumScenarios, const FGameplayTagQuery& FilterTags)
{
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AllScenarios;
	Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

	// Filter by tags if query is valid
	if (!FilterTags.IsEmpty())
	{
		TArray<FPrimaryAssetId> FilteredScenarios;
		for (const FPrimaryAssetId& ScenarioId : AllScenarios)
		{
			UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);
			if (Scenario && FilterTags.Matches(Scenario->ScenarioTags))
			{
				FilteredScenarios.Add(ScenarioId);
			}
		}
		AllScenarios = FilteredScenarios;
	}

	// Sort by total votes (descending = most popular first)
	AllScenarios.Sort([this](const FPrimaryAssetId& A, const FPrimaryAssetId& B) {
		int32 VotesA = 0;
		int32 VotesB = 0;

		if (const FScenarioStats* StatsA = ScenarioStatsMap.Find(A))
		{
			VotesA = StatsA->TotalVotes;
		}
		if (const FScenarioStats* StatsB = ScenarioStatsMap.Find(B))
		{
			VotesB = StatsB->TotalVotes;
		}

		return VotesA > VotesB;
	});

	// Return first N
	int32 NumToReturn = FMath::Min(NumScenarios, AllScenarios.Num());
	TArray<FPrimaryAssetId> Result;
	Result.Append(AllScenarios.GetData(), NumToReturn);

	return Result;
}

// ============================================================================
// EXTENSIBILITY POINTS
// ============================================================================

float UScenarioPersistenceManager::CalculateRotationWeight_Implementation(FPrimaryAssetId ScenarioId) const
{
	const FScenarioStats* Stats = ScenarioStatsMap.Find(ScenarioId);
	if (!Stats)
	{
		// No stats = default weight
		return 1.0f;
	}

	float Weight = Stats->RotationWeight;

	// Apply recency penalty
	int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();
	int64 TimeSinceLastPlayed = CurrentTime - Stats->LastPlayedTimestamp;

	if (TimeSinceLastPlayed < RecencyPenaltyWindow)
	{
		// Recently played = lower weight
		Weight *= RecencyPenalty;
	}

	// Apply popularity bonus
	if (Stats->TotalVotes >= PopularityThreshold)
	{
		Weight *= PopularityBonus;
	}

	return FMath::Max(Weight, 0.1f); // Minimum weight of 0.1
}

FString UScenarioPersistenceManager::GetPersistencePath_Implementation() const
{
	return FPaths::ProjectSavedDir() / PersistenceFileName;
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void UScenarioPersistenceManager::OnScenarioActivated(UGameplayScenario* Scenario)
{
	if (!Scenario)
	{
		return;
	}

	// Count current players
	int32 NumPlayers = 0;
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			NumPlayers = GameState->PlayerArray.Num();
		}
	}

	RecordScenarioPlayed(Scenario->GetPrimaryAssetId(), NumPlayers);
}

FScenarioStats& UScenarioPersistenceManager::GetOrCreateStats(FPrimaryAssetId ScenarioId)
{
	if (FScenarioStats* Found = ScenarioStatsMap.Find(ScenarioId))
	{
		return *Found;
	}

	FScenarioStats NewStats(ScenarioId);
	return ScenarioStatsMap.Add(ScenarioId, NewStats);
}

void UScenarioPersistenceManager::CheckAutoSave()
{
	if (!bAutoSave)
	{
		return;
	}

	double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastSaveTime >= AutoSaveInterval)
	{
		SaveStats();
	}
}
