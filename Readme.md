# Shared Gamemode Plugin

An Unreal Engine Game Feature Plugin for modular gameplay scenarios with **dedicated server support**, designed for multiplayer games across many genres.

## Overview

Shared Gamemode provides a robust, **dedicated server-ready** system for creating and managing gameplay scenarios in Unreal Engine. Built with performance and modularity in mind, it empowers developers to create amazing multiplayer games without rewriting boilerplate for common systems.

### Key Features

- **Modular Scenario System** - Compose complex gameplay experiences from simple, reusable components
- **Dedicated Server Support** - Console commands, RPC communication, and server configuration
- **Voting System** - Extensible player voting with customizable weighting and validation
- **Scenario Rotation** - Intelligent map rotation with persistence and popularity tracking
- **Multiplayer Focused** - Server-authoritative design with efficient network replication
- **Highly Extensible** - Base classes designed for subclassing, not modification

### Design Philosophy

**Don't Assume Game Design - Let Developers Subclass**

This plugin provides core infrastructure without imposing game-specific logic. All systems are designed as base classes that you can extend to match your game's unique requirements.

## Requirements

- Unreal Engine 4.27+ or UE5
- Place in `Plugins/GameFeatures/` folder
- Required Plugins:
  - GameplayAbilities
  - ModularGameplay
  - GameFeatures

**Note:** This plugin no longer requires GameplayEventRouter (dependency removed).

## Quick Start

1. Drop this plugin into your `Plugins/GameFeatures/` folder
2. Enable it in your project settings (auto-activates as a Game Feature)
3. Create a `Gameplay Scenario` data asset in any Game Feature plugin
4. Configure your server settings in **Project Settings → Plugins → Shared Gamemode**
5. Start your dedicated server and use console commands to manage scenarios



## Using Scenarios

Gameplay Scenarios are exposed to the Asset Registry.  If you already have your own mechanism to query asset tags, the three tags exposed to the asset manager are `Name`, `Description`, and `bTopLevel`.  A "Top Level" Scenario should be how you display scenarios to users.  

If you don't have a mechanism to query asset tags, you can use this:

```cpp
//In some BlueprintFunctionLibrary header:
	UFUNCTION(BlueprintPure, Category = "Assets")
	static bool GetAssetTagAsString(FPrimaryAssetId Asset, FName Tag, FString& OutString);

	UFUNCTION(BlueprintPure, Category = "Assets")
	static bool GetAssetTagAsText(FPrimaryAssetId Asset, FName Tag, FText& OutText);

	UFUNCTION(BlueprintPure, Category = "Assets")
	static bool GetAssetTagAsBool(FPrimaryAssetId Asset, FName Tag, bool& OutBool);

//In that library's cpp:

    bool UYourBPFunctionLibrary::GetAssetTagAsString(FPrimaryAssetId Asset, FName Tag, FString& OutString)
    {
        UAssetManager& LocalManager = UAssetManager::Get();

        FAssetData AssetData;

        if (!LocalManager.GetPrimaryAssetData(Asset, AssetData))
        {
            return false;
        }

        bool bFound = AssetData.GetTagValue<FString>(Tag, OutString);

        return bFound;
    }

    bool UYourBPFunctionLibrary::GetAssetTagAsText(FPrimaryAssetId Asset, FName Tag, FText& OutText)
    {
        UAssetManager& LocalManager = UAssetManager::Get();

        FAssetData AssetData;

        if (!LocalManager.GetPrimaryAssetData(Asset, AssetData))
        {
            return false;
        }

        bool bFound = AssetData.GetTagValue<FText>(Tag, OutText);

        return bFound;
    }

    bool UYourBPFunctionLibrary::GetAssetTagAsBool(FPrimaryAssetId Asset, FName Tag, bool& OutBool)
    {
        UAssetManager& LocalManager = UAssetManager::Get();

        FAssetData AssetData;

        if (!LocalManager.GetPrimaryAssetData(Asset, AssetData))
        {
            return false;
        }

        bool bFound = AssetData.GetTagValue<bool>(Tag, OutBool);

        return bFound;
    }
```

Just use the asset manager to get all `GameplayScenario` assets, and query for their tags in your UI to display them.

To activate a scenario, call the `StartScenario` console command (you can do this from blueprint).  Pass in the FPrimaryAssetId for the scenario as the first parameter and the system will take over from there.  If you are trying to activate a scenario from C++, you need to set the PendingScenario on `UScenarioInstanceSubsystem`, then call `TransitionToPendingScenario()`.  This will wipe all active scenarios.  


## What is a Scenario anyway?

