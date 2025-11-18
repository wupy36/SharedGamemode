# Shared Gamemode Plugin - Complete Documentation

## Table of Contents

1. [Quick Start Guide](#quick-start-guide)
2. [Basic Concepts](#basic-concepts)
3. [Creating Your First Scenario](#creating-your-first-scenario)
4. [Dedicated Server Setup](#dedicated-server-setup)
5. [Advanced Features](#advanced-features)
6. [Extending the Plugin](#extending-the-plugin)
7. [API Reference](#api-reference)
8. [Examples & Tutorials](#examples--tutorials)
9. [Performance Optimization](#performance-optimization)
10. [Troubleshooting](#troubleshooting)

---

# Quick Start Guide

## Installation (5 minutes)

### Step 1: Install the Plugin

1. Copy the `SharedGamemode` folder to your project's `Plugins/GameFeatures/` directory
2. Open your project in Unreal Engine
3. Go to **Edit → Plugins**
4. Verify that **Shared Gamemode** is enabled
5. Restart the editor if prompted

### Step 2: Enable Required Plugins

Ensure these plugins are enabled in your project:

- ✅ Gameplay Abilities
- ✅ Modular Gameplay
- ✅ Game Features

### Step 3: Create Your First Scenario

1. **Create a new Game Feature Plugin** (optional, or use existing one):
   - Right-click in Content Browser → **New Folder** → `MyGameFeatures`
   - Right-click folder → **Create Advanced Asset** → **Game Feature**

2. **Create a Gameplay Scenario Data Asset**:
   - Right-click in Content Browser → **Miscellaneous** → **Data Asset**
   - Select **GameplayScenario** as the class
   - Name it `SC_MyFirstScenario`

3. **Configure the Scenario**:
   - Open the data asset
   - Set **Name** to "My First Scenario"
   - Set **Description** to "A simple test scenario"
   - (Optional) Add a **Map** reference if you want to load a specific level

### Step 4: Activate Your Scenario

**From Console:**
```
StartScenario GameplayScenario'/Game/MyScenarios/SC_MyFirstScenario.SC_MyFirstScenario'
```

**From Blueprint:**
```cpp
// Get the Scenario Subsystem
UScenarioInstanceSubsystem* ScenarioSystem =
    GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

// Load and activate your scenario
UGameplayScenario* MyScenario = LoadObject<UGameplayScenario>(
    nullptr,
    TEXT("/Game/MyScenarios/SC_MyFirstScenario.SC_MyFirstScenario")
);

ScenarioSystem->SetPendingScenario(MyScenario);
ScenarioSystem->TransitionToPendingScenario(true);
```

**From C++:**
```cpp
#include "ScenarioInstanceSubsystem.h"
#include "GameplayScenario.h"

void AMyGameMode::StartScenario()
{
    UScenarioInstanceSubsystem* ScenarioSystem =
        GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

    FPrimaryAssetId ScenarioAsset = FPrimaryAssetId::FromString(
        TEXT("GameplayScenario:/Game/MyScenarios/SC_MyFirstScenario.SC_MyFirstScenario")
    );

    ScenarioSystem->PreActivateScenario(ScenarioAsset, true);
    ScenarioSystem->ActivateScenario(ScenarioAsset, true);
}
```

---

## Quick Start: Dedicated Server Setup (10 minutes)

### Step 1: Configure Server Settings

1. Go to **Project Settings → Plugins → Shared Gamemode**
2. Configure basic settings:
   ```
   Default Scenario Asset: [Your default scenario]
   Auto Load Default Scenario: ✓
   Enable Scenario Voting: ✓
   Default Voting Duration: 30.0
   Enable Scenario Persistence: ✓
   ```

### Step 2: Test Server Commands

Launch your game as a dedicated server and test:

```
ListScenarios              # See all available scenarios
GetActiveScenarios         # View active scenarios
StartScenario [AssetId]    # Load a scenario
```

### Step 3: Add Voting to GameState (Optional)

**In Blueprint:**
1. Open your GameState Blueprint
2. Add Component → **Scenario Transition Component**
3. Set Component to **Replicate**
4. Configure voting settings in the Details panel

**In C++:**
```cpp
// MyGameState.h
#include "Components/ScenarioTransitionComponent.h"

UCLASS()
class AMyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMyGameState();

    UPROPERTY(Replicated, BlueprintReadOnly)
    UScenarioTransitionComponent* ScenarioTransition;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

// MyGameState.cpp
AMyGameState::AMyGameState()
{
    ScenarioTransition = CreateDefaultSubobject<UScenarioTransitionComponent>(TEXT("ScenarioTransition"));
    ScenarioTransition->SetIsReplicated(true);
}

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyGameState, ScenarioTransition);
}
```

### Step 4: Start Voting

```cpp
// When you want to start voting (e.g., end of round)
if (HasAuthority())
{
    ScenarioTransition->StartVoting();
}
```

🎉 **You're Done!** Your dedicated server is now set up with scenario management and voting.

---

# Basic Concepts

## What is a Scenario?

A **Scenario** is a data asset that defines a complete gameplay experience. Think of it as a package that can contain:

- 🗺️ **Map transitions** - Load a specific level
- 🧩 **Components** - Add gameplay components to actors
- 🎮 **Abilities** - Apply Gameplay Ability System (GAS) primitives
- 🌍 **Level streaming** - Stream in/out level instances
- 🔗 **Sub-scenarios** - Activate other scenarios for composition

### Example: Battle Royale Scenario

```
SC_BattleRoyale_Storm_Day
├── Activates: SC_BattleRoyale_Rules      (game rules)
├── Activates: SC_Map_Island_Day          (map + lighting)
├── Streams: L_BattleRoyale_POIs          (points of interest)
└── Adds Components: Storm shrinking logic
```

## Scenario Composition

One of the most powerful features is **scenario composition** - building complex scenarios from simple, reusable pieces.

### Traditional Approach (Monolithic)
```
SC_ConquestOnDesertAtNight
├── All conquest rules
├── Desert terrain
├── Night lighting
├── Spawn points
└── UI setup
```
**Problem:** Have to duplicate for every map/time combination!

### Shared Gamemode Approach (Compositional)
```
SC_ConquestOnDesertAtNight
├── ActivateScenario: SC_Conquest          (reusable gamemode)
├── ActivateScenario: SC_DesertTerrain     (reusable map)
└── ActivateScenario: SC_NightLighting     (reusable atmosphere)
```
**Benefit:** Mix and match! Easy to create SC_ConquestOnIslandAtDay by changing 2 references.

---

# Creating Your First Scenario

## Example 1: Simple Map Change Scenario

Let's create a scenario that loads a specific map with custom lighting.

### Step 1: Create the Scenario Asset
1. Content Browser → Right-click → **Miscellaneous** → **Data Asset**
2. Choose **GameplayScenario**
3. Name: `SC_ForestMap_Day`

### Step 2: Configure Basic Properties
```
Name: "Forest Map - Daytime"
Description: "Forest environment with dynamic daytime lighting"
Scenario Tags:
  - Map.Forest
  - Lighting.Day
  - Environment.Outdoor
```

### Step 3: Add a Map Action
1. In **Scenario Actions** array, click **+**
2. This will be auto-populated with the map change when you set the **Map** field
3. Set **Map** → `Map'/Game/Maps/L_Forest_Terrain.L_Forest_Terrain'`

### Step 4: Add Level Streaming
1. Add another action → **StreamLevelInstance**
2. Set **Streamed In Levels** → Add your atmosphere level
   - Example: `Map'/Game/Atmospheres/L_Day_Lighting.L_Day_Lighting'`

### Step 5: Test It
```
StartScenario GameplayScenario'/Game/Scenarios/SC_ForestMap_Day.SC_ForestMap_Day'
```

## Example 2: Gamemode Scenario with Components

Create a scenario that adds gameplay components without changing maps.

### Step 1: Create the Scenario
Name: `SC_Conquest_Rules`

### Step 2: Add Component Action
1. **Scenario Actions** → **+** → **Add Components**
2. Configure the component entries:

```cpp
Component List:
  [0]
    Actor Class: AMyTeamState
    Component Class: UTicketAttributeSetComponent
    bClient Component: false
    bServer Component: true
  [1]
    Actor Class: AMyGameMode
    Component Class: UConquestVictoryComponent
    bClient Component: false
    bServer Component: true
```

### Step 3: This Scenario Can Be Combined
```
SC_ConquestOnForest
├── ActivateScenario: SC_Conquest_Rules
└── ActivateScenario: SC_ForestMap_Day
```

## Example 3: Compositional Scenario

Build a complete experience from smaller pieces.

### Create the Sub-Scenarios First:

**SC_TeamDeathmatch** (just rules, no map)
```
Actions:
  - AddComponents: Add team scoring components
  - AddComponents: Add respawn logic
```

**SC_UrbanMap** (just terrain)
```
Map: L_Urban_Terrain
Actions:
  - StreamLevelInstance: [L_Urban_Buildings]
```

**SC_DayAtmosphere** (just lighting)
```
Actions:
  - StreamLevelInstance: [L_Atmosphere_Day]
```

### Then Combine Them:

**SC_TDM_Urban_Day** (complete experience)
```
Name: "Team Deathmatch - Urban - Daytime"
Actions:
  - ActivateScenario: SC_TeamDeathmatch
  - ActivateScenario: SC_UrbanMap
  - ActivateScenario: SC_DayAtmosphere
```

### Benefits:
- ✅ Create `SC_TDM_Forest_Day` by swapping one reference
- ✅ Create `SC_TDM_Urban_Night` by swapping one reference
- ✅ Reuse `SC_TeamDeathmatch` across all maps
- ✅ Easy to test individual components

---

# Dedicated Server Setup

## Configuration Options

### Project Settings (DefaultGame.ini)

```ini
[/Script/SharedGamemode.SharedGamemodeSettings]

; ============================================================================
; DEFAULT SCENARIO
; ============================================================================
; Scenario to load when server starts
DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_Default.SC_Default'"
bAutoLoadDefaultScenario=true

; ============================================================================
; VOTING SYSTEM
; ============================================================================
; Enable player voting for scenario changes
bEnableScenarioVoting=true
; Allow clients to request scenario changes
bAllowClientScenarioRequests=true
; Minimum players needed to start a vote
MinimumPlayersForVoting=2
; How long voting lasts (seconds)
DefaultVotingDuration=30.0
; Number of scenario options to show
DefaultNumScenarioOptions=3

; ============================================================================
; SCENARIO FILTERING
; ============================================================================
; Only allow these scenarios (leave empty to allow all)
+ScenarioWhitelist=(ScenarioId="GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'",Reason="Production Ready")
+ScenarioWhitelist=(ScenarioId="GameplayScenario'/Game/Scenarios/SC_TDM.SC_TDM'",Reason="Production Ready")

; Block these scenarios
+ScenarioBlacklist=(ScenarioId="GameplayScenario'/Game/Scenarios/SC_Test.SC_Test'",Reason="Testing only")
+ScenarioBlacklist=(ScenarioId="GameplayScenario'/Game/Scenarios/SC_Alpha.SC_Alpha'",Reason="Not ready for public")

; Require these tags (scenario must have ALL)
+RequiredScenarioTags="Environment.Production"

; Exclude these tags (scenario must have NONE)
+ExcludedScenarioTags="Debug.Test"
+ExcludedScenarioTags="WIP.Alpha"

; ============================================================================
; PERSISTENCE & ROTATION
; ============================================================================
; Enable stat tracking and rotation
bEnableScenarioPersistence=true
; Where to save stats (relative to Saved/)
PersistenceFileName="ScenarioStats.json"
; Auto-save stats periodically
bAutoSaveStats=true
; Save interval (seconds)
AutoSaveInterval=60.0

; ============================================================================
; SERVER OPTIONS
; ============================================================================
; Enable console commands
bEnableConsoleCommands=true
; Use seamless travel for map transitions
bUseSeamlessTravel=false
; Logging level (0=Errors, 1=Warnings, 2=Log, 3=Verbose)
LogVerbosity=2
```

### Command Line Overrides

Start your dedicated server with custom settings:

```bash
# Windows
MyGameServer.exe /Game/Maps/MainMenu?listen -log -server \
  -ScenarioVotingEnabled=true \
  -DefaultScenario="GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'" \
  -MinimumPlayersForVoting=4

# Linux
./MyGameServer.sh /Game/Maps/MainMenu?listen -log -server \
  -ScenarioVotingEnabled=true \
  -DefaultScenario="GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'"
```

### Per-Server Configuration Files

For multi-server setups, create separate config files:

**Server1.ini:**
```ini
[/Script/SharedGamemode.SharedGamemodeSettings]
DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'"
PersistenceFileName="Server1_Stats.json"
```

**Server2.ini:**
```ini
[/Script/SharedGamemode.SharedGamemodeSettings]
DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_TDM.SC_TDM'"
PersistenceFileName="Server2_Stats.json"
```

Then launch with:
```bash
MyGameServer.exe -ini=Server1.ini
```

## Console Command Reference

### Scenario Management Commands

#### StartScenario
```
StartScenario <PrimaryAssetId>
```
Loads and activates a scenario, changing maps if needed.

**Examples:**
```
StartScenario GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'
StartScenario GameplayScenario'/MyGameFeature/Scenarios/SC_Custom.SC_Custom'
```

**Output:**
```
Loading scenario: GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'...
Successfully loaded scenario: SC_Conquest
```

**Errors:**
```
Error: Asset Id 'InvalidAsset' is not valid
Error: Scenario 'GameplayScenario:/...' does not exist in asset registry
Error: Failed to load scenario
```

#### ListScenarios
```
ListScenarios [OptionalFilter]
```
Lists all available scenarios, optionally filtered by name.

**Examples:**
```
ListScenarios              # List all
ListScenarios Conquest     # List scenarios containing "Conquest"
ListScenarios Urban        # List scenarios containing "Urban"
```

**Output:**
```
Available Scenarios (Filter: Conquest):
----------------------------------------
  [1] GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'
      Name: Conquest Mode
      Desc: Capture points to win
  [2] GameplayScenario'/Game/Scenarios/SC_ConquestNight.SC_ConquestNight'
      Name: Conquest - Night
      Desc: Conquest mode with night lighting
----------------------------------------
Total displayed: 2
```

#### GetActiveScenarios
```
GetActiveScenarios
```
Shows all currently active scenarios.

**Output:**
```
Active Scenarios (3):
----------------------------------------
  [1] SC_Conquest
      Asset: GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'
      Actions: 5
  [2] SC_UrbanMap
      Asset: GameplayScenario'/Game/Scenarios/SC_UrbanMap.SC_UrbanMap'
      Actions: 2
  [3] SC_DayLighting
      Asset: GameplayScenario'/Game/Scenarios/SC_DayLighting.SC_DayLighting'
      Actions: 1
----------------------------------------
```

#### DeactivateScenario
```
DeactivateScenario <PrimaryAssetId>
```
Deactivates a specific scenario without affecting others.

**Examples:**
```
DeactivateScenario GameplayScenario'/Game/Scenarios/SC_DayLighting.SC_DayLighting'
```

**Output:**
```
Deactivating scenario: SC_DayLighting
Successfully deactivated scenario
```

#### TearDownScenarios
```
TearDownScenarios
```
Deactivates ALL active scenarios. Useful for server cleanup.

**Output:**
```
Tearing down 3 active scenario(s)...
Successfully deactivated 3 scenario(s)
```

## Automated Server Management

### Auto-Start Scenario on Server Launch

**Option 1: Project Settings**
```ini
[/Script/SharedGamemode.SharedGamemodeSettings]
DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_Default.SC_Default'"
bAutoLoadDefaultScenario=true
```

**Option 2: GameMode BeginPlay**
```cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        UScenarioInstanceSubsystem* ScenarioSystem =
            GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

        // Get default from settings
        const USharedGamemodeSettings* Settings = USharedGamemodeSettings::Get();
        if (Settings && Settings->bAutoLoadDefaultScenario)
        {
            ScenarioSystem->PreActivateScenario(Settings->DefaultScenarioAsset, true);
            ScenarioSystem->ActivateScenario(Settings->DefaultScenarioAsset, true);
        }
    }
}
```

### Scheduled Scenario Rotation

Automatically rotate scenarios on a timer:

```cpp
// MyGameMode.h
UCLASS()
class AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    float ScenarioRotationInterval = 1800.0f; // 30 minutes

    UPROPERTY(EditDefaultsOnly)
    TArray<FPrimaryAssetId> RotationScenarios;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void RotateToNextScenario();

private:
    FTimerHandle RotationTimerHandle;
    int32 CurrentRotationIndex = 0;
};

// MyGameMode.cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && RotationScenarios.Num() > 0)
    {
        // Start rotation timer
        GetWorldTimerManager().SetTimer(
            RotationTimerHandle,
            this,
            &AMyGameMode::RotateToNextScenario,
            ScenarioRotationInterval,
            true
        );
    }
}

void AMyGameMode::RotateToNextScenario()
{
    if (RotationScenarios.Num() == 0)
        return;

    UScenarioInstanceSubsystem* ScenarioSystem =
        GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

    // Get next scenario in rotation
    FPrimaryAssetId NextScenario = RotationScenarios[CurrentRotationIndex];
    CurrentRotationIndex = (CurrentRotationIndex + 1) % RotationScenarios.Num();

    UE_LOG(LogTemp, Log, TEXT("Auto-rotating to scenario: %s"), *NextScenario.ToString());

    ScenarioSystem->PreActivateScenario(NextScenario, true);
    ScenarioSystem->ActivateScenario(NextScenario, true);
}
```

---

# Advanced Features

## Voting System Deep Dive

### Basic Voting Flow

1. **Server starts voting** → `ScenarioTransition->StartVoting()`
2. **Scenarios are selected** → Via `SelectScenarioOptions()`
3. **Voting period begins** → Timer starts counting down
4. **Clients cast votes** → `CastVote(ScenarioId)` → RPC to server
5. **Server validates votes** → Via `ShouldAllowVote()` and `GetVoteWeight()`
6. **Timer expires** → `CompleteVoting()` called
7. **Winner determined** → Via `GetWinningScenario()`
8. **Transition happens** → `OnVotingComplete()` → Auto-transition if enabled

### Creating a Custom Voting Component

```cpp
// CustomVotingComponent.h
#pragma once

#include "Components/ScenarioTransitionComponent.h"
#include "CustomVotingComponent.generated.h"

UCLASS()
class MYGAME_API UCustomVotingComponent : public UScenarioTransitionComponent
{
    GENERATED_BODY()

public:
    // ========================================================================
    // CUSTOM PROPERTIES
    // ========================================================================

    /** Minimum player level to vote */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting")
    int32 MinimumLevelToVote = 5;

    /** Vote weight bonus for premium players */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting")
    float PremiumPlayerVoteBonus = 0.5f;

    /** Only show scenarios the player has unlocked */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Voting")
    bool bOnlyShowUnlockedScenarios = true;

    // ========================================================================
    // OVERRIDE: Custom Vote Weighting
    // ========================================================================

    virtual float GetVoteWeight_Implementation(APlayerState* Voter) const override
    {
        // Get custom player state
        AMyPlayerState* MyPS = Cast<AMyPlayerState>(Voter);
        if (!MyPS)
            return 1.0f;

        float Weight = 1.0f;

        // Bonus for premium players
        if (MyPS->bIsPremiumPlayer)
        {
            Weight += PremiumPlayerVoteBonus;
        }

        // Penalty for low-level players
        if (MyPS->PlayerLevel < MinimumLevelToVote)
        {
            Weight *= 0.5f;
        }

        // Bonus based on playtime
        float PlaytimeHours = MyPS->TotalPlaytimeSeconds / 3600.0f;
        Weight += FMath::Min(PlaytimeHours * 0.01f, 0.5f);

        return Weight;
    }

    // ========================================================================
    // OVERRIDE: Custom Scenario Selection
    // ========================================================================

    virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios) override
    {
        if (bOnlyShowUnlockedScenarios)
        {
            // Get scenarios unlocked by majority of players
            OutScenarios = GetMostCommonUnlockedScenarios();
        }
        else
        {
            // Use default random selection
            Super::SelectScenarioOptions_Implementation(OutScenarios);
        }
    }

    // ========================================================================
    // OVERRIDE: Custom Vote Validation
    // ========================================================================

    virtual bool ShouldAllowVote_Implementation(APlayerState* Voter) const override
    {
        AMyPlayerState* MyPS = Cast<AMyPlayerState>(Voter);
        if (!MyPS)
            return false;

        // Don't let spectators vote
        if (MyPS->IsOnlyASpectator())
            return false;

        // Don't let low-level players vote
        if (MyPS->PlayerLevel < MinimumLevelToVote)
        {
            // Could show UI message to player here
            return false;
        }

        // Don't let muted/banned players vote
        if (MyPS->bIsMuted || MyPS->bIsBanned)
            return false;

        return true;
    }

    // ========================================================================
    // OVERRIDE: Post-Vote Actions
    // ========================================================================

    virtual void OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId) override
    {
        Super::OnVotingComplete_Implementation(WinningScenarioId);

        // Track which scenarios won for analytics
        TrackScenarioVoteResult(WinningScenarioId);

        // Award XP to players who voted for the winner
        RewardPlayersWhoVotedForWinner(WinningScenarioId);

        // Send notification to all players
        BroadcastVoteResult(WinningScenarioId);
    }

private:
    TArray<FPrimaryAssetId> GetMostCommonUnlockedScenarios()
    {
        // Implementation: Check which scenarios most players have unlocked
        // Return those scenarios
        TArray<FPrimaryAssetId> Scenarios;
        // ... your unlock checking logic ...
        return Scenarios;
    }

    void TrackScenarioVoteResult(FPrimaryAssetId ScenarioId)
    {
        // Send to analytics, save to database, etc.
    }

    void RewardPlayersWhoVotedForWinner(FPrimaryAssetId WinningId)
    {
        // Give XP/currency to players who voted for winning scenario
        for (const auto& Vote : PlayerVotes)
        {
            if (Vote.Value.ScenarioId == WinningId)
            {
                AMyPlayerState* PS = Cast<AMyPlayerState>(Vote.Key);
                if (PS)
                {
                    PS->AddExperience(100); // Reward for voting with majority
                }
            }
        }
    }

    void BroadcastVoteResult(FPrimaryAssetId WinningId)
    {
        // Show UI notification to all players
        // "Players voted for: Conquest Mode!"
    }
};
```

### Voting UI Example (Blueprint/UMG)

**Widget: WBP_VotingPanel**

Blueprint Event Graph:
```
Event Construct
  └─ Bind Event to "On Voting State Changed"
      └─ If Voting Active
          ├─ Show Panel
          └─ Populate Scenario Options
      └─ Else
          └─ Hide Panel

Event Tick (only when voting active)
  └─ Update Timer Display
      └─ Get Voting Time Remaining
      └─ Update Text Block

On Scenario Button Clicked (for each option)
  └─ Get Scenario Transition Component
  └─ Cast Vote (Scenario ID)
  └─ Highlight Selected Button
```

**C++ Widget Implementation:**
```cpp
// VotingPanelWidget.h
UCLASS()
class UVotingPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    class UVerticalBox* ScenarioOptionsBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    class UTextBlock* TimerText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    class UProgressBar* TimerProgress;

    UFUNCTION()
    void OnVotingStateChanged(bool bIsVoting);

    UFUNCTION()
    void OnScenarioButtonClicked(FPrimaryAssetId ScenarioId);

    void PopulateScenarioOptions();
    void UpdateTimer();

private:
    UPROPERTY()
    UScenarioTransitionComponent* ScenarioTransition;
};

// VotingPanelWidget.cpp
void UVotingPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Get scenario transition component from GameState
    if (AGameStateBase* GS = GetWorld()->GetGameState())
    {
        ScenarioTransition = GS->FindComponentByClass<UScenarioTransitionComponent>();

        if (ScenarioTransition)
        {
            ScenarioTransition->OnVotingStateChanged.AddDynamic(this, &UVotingPanelWidget::OnVotingStateChanged);
        }
    }
}

void UVotingPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (ScenarioTransition && ScenarioTransition->bVotingActive)
    {
        UpdateTimer();
    }
}

void UVotingPanelWidget::OnVotingStateChanged(bool bIsVoting)
{
    if (bIsVoting)
    {
        SetVisibility(ESlateVisibility::Visible);
        PopulateScenarioOptions();
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UVotingPanelWidget::PopulateScenarioOptions()
{
    if (!ScenarioOptionsBox || !ScenarioTransition)
        return;

    ScenarioOptionsBox->ClearChildren();

    for (const FScenarioVoteOption& Option : ScenarioTransition->VoteOptions)
    {
        // Create button for each scenario
        UMyScenarioButton* Button = CreateWidget<UMyScenarioButton>(this, ScenarioButtonClass);
        Button->SetScenarioId(Option.ScenarioId);
        Button->OnClicked.AddDynamic(this, &UVotingPanelWidget::OnScenarioButtonClicked);

        ScenarioOptionsBox->AddChild(Button);
    }
}

void UVotingPanelWidget::OnScenarioButtonClicked(FPrimaryAssetId ScenarioId)
{
    if (ScenarioTransition)
    {
        ScenarioTransition->CastVote(ScenarioId);
    }
}

void UVotingPanelWidget::UpdateTimer()
{
    if (!ScenarioTransition)
        return;

    float TimeRemaining = ScenarioTransition->VotingTimeRemaining;
    float TotalDuration = ScenarioTransition->VotingDuration;

    // Update text
    int32 Seconds = FMath::CeilToInt(TimeRemaining);
    FText TimerDisplay = FText::Format(
        NSLOCTEXT("Voting", "TimeRemaining", "Time Remaining: {0}s"),
        FText::AsNumber(Seconds)
    );
    TimerText->SetText(TimerDisplay);

    // Update progress bar
    float Progress = TimeRemaining / TotalDuration;
    TimerProgress->SetPercent(Progress);
}
```

## Persistence & Rotation Advanced Usage

### Custom Persistence Manager

```cpp
// MyPersistenceManager.h
#pragma once

#include "ScenarioPersistenceManager.h"
#include "MyPersistenceManager.generated.h"

UCLASS()
class MYGAME_API UMyPersistenceManager : public UScenarioPersistenceManager
{
    GENERATED_BODY()

public:
    // ========================================================================
    // CUSTOM ROTATION LOGIC
    // ========================================================================

    /** Time-of-day based rotation */
    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    bool bUseTimeOfDayRotation = true;

    /** Player count based weighting */
    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    bool bScaleWeightByPlayerCount = true;

    virtual float CalculateRotationWeight_Implementation(FPrimaryAssetId ScenarioId) const override
    {
        float Weight = Super::CalculateRotationWeight_Implementation(ScenarioId);

        // Get scenario stats
        FScenarioStats Stats;
        if (!GetScenarioStats(ScenarioId, Stats))
        {
            return Weight;
        }

        // Apply time-of-day bonus
        if (bUseTimeOfDayRotation)
        {
            FString ScenarioName = ScenarioId.PrimaryAssetName.ToString();
            FDateTime Now = FDateTime::Now();
            int32 Hour = Now.GetHour();

            // Night scenarios (8pm - 6am)
            if ((Hour >= 20 || Hour < 6) && ScenarioName.Contains("Night"))
            {
                Weight *= 1.5f;
            }
            // Day scenarios (6am - 8pm)
            else if (Hour >= 6 && Hour < 20 && ScenarioName.Contains("Day"))
            {
                Weight *= 1.5f;
            }
        }

        // Scale weight based on average player count
        if (bScaleWeightByPlayerCount)
        {
            float CurrentPlayers = GetCurrentPlayerCount();
            float AveragePlayers = Stats.AveragePlayerCount;

            // Prefer scenarios that work well with current player count
            if (FMath::Abs(CurrentPlayers - AveragePlayers) < 5.0f)
            {
                Weight *= 1.3f; // Boost scenarios with similar player counts
            }
        }

        return Weight;
    }

    // ========================================================================
    // CUSTOM PERSISTENCE PATH
    // ========================================================================

    virtual FString GetPersistencePath_Implementation() const override
    {
        // Per-region saves for multi-region servers
        FString Region = GetServerRegion();
        return FPaths::ProjectSavedDir() / FString::Printf(TEXT("Stats_%s.json"), *Region);
    }

private:
    float GetCurrentPlayerCount() const
    {
        if (UWorld* World = GetWorld())
        {
            if (AGameStateBase* GS = World->GetGameState())
            {
                return static_cast<float>(GS->PlayerArray.Num());
            }
        }
        return 0.0f;
    }

    FString GetServerRegion() const
    {
        // Get from config, environment variable, or command line
        // Example: return "US-West", "EU-Central", "Asia-Pacific"
        return TEXT("Default");
    }
};
```

### Analytics Integration

Track scenario performance and player preferences:

```cpp
void UMyPersistenceManager::OnScenarioActivated(UGameplayScenario* Scenario)
{
    Super::OnScenarioActivated(Scenario);

    // Send analytics event
    TArray<FAnalyticsEventAttribute> Attributes;
    Attributes.Add(FAnalyticsEventAttribute(TEXT("ScenarioName"), GetNameSafe(Scenario)));
    Attributes.Add(FAnalyticsEventAttribute(TEXT("ScenarioId"), Scenario->GetPrimaryAssetId().ToString()));
    Attributes.Add(FAnalyticsEventAttribute(TEXT("PlayerCount"), GetCurrentPlayerCount()));
    Attributes.Add(FAnalyticsEventAttribute(TEXT("TimeOfDay"), FDateTime::Now().GetHour()));

    FAnalytics::Get().GetProvider()->RecordEvent(TEXT("Scenario.Activated"), Attributes);
}

// Track voting results
void UMyCustomVotingComponent::OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId)
{
    Super::OnVotingComplete_Implementation(WinningScenarioId);

    // Calculate vote distribution
    TMap<FPrimaryAssetId, int32> VoteDistribution;
    for (const FScenarioVoteOption& Option : VoteOptions)
    {
        VoteDistribution.Add(Option.ScenarioId, Option.VoteCount);
    }

    // Send to analytics
    for (const auto& Pair : VoteDistribution)
    {
        TArray<FAnalyticsEventAttribute> Attributes;
        Attributes.Add(FAnalyticsEventAttribute(TEXT("ScenarioId"), Pair.Key.ToString()));
        Attributes.Add(FAnalyticsEventAttribute(TEXT("Votes"), Pair.Value));
        Attributes.Add(FAnalyticsEventAttribute(TEXT("WonVote"), Pair.Key == WinningScenarioId));

        FAnalytics::Get().GetProvider()->RecordEvent(TEXT("Scenario.VoteResult"), Attributes);
    }
}
```

### Database Integration Example

Store stats in a database for cross-server persistence:

```cpp
void UMyPersistenceManager::SaveStatsToDatabase()
{
    // Pseudo-code for database integration

    for (const auto& Pair : ScenarioStatsMap)
    {
        const FScenarioStats& Stats = Pair.Value;

        FString Query = FString::Printf(TEXT(
            "INSERT INTO ScenarioStats (ScenarioId, TimesPlayed, TotalVotes, LastPlayed, AvgPlayers) "
            "VALUES ('%s', %d, %d, %lld, %.2f) "
            "ON DUPLICATE KEY UPDATE "
            "TimesPlayed = %d, TotalVotes = %d, LastPlayed = %lld, AvgPlayers = %.2f"
        ),
            *Stats.ScenarioId.ToString(),
            Stats.TimesPlayed,
            Stats.TotalVotes,
            Stats.LastPlayedTimestamp,
            Stats.AveragePlayerCount,
            Stats.TimesPlayed,
            Stats.TotalVotes,
            Stats.LastPlayedTimestamp,
            Stats.AveragePlayerCount
        );

        // Execute query via your database wrapper
        // DatabaseConnection->ExecuteQuery(Query);
    }
}

void UMyPersistenceManager::LoadStatsFromDatabase()
{
    // Pseudo-code for database integration

    // FString Query = "SELECT * FROM ScenarioStats";
    // TArray<FDatabaseRow> Results = DatabaseConnection->ExecuteQuery(Query);

    // for (const FDatabaseRow& Row : Results)
    // {
    //     FScenarioStats Stats;
    //     Stats.ScenarioId = FPrimaryAssetId::FromString(Row.GetString("ScenarioId"));
    //     Stats.TimesPlayed = Row.GetInt("TimesPlayed");
    //     Stats.TotalVotes = Row.GetInt("TotalVotes");
    //     Stats.LastPlayedTimestamp = Row.GetInt64("LastPlayed");
    //     Stats.AveragePlayerCount = Row.GetFloat("AvgPlayers");
    //
    //     ScenarioStatsMap.Add(Stats.ScenarioId, Stats);
    // }
}
```

---

# Extending the Plugin

## Creating Custom Scenario Actions

Scenario actions are the building blocks of scenarios. Here's how to create your own.

### Example: Spawn Actor Action

```cpp
// GameplaySA_SpawnActor.h
#pragma once

#include "GameplayScenarioAction.h"
#include "GameplaySA_SpawnActor.generated.h"

/**
 * Scenario Action: Spawn Actor
 * Spawns actors when the scenario activates
 */
UCLASS()
class MYGAME_API UGameplaySA_SpawnActor : public UGameplayScenarioAction
{
    GENERATED_BODY()

public:
    /** Actors to spawn */
    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TArray<TSubclassOf<AActor>> ActorsToSpawn;

    /** Spawn locations (optional, uses spawn points if empty) */
    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TArray<FTransform> SpawnTransforms;

    /** Whether to destroy spawned actors on deactivation */
    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    bool bDestroyOnDeactivate = true;

    virtual void OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem) override;
    virtual void OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown) override;

private:
    UPROPERTY(Transient)
    TArray<AActor*> SpawnedActors;
};

// GameplaySA_SpawnActor.cpp
void UGameplaySA_SpawnActor::OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem)
{
    UWorld* World = Subsystem->GetWorld();
    if (!World || !World->GetAuthGameMode())
        return; // Only spawn on server

    for (int32 i = 0; i < ActorsToSpawn.Num(); i++)
    {
        if (!ActorsToSpawn[i])
            continue;

        FTransform SpawnTransform = FTransform::Identity;

        // Use provided transform or find spawn point
        if (SpawnTransforms.IsValidIndex(i))
        {
            SpawnTransform = SpawnTransforms[i];
        }
        else
        {
            // Find a spawn point
            // Implementation depends on your game
            SpawnTransform = FindSpawnPoint();
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor* SpawnedActor = World->SpawnActor<AActor>(
            ActorsToSpawn[i],
            SpawnTransform,
            SpawnParams
        );

        if (SpawnedActor)
        {
            SpawnedActors.Add(SpawnedActor);
            UE_LOG(LogTemp, Log, TEXT("Spawned actor: %s"), *GetNameSafe(SpawnedActor));
        }
    }
}

void UGameplaySA_SpawnActor::OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown)
{
    if (bDestroyOnDeactivate)
    {
        for (AActor* Actor : SpawnedActors)
        {
            if (IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
    }

    SpawnedActors.Empty();
}
```

### Example: Apply Game Rules Action

```cpp
// GameplaySA_ApplyGameRules.h
#pragma once

#include "GameplayScenarioAction.h"
#include "GameplaySA_ApplyGameRules.generated.h"

/**
 * Scenario Action: Apply Game Rules
 * Modifies game mode settings when scenario activates
 */
UCLASS()
class MYGAME_API UGameplaySA_ApplyGameRules : public UGameplayScenarioAction
{
    GENERATED_BODY()

public:
    /** Time limit in seconds (0 = no limit) */
    UPROPERTY(EditDefaultsOnly, Category = "Rules")
    float TimeLimit = 600.0f;

    /** Score limit (0 = no limit) */
    UPROPERTY(EditDefaultsOnly, Category = "Rules")
    int32 ScoreLimit = 100;

    /** Enable friendly fire */
    UPROPERTY(EditDefaultsOnly, Category = "Rules")
    bool bFriendlyFire = false;

    /** Respawn delay in seconds */
    UPROPERTY(EditDefaultsOnly, Category = "Rules")
    float RespawnDelay = 5.0f;

    virtual void OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem) override;
    virtual void OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown) override;

private:
    // Store original values to restore
    float OriginalTimeLimit;
    int32 OriginalScoreLimit;
    bool bOriginalFriendlyFire;
    float OriginalRespawnDelay;
};

// GameplaySA_ApplyGameRules.cpp
void UGameplaySA_ApplyGameRules::OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem)
{
    UWorld* World = Subsystem->GetWorld();
    AMyGameMode* GameMode = World ? World->GetAuthGameMode<AMyGameMode>() : nullptr;

    if (!GameMode)
        return;

    // Store original values
    OriginalTimeLimit = GameMode->TimeLimit;
    OriginalScoreLimit = GameMode->ScoreLimit;
    bOriginalFriendlyFire = GameMode->bFriendlyFireEnabled;
    OriginalRespawnDelay = GameMode->RespawnDelay;

    // Apply new rules
    GameMode->TimeLimit = TimeLimit;
    GameMode->ScoreLimit = ScoreLimit;
    GameMode->bFriendlyFireEnabled = bFriendlyFire;
    GameMode->RespawnDelay = RespawnDelay;

    // Notify game mode that rules changed
    GameMode->OnGameRulesChanged();

    UE_LOG(LogTemp, Log, TEXT("Applied game rules: TimeLimit=%.0f, ScoreLimit=%d, FriendlyFire=%s"),
        TimeLimit, ScoreLimit, bFriendlyFire ? TEXT("Yes") : TEXT("No"));
}

void UGameplaySA_ApplyGameRules::OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown)
{
    UWorld* World = Subsystem->GetWorld();
    AMyGameMode* GameMode = World ? World->GetAuthGameMode<AMyGameMode>() : nullptr;

    if (!GameMode)
        return;

    // Restore original values
    GameMode->TimeLimit = OriginalTimeLimit;
    GameMode->ScoreLimit = OriginalScoreLimit;
    GameMode->bFriendlyFireEnabled = bOriginalFriendlyFire;
    GameMode->RespawnDelay = OriginalRespawnDelay;

    GameMode->OnGameRulesChanged();
}
```

### Example: Blueprint Callable Action

```cpp
// GameplaySA_BlueprintEvent.h
#pragma once

#include "GameplayScenarioAction.h"
#include "GameplaySA_BlueprintEvent.generated.h"

/**
 * Scenario Action: Blueprint Event
 * Triggers Blueprint events when scenario activates/deactivates
 * Allows designers to add custom logic without C++
 */
UCLASS(Blueprintable, BlueprintType)
class MYGAME_API UGameplaySA_BlueprintEvent : public UGameplayScenarioAction
{
    GENERATED_BODY()

public:
    /** Event called when scenario activates */
    UFUNCTION(BlueprintImplementableEvent, Category = "Scenario", meta = (DisplayName = "On Activated"))
    void ReceiveOnActivated();

    /** Event called before scenario deactivates */
    UFUNCTION(BlueprintImplementableEvent, Category = "Scenario", meta = (DisplayName = "On Deactivated"))
    void ReceiveOnDeactivated(bool bTearDown);

    virtual void OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem) override
    {
        ReceiveOnActivated();
    }

    virtual void OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown) override
    {
        ReceiveOnDeactivated(bTearDown);
    }
};
```

Now designers can create Blueprint child classes and implement custom logic!

---

# API Reference

## Core Classes

### UScenarioInstanceSubsystem

Main subsystem for scenario management.

**Key Methods:**
```cpp
// Activate a scenario
void ActivateScenario(UGameplayScenario* Scenario, bool bForce = false);
void ActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce = false);

// Deactivate a scenario
void DeactivateScenario(UGameplayScenario* Scenario);
void DeactivateScenario(FPrimaryAssetId ScenarioAsset);

// Pre-activation (loads assets)
void PreActivateScenario(UGameplayScenario* Scenario, bool bForce = false);
void PreActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce = false);

// Transition workflow
void SetPendingScenario(UGameplayScenario* Scenario);
void TransitionToPendingScenario(bool bForce = false);

// Query active scenarios
bool IsScenarioActive(UGameplayScenario* Scenario) const;
TArray<UGameplayScenario*> GetActiveScenarios() const { return ActiveScenarios; }

// Cleanup
void TearDownActiveScenarios();
```

**Delegates:**
```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FScenarioDelegate, UGameplayScenario*);

FScenarioDelegate OnScenarioActivated;    // Broadcast when scenario activates
FScenarioDelegate OnScenarioDeactivated;  // Broadcast when scenario deactivates
```

### UGameplayScenario

Data asset defining a scenario.

**Properties:**
```cpp
// Optional map to load
UPROPERTY(EditAnywhere, AssetRegistrySearchable)
FPrimaryAssetId Map;

// Actions to execute
UPROPERTY(EditDefaultsOnly, Instanced)
TArray<UGameplayScenarioAction*> ScenarioActions;

// UI metadata
UPROPERTY(EditAnywhere, AssetRegistrySearchable)
FText Name;

UPROPERTY(EditAnywhere, AssetRegistrySearchable)
FText Description;

// Gameplay tags for filtering
UPROPERTY(EditAnywhere)
FGameplayTagContainer ScenarioTags;
```

**Methods:**
```cpp
void PreActivateScenario(UScenarioInstanceSubsystem* Subsystem);
void ActivateScenario(UScenarioInstanceSubsystem* Subsystem);
void DeactivateScenario(UScenarioInstanceSubsystem* Subsystem, bool bTearDown = false);
```

### UScenarioTransitionComponent

Base voting component.

**Key Methods:**
```cpp
// Start/cancel voting
bool StartVoting();
void CancelVoting();

// Client RPC to cast vote
void CastVote(FPrimaryAssetId ScenarioId);

// Query vote state
bool GetWinningScenario(FPrimaryAssetId& OutScenarioId) const;
bool GetScenarioVotes(FPrimaryAssetId ScenarioId, float& OutVoteTally, int32& OutVoteCount) const;
```

**Extensibility Points:**
```cpp
// Override these in your subclass
virtual float GetVoteWeight_Implementation(APlayerState* Voter) const;
virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios);
virtual bool ShouldAllowVote_Implementation(APlayerState* Voter) const;
virtual void OnVotingComplete_Implementation(FPrimaryAssetId WinningScenarioId);
```

### UScenarioPersistenceManager

Statistics and rotation subsystem.

**Key Methods:**
```cpp
// Record statistics
void RecordScenarioPlayed(FPrimaryAssetId ScenarioId, int32 NumPlayers);
void RecordScenarioVotes(FPrimaryAssetId ScenarioId, int32 NumVotes);

// Query statistics
bool GetScenarioStats(FPrimaryAssetId ScenarioId, FScenarioStats& OutStats) const;
TArray<FScenarioStats> GetAllStats() const;

// Persistence
bool SaveStats();
bool LoadStats();
void ResetStats();

// Rotation
TArray<FPrimaryAssetId> SelectScenariosWeighted(int32 NumScenarios, const FGameplayTagQuery& FilterTags);
TArray<FPrimaryAssetId> GetLeastRecentlyPlayed(int32 NumScenarios, const FGameplayTagQuery& FilterTags);
TArray<FPrimaryAssetId> GetMostPopular(int32 NumScenarios, const FGameplayTagQuery& FilterTags);
```

**Extensibility Points:**
```cpp
virtual float CalculateRotationWeight_Implementation(FPrimaryAssetId ScenarioId) const;
virtual FString GetPersistencePath_Implementation() const;
```

### USharedGamemodeSettings

Project settings (UDeveloperSettings).

**Access Settings:**
```cpp
const USharedGamemodeSettings* Settings = USharedGamemodeSettings::Get();

if (Settings->bEnableScenarioVoting)
{
    // Voting is enabled
}

// Check if scenario is allowed
if (Settings->IsScenarioAllowed(ScenarioId))
{
    // Scenario passes whitelist/blacklist/tag filters
}
```

---

# Examples & Tutorials

## Tutorial 1: Creating a Multi-Map Rotation System

**Goal:** Create a server that rotates through 3 maps every 30 minutes.

### Step 1: Create Map Scenarios

Create three scenario assets:
- `SC_Map_Desert`
- `SC_Map_Forest`
- `SC_Map_Urban`

Each with their respective map and streaming levels.

### Step 2: Add Rotation to GameMode

```cpp
// MyGameMode.h
UCLASS()
class AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    TArray<FPrimaryAssetId> MapRotation = {
        FPrimaryAssetId::FromString(TEXT("GameplayScenario:/Game/Scenarios/SC_Map_Desert.SC_Map_Desert")),
        FPrimaryAssetId::FromString(TEXT("GameplayScenario:/Game/Scenarios/SC_Map_Forest.SC_Map_Forest")),
        FPrimaryAssetId::FromString(TEXT("GameplayScenario:/Game/Scenarios/SC_Map_Urban.SC_Map_Urban"))
    };

    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    float RotationInterval = 1800.0f; // 30 minutes

protected:
    virtual void BeginPlay() override;
    void RotateMap();

private:
    FTimerHandle RotationTimer;
    int32 CurrentMapIndex = 0;
};

// MyGameMode.cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        // Start first map
        RotateMap();

        // Set up timer for rotation
        GetWorldTimerManager().SetTimer(
            RotationTimer,
            this,
            &AMyGameMode::RotateMap,
            RotationInterval,
            true
        );
    }
}

void AMyGameMode::RotateMap()
{
    if (MapRotation.Num() == 0)
        return;

    FPrimaryAssetId NextMap = MapRotation[CurrentMapIndex];
    CurrentMapIndex = (CurrentMapIndex + 1) % MapRotation.Num();

    UScenarioInstanceSubsystem* ScenarioSystem =
        GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

    ScenarioSystem->PreActivateScenario(NextMap, true);
    ScenarioSystem->ActivateScenario(NextMap, true);
}
```

## Tutorial 2: Player-Unlockable Scenarios

**Goal:** Only show scenarios in voting that players have unlocked.

### Step 1: Track Player Unlocks

```cpp
// MyPlayerState.h
UCLASS()
class AMyPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, SaveGame)
    TArray<FPrimaryAssetId> UnlockedScenarios;

    UFUNCTION(BlueprintCallable)
    void UnlockScenario(FPrimaryAssetId ScenarioId);

    UFUNCTION(BlueprintPure)
    bool HasUnlockedScenario(FPrimaryAssetId ScenarioId) const;
};
```

### Step 2: Custom Voting Component

```cpp
// UnlockableVotingComponent.h
UCLASS()
class UUnlockableVotingComponent : public UScenarioTransitionComponent
{
    GENERATED_BODY()

public:
    virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios) override
    {
        // Get all scenarios
        UAssetManager& Manager = UAssetManager::Get();
        TArray<FPrimaryAssetId> AllScenarios;
        Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

        // Find scenarios unlocked by at least 50% of players
        TMap<FPrimaryAssetId, int32> UnlockCounts;
        int32 TotalPlayers = 0;

        if (UWorld* World = GetWorld())
        {
            if (AGameStateBase* GS = World->GetGameState())
            {
                TotalPlayers = GS->PlayerArray.Num();

                for (APlayerState* PS : GS->PlayerArray)
                {
                    AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
                    if (MyPS)
                    {
                        for (const FPrimaryAssetId& UnlockedId : MyPS->UnlockedScenarios)
                        {
                            UnlockCounts.FindOrAdd(UnlockedId, 0)++;
                        }
                    }
                }
            }
        }

        // Filter scenarios
        TArray<FPrimaryAssetId> AvailableScenarios;
        int32 RequiredUnlocks = FMath::Max(1, TotalPlayers / 2);

        for (const FPrimaryAssetId& ScenarioId : AllScenarios)
        {
            int32 Unlocks = UnlockCounts.FindRef(ScenarioId);
            if (Unlocks >= RequiredUnlocks)
            {
                AvailableScenarios.Add(ScenarioId);
            }
        }

        // Randomly select from available
        int32 NumToSelect = FMath::Min(NumScenarioOptions, AvailableScenarios.Num());
        for (int32 i = 0; i < NumToSelect; i++)
        {
            int32 RandomIndex = FMath::RandRange(i, AvailableScenarios.Num() - 1);
            AvailableScenarios.Swap(i, RandomIndex);
        }

        OutScenarios.Append(AvailableScenarios.GetData(), NumToSelect);
    }
};
```

## Tutorial 3: Time-Limited Event Scenarios

**Goal:** Special scenarios that only appear during specific dates/times.

### Step 1: Add Metadata to Scenarios

```cpp
// EventScenario.h
UCLASS()
class UEventScenario : public UGameplayScenario
{
    GENERATED_BODY()

public:
    /** Start date for event (UTC) */
    UPROPERTY(EditAnywhere, Category = "Event")
    FDateTime EventStartDate;

    /** End date for event (UTC) */
    UPROPERTY(EditAnywhere, Category = "Event")
    FDateTime EventEndDate;

    /** Is this scenario currently available? */
    UFUNCTION(BlueprintPure, Category = "Event")
    bool IsEventActive() const
    {
        FDateTime Now = FDateTime::UtcNow();
        return Now >= EventStartDate && Now <= EventEndDate;
    }
};
```

### Step 2: Filter in Voting

```cpp
virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios) override
{
    UAssetManager& Manager = UAssetManager::Get();
    TArray<FPrimaryAssetId> AllScenarios;
    Manager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

    // Filter for active events
    TArray<FPrimaryAssetId> ActiveScenarios;
    for (const FPrimaryAssetId& ScenarioId : AllScenarios)
    {
        UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);

        // Check if it's an event scenario
        if (UEventScenario* EventScenario = Cast<UEventScenario>(Scenario))
        {
            if (!EventScenario->IsEventActive())
                continue; // Skip inactive events
        }

        ActiveScenarios.Add(ScenarioId);
    }

    // Select from active scenarios
    // ... random selection logic ...
}
```

---

# Performance Optimization

## Async Asset Loading

The plugin uses synchronous loading in some places (`WaitUntilComplete()`). For production, consider async loading:

```cpp
void UMyScenarioSubsystem::ActivateScenarioAsync(FPrimaryAssetId ScenarioId)
{
    UAssetManager& Manager = UAssetManager::Get();

    // Start async load
    TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAsset(
        ScenarioId,
        TArray<FName>(),
        FStreamableDelegate::CreateUObject(this, &UMyScenarioSubsystem::OnScenarioLoaded, ScenarioId)
    );
}

void UMyScenarioSubsystem::OnScenarioLoaded(FPrimaryAssetId ScenarioId)
{
    UAssetManager& Manager = UAssetManager::Get();
    UGameplayScenario* Scenario = Manager.GetPrimaryAssetObject<UGameplayScenario>(ScenarioId);

    if (Scenario)
    {
        // Activate now that it's loaded
        ActivateScenario(Scenario, false);
    }
}
```

## Persistence Optimization

### Batch Saves

Instead of saving after every change:

```cpp
void UMyPersistenceManager::QueueSave()
{
    bPendingSave = true;
}

void UMyPersistenceManager::Tick(float DeltaTime)
{
    SaveTimer += DeltaTime;

    if (bPendingSave && SaveTimer >= AutoSaveInterval)
    {
        SaveStats();
        bPendingSave = false;
        SaveTimer = 0.0f;
    }
}
```

### Compress JSON

For large stat files, compress before saving:

```cpp
bool UMyPersistenceManager::SaveStats()
{
    FString JsonString = SerializeStatsToJson();

    // Compress
    TArray<uint8> CompressedData;
    FArchiveSaveCompressedProxy Compressor(CompressedData, NAME_Zlib);
    Compressor << JsonString;
    Compressor.Close();

    // Save compressed
    FString FilePath = GetPersistencePath() + TEXT(".gz");
    return FFileHelper::SaveArrayToFile(CompressedData, *FilePath);
}
```

## Network Optimization

### Throttle Vote Updates

Don't replicate every single vote immediately:

```cpp
void UMyVotingComponent::ServerCastVote_Implementation(APlayerState* Voter, FPrimaryAssetId ScenarioId)
{
    // ... validation ...

    // Update vote locally
    UpdateVoteLocally(Voter, ScenarioId);

    // Queue replication update (throttled)
    QueueVoteReplicationUpdate();
}

void UMyVotingComponent::QueueVoteReplicationUpdate()
{
    if (!GetWorld()->GetTimerManager().IsTimerActive(ReplicationThrottleTimer))
    {
        GetWorld()->GetTimerManager().SetTimer(
            ReplicationThrottleTimer,
            this,
            &UMyVotingComponent::ReplicateVoteOptions,
            0.5f, // Update clients every 0.5 seconds max
            false
        );
    }
}
```

---

# Troubleshooting

## Common Issues

### Issue: "Scenario not found in asset registry"

**Cause:** Asset manager hasn't indexed the scenario.

**Solutions:**
1. Ensure scenario is in a Game Feature plugin that's active
2. Check Project Settings → Asset Manager → Primary Asset Types
3. Add GameplayScenario as a primary asset type if missing:
   ```
   Primary Asset Type: GameplayScenario
   Asset Base Class: GameplayScenario
   Directories: /Game/
   Cook Rule: Always Cook
   ```
4. Force rescan: Content Browser → Right-click → "Rescan Asset Registry"

### Issue: Voting not starting

**Checks:**
1. Is component on GameState? `GetGameState()->FindComponentByClass<UScenarioTransitionComponent>()`
2. Is component replicated? Check `SetIsReplicated(true)` in constructor
3. Is `bEnableScenarioVoting` true in settings?
4. Are there enough players? Check `MinimumPlayersForVoting`
5. Server authority check: Only server can call `StartVoting()`

**Debug:**
```cpp
void AMyGameMode::StartVoting()
{
    AGameStateBase* GS = GetGameState<AGameStateBase>();
    check(GS);

    UScenarioTransitionComponent* Voting = GS->FindComponentByClass<UScenarioTransitionComponent>();

    if (!Voting)
    {
        UE_LOG(LogTemp, Error, TEXT("No voting component on GameState!"));
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start voting from client!"));
        return;
    }

    bool bSuccess = Voting->StartVoting();
    UE_LOG(LogTemp, Log, TEXT("Start voting result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
}
```

### Issue: Scenarios not replicating to clients

**Checks:**
1. Is `UGamestateScenarioComponent` added to GameState?
2. Is `SetIsReplicatedByDefault(true)` called in component constructor?
3. Are scenario assets cooked for clients?
4. Check network role: `GetOwnerRole()` should be `ROLE_Authority` on server

**Debug:**
```cpp
// Server
void AMyGameState::LogScenarioReplication()
{
    UGamestateScenarioComponent* Comp = FindComponentByClass<UGamestateScenarioComponent>();
    if (Comp)
    {
        UE_LOG(LogTemp, Log, TEXT("[SERVER] Active scenarios: %d"), Comp->Scenarios.Items.Num());
        for (const auto& Item : Comp->Scenarios.Items)
        {
            UE_LOG(LogTemp, Log, TEXT("  - %s"), *GetNameSafe(Item.Scenario));
        }
    }
}

// Client
void AMyPlayerController::LogScenarioReplication()
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    UGamestateScenarioComponent* Comp = GS->FindComponentByClass<UGamestateScenarioComponent>();
    if (Comp)
    {
        UE_LOG(LogTemp, Log, TEXT("[CLIENT] Replicated scenarios: %d"), Comp->Scenarios.Items.Num());
        for (const auto& Item : Comp->Scenarios.Items)
        {
            UE_LOG(LogTemp, Log, TEXT("  - %s"), *GetNameSafe(Item.Scenario));
        }
    }
}
```

### Issue: Persistence file not saving

**Checks:**
1. Is directory writable? Check permissions on `ProjectSaved/`
2. Is `bEnableScenarioPersistence` true?
3. Is `bAutoSaveStats` enabled if relying on auto-save?
4. Check disk space

**Manual save test:**
```cpp
UScenarioPersistenceManager* PM = GetGameInstance()->GetSubsystem<UScenarioPersistenceManager>();
if (PM)
{
    bool bSuccess = PM->SaveStats();
    UE_LOG(LogTemp, Log, TEXT("Manual save result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));

    FString Path = PM->GetPersistencePath();
    UE_LOG(LogTemp, Log, TEXT("Save path: %s"), *Path);
    UE_LOG(LogTemp, Log, TEXT("File exists: %s"), FPaths::FileExists(Path) ? TEXT("Yes") : TEXT("No"));
}
```

### Issue: Custom voting component not being used

**Check:**
1. Did you add YOUR component to GameState (not the base one)?
2. Is component class set correctly in Blueprint?

**Correct setup:**
```cpp
// MyGameState.cpp
AMyGameState::AMyGameState()
{
    // Use YOUR custom component class
    ScenarioTransition = CreateDefaultSubobject<UMyCustomVotingComponent>(TEXT("ScenarioTransition"));
}
```

---

## Debug Commands

Add these to your game for easier debugging:

```cpp
// In your cheat manager or console commands
void UMyCheatManager::DebugScenarios()
{
    UScenarioInstanceSubsystem* SS = GetWorld()->GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();

    UE_LOG(LogTemp, Log, TEXT("=== SCENARIO DEBUG ==="));
    UE_LOG(LogTemp, Log, TEXT("Active Scenarios: %d"), SS->ActiveScenarios.Num());

    for (UGameplayScenario* Scenario : SS->ActiveScenarios)
    {
        UE_LOG(LogTemp, Log, TEXT("  - %s (%d actions)"),
            *GetNameSafe(Scenario),
            Scenario->ScenarioActions.Num());
    }

    UE_LOG(LogTemp, Log, TEXT("Pending Scenario: %s"), *GetNameSafe(SS->PendingScenario));
    UE_LOG(LogTemp, Log, TEXT("Map Transition Scenario: %s"), *GetNameSafe(SS->MapTransitionScenario));
}

void UMyCheatManager::DebugVoting()
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    UScenarioTransitionComponent* Voting = GS->FindComponentByClass<UScenarioTransitionComponent>();

    if (!Voting)
    {
        UE_LOG(LogTemp, Error, TEXT("No voting component!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("=== VOTING DEBUG ==="));
    UE_LOG(LogTemp, Log, TEXT("Voting Active: %s"), Voting->bVotingActive ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Time Remaining: %.1f"), Voting->VotingTimeRemaining);
    UE_LOG(LogTemp, Log, TEXT("Vote Options: %d"), Voting->VoteOptions.Num());

    for (const FScenarioVoteOption& Option : Voting->VoteOptions)
    {
        UE_LOG(LogTemp, Log, TEXT("  - %s: %.1f votes (%d players)"),
            *Option.ScenarioId.ToString(),
            Option.VoteTally,
            Option.VoteCount);
    }
}

void UMyCheatManager::DebugPersistence()
{
    UScenarioPersistenceManager* PM = GetWorld()->GetGameInstance()->GetSubsystem<UScenarioPersistenceManager>();

    UE_LOG(LogTemp, Log, TEXT("=== PERSISTENCE DEBUG ==="));
    UE_LOG(LogTemp, Log, TEXT("Save Path: %s"), *PM->GetPersistencePath());
    UE_LOG(LogTemp, Log, TEXT("Auto Save: %s"), PM->bAutoSave ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Tracked Scenarios: %d"), PM->GetAllStats().Num());

    for (const FScenarioStats& Stats : PM->GetAllStats())
    {
        UE_LOG(LogTemp, Log, TEXT("  - %s: Played %d times, %d votes"),
            *Stats.ScenarioId.ToString(),
            Stats.TimesPlayed,
            Stats.TotalVotes);
    }
}
```

---

## Support

For additional help:

1. **Check the logs** - Look for LogScenarioTransition, LogScenarioPersistence, LogGameplayScenario
2. **Use debug commands** - Add the debug commands above to diagnose issues
3. **Review examples** - Check the examples in this documentation
4. **Community** - Ask questions on the GitHub issues page
5. **Documentation** - Re-read relevant sections of this doc

---

**End of Documentation**

Remember: **Subclass, Don't Modify!** The plugin is designed to be extended, not edited directly. Create your own classes that inherit from the base classes provided.

Happy developing! 🎮