Scenarios are an asset that can be activated to provide gameplay functionality to a level.  In Empires, we have multiple gamemodes (Commander, Conquest, etc) with multiple maps (Canyon, BaenIsland, etc).  We also have a bunch of lighting types (Day, Night, Late Afternoon, Dynamic, etc).  Combining these options to present to users was a challenge, and we added to that challenge by allowing moddable Gamemodes, Maps, and Atmospheres.  

So, that's where Scenarios come in.  Scenarios allow us to select a Terrain, an Atmosphere Layer, and any number of Gamemode elements to combine into one "Map" experience that a player will experience.  

Scenarios have a list of Actions that are taken whe the scenario is made active.  Actions include:

* `ActivateScenario` - Activate another scenario, allowing for scenario composition
* `DeactivateScenario` - Deactivate another scenario by gameplay tag query
* `ChangeMap` - Transition the game/server to another map.  This clears all other scenarios when activated
* `StreamLevelInstance` - Streams in a LevelInstanceDynamic for a given list of levels. 
* `AddComponents` - Adds components to the Modular Gameplay Framework subsystem, so actors registered and listening for components will receive them
* (In Progress) `AddGASPrimitives` - Adds GAS Primitives (Abilities, AttributeSets, and GameplayEffects) to Actors that are listening for them

In Empires, we use Scenario Composition to create our map experiences.  For example, we have the following layout split into 3 different game feature plugins:

```
SC_BaenIslandCQDay (in GameFeatures/Scenarios/BaenIslandConquest)
  Actions: [
    ActivateScenario: SC_BaenIslandDay
    ActivateScenario: SC_Conquest
    StreamLevelInstance: [ L_BaenIslandCQ ]
  ]

SC_BaenIslandDay (in GameFeatures/Terrain/BaenIslandConquest)
  Actions: [
    ChangeMap: L_BaenIsland_Terrain
    StreamLevelInstance: [ L_BaenIsland_Atmo_Day ]
  ]

SC_Conquest (in GameFeatures/Gamemode)
  Actions: [
      ApplyGASPrimitives: {To: TeamState, AddAttributeSets: [Tickets], AddAbilities: [BP_SubtractTicketOnDeath_GA, BP_ConquestVictory_GA] }
  ]
```

---

## Dedicated Server Features

### Console Commands

The plugin provides comprehensive console commands for dedicated server management:

#### Scenario Management

```
StartScenario <AssetId>
```
Begin a scenario, changing maps if needed.
```
Example: StartScenario GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'
```

```
ListScenarios [Filter]
```
List all available scenarios. Optional filter parameter to search by name.
```
Example: ListScenarios Conquest
```

```
GetActiveScenarios
```
Show all currently active scenarios with their asset IDs and action counts.

```
DeactivateScenario <AssetId>
```
Deactivate a specific scenario by asset ID.
```
Example: DeactivateScenario GameplayScenario'/Game/Scenarios/SC_Conquest.SC_Conquest'
```

```
TearDownScenarios
```
Deactivate all currently active scenarios (useful for server cleanup).

### Server Configuration

Configure your server via **Project Settings → Plugins → Shared Gamemode** or override in `DefaultGame.ini`:

```ini
[/Script/SharedGamemode.SharedGamemodeSettings]
; Default scenario to load on server start
DefaultScenarioAsset="GameplayScenario'/Game/Scenarios/SC_Default.SC_Default'"
bAutoLoadDefaultScenario=true

; Voting configuration
bEnableScenarioVoting=true
DefaultVotingDuration=30.0
DefaultNumScenarioOptions=3
MinimumPlayersForVoting=2

; Scenario filtering
+ScenarioBlacklist=(ScenarioId="GameplayScenario'/Game/Scenarios/SC_Test.SC_Test'",Reason="Not ready for production")

; Persistence
bEnableScenarioPersistence=true
PersistenceFileName="ScenarioStats.json"
bAutoSaveStats=true
AutoSaveInterval=60.0
```

Command line overrides:
```bash
MyGameServer.exe -ScenarioVotingEnabled=true -DefaultScenario="GameplayScenario'/Game/...'"
```

---

## Voting System

### Basic Usage

Add a `ScenarioTransitionComponent` to your GameState:

```cpp
// In your GameState class
UPROPERTY(Replicated)
UScenarioTransitionComponent* ScenarioTransition;

// In constructor
ScenarioTransition = CreateDefaultSubobject<UScenarioTransitionComponent>("ScenarioTransition");

// Start voting
ScenarioTransition->StartVoting();
```

From Blueprint:
1. Add `ScenarioTransitionComponent` to your GameState Blueprint
2. Call `Start Voting` when appropriate (e.g., end of round)
3. Players can vote using `Cast Vote` with a scenario asset ID

### Extending the Voting System

The voting system is designed to be subclassed. Create your own component:

```cpp
UCLASS()
class UMyGameVotingComponent : public UScenarioTransitionComponent
{
    GENERATED_BODY()

public:
    // Override vote weight for performance-based voting
    virtual float GetVoteWeight_Implementation(APlayerState* Voter) const override
    {
        // Your custom logic here
        // Example: Weight based on player level
        if (UMyPlayerState* MyPS = Cast<UMyPlayerState>(Voter))
        {
            return 1.0f + (MyPS->PlayerLevel * 0.05f);
        }
        return 1.0f;
    }

    // Override scenario selection for custom rotation logic
    virtual void SelectScenarioOptions_Implementation(TArray<FPrimaryAssetId>& OutScenarios) override
    {
        // Your custom selection logic
        // Example: Only show scenarios the player has unlocked
    }

    // Override vote validation
    virtual bool ShouldAllowVote_Implementation(APlayerState* Voter) const override
    {
        // Your custom validation
        // Example: Don't let spectators vote
        if (APlayerController* PC = Voter->GetOwner<APlayerController>())
        {
            return !PC->IsInState(NAME_Spectating);
        }
        return true;
    }
};
```

### Enhanced Voting Component

The plugin includes `UEnhancedScenarioTransitionComponent` as a reference implementation:

**Features:**
- Performance-based vote weighting (higher scores = more weight)
- Integration with ScenarioPersistenceManager for rotation
- Automatic vote statistics tracking
- Configurable weight ranges

**Usage:**
```cpp
// Use EnhancedScenarioTransitionComponent instead of base
UPROPERTY(Replicated)
UEnhancedScenarioTransitionComponent* ScenarioTransition;

// Configure in Blueprint or C++
ScenarioTransition->MinimumVoteWeight = 0.5f;
ScenarioTransition->MaximumVoteWeight = 2.0f;
ScenarioTransition->bUseRotationLogic = true;
```

---

## Scenario Persistence & Rotation

### Persistence Manager

The `ScenarioPersistenceManager` subsystem automatically tracks:
- Times each scenario has been played
- Total votes received
- Average player count
- Last played timestamp
- Custom rotation weights

**Auto-tracking:** Statistics are automatically recorded when scenarios activate. No manual setup required!

**Persistence Location:** `ProjectSaved/ScenarioStats.json` (configurable)

### Using Persistence for Rotation

```cpp
// Get the persistence manager
UScenarioPersistenceManager* PersistenceManager =
    GameInstance->GetSubsystem<UScenarioPersistenceManager>();

// Select scenarios using weighted rotation (avoids recently played)
TArray<FPrimaryAssetId> Scenarios =
    PersistenceManager->SelectScenariosWeighted(3, FilterQuery);

// Get least recently played scenarios
TArray<FPrimaryAssetId> LeastRecent =
    PersistenceManager->GetLeastRecentlyPlayed(5, FilterQuery);

// Get most popular scenarios (by votes)
TArray<FPrimaryAssetId> MostPopular =
    PersistenceManager->GetMostPopular(5, FilterQuery);

// Query stats for a specific scenario
FScenarioStats Stats;
if (PersistenceManager->GetScenarioStats(ScenarioId, Stats))
{
    UE_LOG(LogTemp, Log, TEXT("Times played: %d, Total votes: %d"),
        Stats.TimesPlayed, Stats.TotalVotes);
}
```

### Custom Rotation Logic

Extend the persistence manager for your game's specific needs:

```cpp
UCLASS()
class UMyGamePersistenceManager : public UScenarioPersistenceManager
{
    GENERATED_BODY()

public:
    // Override weight calculation
    virtual float CalculateRotationWeight_Implementation(FPrimaryAssetId ScenarioId) const override
    {
        float Weight = Super::CalculateRotationWeight_Implementation(ScenarioId);

        // Add your custom logic
        // Example: Boost weight for scenarios matching current time of day
        if (IsNightTime() && ScenarioId.PrimaryAssetName.ToString().Contains("Night"))
        {
            Weight *= 1.5f;
        }

        return Weight;
    }

    // Override persistence path for per-server configs
    virtual FString GetPersistencePath_Implementation() const override
    {
        // Example: Separate stats per server instance
        return FPaths::ProjectSavedDir() / FString::Printf(TEXT("Server_%d_Stats.json"), ServerID);
    }
};
```

---

## Extending the Plugin

### Philosophy: Subclass, Don't Modify

All major systems are designed as base classes with virtual methods and Blueprint events. **Never modify the plugin code directly** - instead, create subclasses in your project.

### Extension Points

| System | Base Class | Key Overrides |
|--------|------------|---------------|
| Voting | `UScenarioTransitionComponent` | `GetVoteWeight`, `SelectScenarioOptions`, `ShouldAllowVote` |
| Persistence | `UScenarioPersistenceManager` | `CalculateRotationWeight`, `GetPersistencePath` |
| Scenario Actions | `UGameplayScenarioAction` | `OnScenarioActivated`, `OnScenarioDeactivated` |

### Creating Custom Scenario Actions

```cpp
// MyCustomScenarioAction.h
UCLASS()
class UMyCustomScenarioAction : public UGameplayScenarioAction
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    float MyCustomParameter;

    virtual void OnScenarioActivated(UScenarioInstanceSubsystem* Subsystem) override
    {
        // Your activation logic
    }

    virtual void OnScenarioDeactivated(UScenarioInstanceSubsystem* Subsystem, bool bTearDown) override
    {
        // Your cleanup logic
    }
};
```

Add your custom action to scenario data assets in the editor!

---

## Multiplayer Best Practices

### Server Authority

- **Scenario activation is server-only** - Clients receive scenarios via replication
- **Voting is client-initiated, server-validated** - Use `CastVote()` from clients
- **All scenario logic runs on server** - Client activations are for display only

### Network Replication

Scenarios use `FFastArraySerializer` for efficient delta replication:
- Only changed scenarios are sent to clients
- Automatic client-side activation/deactivation
- Minimal bandwidth usage

### Performance Considerations

- Scenarios use **async asset loading** where possible
- Level streaming is **non-blocking** on clients
- Persistence auto-saves are **throttled** to avoid hitches
- Console commands include **validation and error handling**

### Dedicated Server Checklist

- [ ] Configure default scenario in Project Settings
- [ ] Set up scenario whitelist/blacklist if needed
- [ ] Configure voting duration and options count
- [ ] Enable persistence for rotation tracking
- [ ] Add ScenarioTransitionComponent to your GameState
- [ ] Test console commands in standalone dedicated server
- [ ] Verify scenario replication with multiple clients
- [ ] Check persistence file location and permissions

---

## Troubleshooting

### Scenarios Not Loading
- Verify asset references are valid in Content Browser
- Check for circular dependencies between scenarios
- Ensure required plugins (GameplayAbilities, ModularGameplay) are enabled
- Use `ListScenarios` console command to verify asset registry

### Voting Not Working
- Ensure `bEnableScenarioVoting=true` in settings
- Check minimum player count is met
- Verify ScenarioTransitionComponent is replicated
- Look for validation errors in server log

### Network Synchronization Issues
- Scenarios must be activated on server (check `GetOwnerRole() == ROLE_Authority`)
- Verify GamestateScenarioComponent is added to GameState
- Check that scenario assets are cooked for clients
- Ensure network role checks before RPC calls

### Persistence Not Saving
- Check file path permissions in `ProjectSaved/` directory
- Verify `bEnableScenarioPersistence=true` in settings
- Look for JSON serialization errors in log
- Manually call `SaveStats()` to test persistence

---

## API Reference

### Core Classes

- **UScenarioInstanceSubsystem** - Main scenario management subsystem
- **UGameplayScenario** - Data asset defining a gameplay scenario
- **UGameplayScenarioAction** - Base class for scenario actions
- **UGamestateScenarioComponent** - Network replication component
- **UScenarioTransitionComponent** - Voting and transition system
- **UScenarioPersistenceManager** - Statistics and rotation tracking
- **USharedGamemodeSettings** - Project settings object

### Scenario Actions

- **ActivateScenario** - Activate child scenarios
- **DeactivateScenario** - Deactivate scenarios by tag query
- **AddComponents** - Add modular gameplay components
- **StreamLevelInstance** - Stream dynamic level instances
- **ApplyGASPrimitives** - (In Progress) Apply GAS abilities/attributes

---

## Future Development

- Enhanced UE5 support and optimization
- Async asset loading improvements for faster scenario transitions
- Additional GAS integration for gameplay effects
- Example game modes demonstrating advanced usage
- Blueprint-only scenario action support
- Admin permission system for console commands

---

## License

Licensed under the Apache License, Version 2.0. See LICENSE file for details.

## Contributing

This plugin is designed for the community. If you create useful extensions or improvements:
1. Keep them as subclasses in your own project
2. Share examples and documentation with the community
3. Report bugs and suggest features via GitHub issues

**Remember:** Subclass, don't modify!

