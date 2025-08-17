#include "SnakeGameMode.h"
#include "MyUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "SnakeWorld.h"
#include "SnakeAIController.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Components/AudioComponent.h"

EGameType ASnakeGameMode::ToV2Variant(EGameType BaseType)
{
    switch (BaseType)
    {
    case EGameType::SinglePlayer: return EGameType::SinglePlayerV2;
    case EGameType::PvP:          return EGameType::PvPV2;
    case EGameType::Coop:         return EGameType::CoopV2;
    case EGameType::PvAI:         return EGameType::PvAIV2;
    case EGameType::CoopAI:       return EGameType::CoopAIV2;
    default:                      return BaseType;
    }
}

EGameType ASnakeGameMode::ToBaseVariant(EGameType Type)
{
    switch (Type)
    {
    case EGameType::SinglePlayerV2: return EGameType::SinglePlayer;
    case EGameType::PvPV2:          return EGameType::PvP;
    case EGameType::CoopV2:         return EGameType::Coop;
    case EGameType::PvAIV2:         return EGameType::PvAI;
    case EGameType::CoopAIV2:       return EGameType::CoopAI;
    default:                        return Type;
    }
}

ASnakeGameMode::ASnakeGameMode()
    : CurrentWidget(nullptr)
    , PauseWidget(nullptr)
    , InGameWidget(nullptr)
    , SpawnedAISnake(nullptr)
    , ApplesToFinish(5)
    , ApplesEaten(0)
    , Score(0)
    , LevelApplesP1(0)
    , LevelApplesP2(0)
    , CurrentGameType(EGameType::SinglePlayer)
    , CurrentState(EGameState::MainMenu)
{
}

void ASnakeGameMode::BeginPlay()
{
    Super::BeginPlay();
    SetGameState(CurrentState);
    if (AmbientSound)
    {
        UGameplayStatics::SpawnSound2D(GetWorld(), AmbientSound);
        AmbientAudioComponent = UGameplayStatics::SpawnSound2D(GetWorld(), AmbientSound);
    }
}

void ASnakeGameMode::SetGameType(EGameType NewType)
{
    // 1) If depth-level is on, swap in the V2 sibling.
    if (bEnable3DDepthLevel)
    {
        NewType = ToV2Variant(NewType);
    }

    // 2) Work off the *base* version for all internal spawn/UI checks:
    const EGameType BaseType = ToBaseVariant(NewType);

    // Cleanup extra local player 
    if (GetGameInstance()->GetNumLocalPlayers() > 1
        && NewType != EGameType::Coop
        && NewType != EGameType::PvP
        && NewType != EGameType::CoopAI)
    {
        if (ULocalPlayer* Extra = GetGameInstance()->GetLocalPlayers()[1])
        {
            GetGameInstance()->RemoveLocalPlayer(Extra);
            UE_LOG(LogTemp, Log,
                   TEXT("Removed extra local player when switching to %s"),
                   *UEnum::GetValueAsString(NewType));
        }
    }

    // Cleanup AI snake
    if (SpawnedAISnake
        && NewType != EGameType::PvAI
        && NewType != EGameType::CoopAI)
    {
        if (AController* AICon = SpawnedAISnake->GetController())
        {
            AICon->Destroy();
        }
        SpawnedAISnake->Destroy();
        SpawnedAISnake = nullptr;
        UE_LOG(LogTemp, Log,
               TEXT("Destroyed AI snake when switching to %s"),
               *UEnum::GetValueAsString(NewType));
    }

    // Core spawn logic  
    CurrentGameType = NewType;
    UE_LOG(LogTemp, Log, TEXT("GameType set to %s"),
           *UEnum::GetValueAsString(NewType));

    UWorld* W = GetWorld();
    if (!W) return;

    // Spawn second human player
    if ((NewType == EGameType::Coop || NewType == EGameType::PvP)
        && GetGameInstance()->GetNumLocalPlayers() < 2)
    {
        UGameplayStatics::CreatePlayer(W, 1, true);
        UE_LOG(LogTemp, Log, TEXT("Created second local player (ID 1)"));
    }

    // Spawn AI snake
    if (NewType == EGameType::PvAI || NewType == EGameType::CoopAI)
    {
        if (!IsValid(SpawnedAISnake))
        {
            FTransform SpawnT;
            APlayerStart* P2 = nullptr;
            TArray<AActor*> Starts;
            UGameplayStatics::GetAllActorsOfClass(W, APlayerStart::StaticClass(), Starts);
            for (AActor* A : Starts)
            {
                if (A->ActorHasTag(TEXT("PlayerStart2")))
                {
                    P2 = Cast<APlayerStart>(A);
                    break;
                }
            }

            if (P2)
            {
                SpawnT = P2->GetActorTransform();
            }
            else
            {
                FVector RawFallback(400.f, 1000.f, 0.f);
                FVector Snapped = SnapToGrid(RawFallback);
                SpawnT.SetLocation(Snapped);
                UE_LOG(LogTemp, Warning,
                       TEXT("PlayerStart2 not found, using fallback at %s."),
                       *Snapped.ToString());
            }

            auto& ChosenBP = AISnakePawnBP ? AISnakePawnBP : Player2PawnBP;
            if (ChosenBP)
            {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                ASnakePawn* NewAI = W->SpawnActor<ASnakePawn>(ChosenBP, SpawnT, Params);
                if (NewAI)
                {
                    SpawnedAISnake = NewAI;
                    ASnakeAIController* AICon = W->SpawnActor<ASnakeAIController>(
                        ASnakeAIController::StaticClass());
                    if (AICon)
                    {
                        AICon->Possess(NewAI);
                        UE_LOG(LogTemp, Log,
                               TEXT("Spawned & possessed AI snake with %s"),
                               *ChosenBP->GetName());
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI snake already spawned; skipping."));
        }
    }
    
    SetGameState(EGameState::Game);
}

void ASnakeGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    int32 Id = NewPlayer->GetLocalPlayer()->GetControllerId();
    if (Id == 1 && (CurrentGameType == EGameType::Coop || CurrentGameType == EGameType::PvP))
    {
        APlayerStart* TargetStart = nullptr;
        TArray<AActor*> Starts;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Starts);
        for (AActor* Actor : Starts)
        {
            if (Actor->ActorHasTag(TEXT("PlayerStart2")))
            {
                TargetStart = Cast<APlayerStart>(Actor);
                break;
            }
        }

        FTransform SpawnTransform;
        if (TargetStart)
        {
            SpawnTransform = TargetStart->GetActorTransform();
        }
        else
        {
            FVector RawFallback(400.f, 1000.f, 0.f);
            FVector Snapped = SnapToGrid(RawFallback);
            SpawnTransform.SetLocation(Snapped);
            UE_LOG(LogTemp, Warning,
                   TEXT("PlayerStart2 not found (PostLogin), using fallback at %s."),
                   *Snapped.ToString());
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        if (Player2PawnBP)
        {
            ASnakePawn* Pawn = GetWorld()->SpawnActor<ASnakePawn>(
                Player2PawnBP, SpawnTransform, Params);
            if (Pawn)
            {
                NewPlayer->Possess(Pawn);
                UE_LOG(LogTemp, Log,
                       TEXT("Spawned P2 at %s"),
                       *SpawnTransform.GetLocation().ToString());
            }
        }
    }
}

void ASnakeGameMode::NotifyAppleEaten(int32 ControllerId)
{
    // Update counters
    if (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
    {
        if (ControllerId == 0)
        {
            ++LevelApplesP1;
            ++TotalApplesP1;
        }
        else
        {
            ++LevelApplesP2;
            ++TotalApplesP2;
        }
    }
    else
    {
        ++ApplesEaten;
    }
    
    ++Score;

    // Update UI
    if (CurrentState == EGameState::Game && InGameWidget)
    {
        if (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
        {
            InGameWidget->SetPlayerScores(TotalApplesP1, TotalApplesP2);
        }
        else
        {
            InGameWidget->SetScore(Score);
        }
    }
    
    ASnakeWorld* World = Cast<ASnakeWorld>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ASnakeWorld::StaticClass())
    );
    if (!World) return;
    
    int32 EatenThisLevel = (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
                           ? (LevelApplesP1 + LevelApplesP2)
                           : ApplesEaten;
    
    if (EatenThisLevel < ApplesToFinish)
    {
        World->SpawnFood();
    }
    else
    {
        SetGameState(EGameState::Pause);
        
        int32 Next = World->LevelIndex + 1;
        if (!World->DoesLevelExist(Next))
        {
            SetGameState(EGameState::Outro);
            return;
        }
        
        World->LevelIndex = Next;
        World->LoadLevelFromText();
        World->SpawnFood();
        
        LevelApplesP1 = 0;
        LevelApplesP2 = 0;
        ApplesEaten   = 0;
        
        SetGameState(EGameState::Game);
    }
}


void ASnakeGameMode::SetGameState(EGameState NewState)
{
    if (CurrentWidget) { CurrentWidget->RemoveFromParent(); CurrentWidget = nullptr; }
    if (PauseWidget)   { PauseWidget->RemoveFromParent();   PauseWidget   = nullptr; }

    CurrentState = NewState;
    switch (CurrentState)
    {
    case EGameState::MainMenu:
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        if (MainMenuWidgetClass)
        {
            CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
            if (CurrentWidget)
            {
                CurrentWidget->AddToViewport();
                
                if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
                {
                    PC->bShowMouseCursor = true;
                    FInputModeUIOnly UIInput;
                    UIInput.SetWidgetToFocus( CurrentWidget->TakeWidget() );
                    UIInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                    PC->SetInputMode(UIInput);
                }
            }
        }
        break;

    case EGameState::Game:
    UGameplayStatics::SetGamePaused(GetWorld(), false);
        if (!InGameWidget && InGameWidgetClass)
        {
            InGameWidget = CreateWidget<UMyUserWidget>(GetWorld(), InGameWidgetClass);
            if (InGameWidget)
            {
                InGameWidget->AddToViewport();

                if (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
                {
                    InGameWidget->ScoreText  ->SetVisibility(ESlateVisibility::Collapsed);
                    InGameWidget->ScoreP1Text->SetVisibility(ESlateVisibility::Visible);
                    InGameWidget->ScoreP2Text->SetVisibility(ESlateVisibility::Visible);
                    InGameWidget->SetPlayerScores(TotalApplesP1, TotalApplesP2);
                }
                else
                {
                    InGameWidget->ScoreText    ->SetVisibility(ESlateVisibility::Visible);
                    InGameWidget->ScoreP1Text  ->SetVisibility(ESlateVisibility::Collapsed);
                    InGameWidget->ScoreP2Text  ->SetVisibility(ESlateVisibility::Collapsed);
                    InGameWidget->SetScore(Score);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create InGameWidget from %s"), *GetNameSafe(InGameWidgetClass));
            }
        }
        if (InGameWidget)
        {
            if (ASnakeWorld* W = Cast<ASnakeWorld>(
                    UGameplayStatics::GetActorOfClass(GetWorld(), ASnakeWorld::StaticClass())))
            {
                InGameWidget->SetLevel(W->LevelIndex);
            }
            else
            {
                InGameWidget->SetLevel(1);
            }
        }
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
        break;


    case EGameState::Pause:
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        if (PauseMenuWidgetClass)
        {
            auto* UW = CreateWidget<UMyUserWidget>(GetWorld(), PauseMenuWidgetClass);
            PauseWidget = UW;
            if (UW)
            {
                UW->AddToViewport();
                
                if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
                {
                    PC->bShowMouseCursor = true;
                    FInputModeUIOnly UIInput;
                    UIInput.SetWidgetToFocus(UW->TakeWidget());
                    UIInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                    PC->SetInputMode(UIInput);
                }
                
                if (ASnakeWorld* W = Cast<ASnakeWorld>(
                        UGameplayStatics::GetActorOfClass(GetWorld(), ASnakeWorld::StaticClass())))
                {
                    UW->SetLevel(W->LevelIndex);
                }
                if (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
                {
                    UW->ScoreText  ->SetVisibility(ESlateVisibility::Collapsed);
                    UW->ScoreP1Text->SetVisibility(ESlateVisibility::Visible);
                    UW->ScoreP2Text->SetVisibility(ESlateVisibility::Visible);
                    
                    UW->SetPlayerScores(TotalApplesP1, TotalApplesP2);
                }
                else
                {
                    UW->ScoreText  ->SetVisibility(ESlateVisibility::Visible);
                    UW->ScoreP1Text->SetVisibility(ESlateVisibility::Collapsed);
                    UW->ScoreP2Text->SetVisibility(ESlateVisibility::Collapsed);
                    UW->SetScore(Score);
                }
            }
        }
        break;

    case EGameState::Outro:
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        if (GameOverWidgetClass)
        {
            if (AmbientAudioComponent)
            {
                AmbientAudioComponent->Stop();
            }
            if (GameOverSound)
            {
                UGameplayStatics::SpawnSound2D(GetWorld(), GameOverSound);
            }

            auto* UW = CreateWidget<UMyUserWidget>(GetWorld(), GameOverWidgetClass);
            CurrentWidget = UW;
            if (UW)
            {
                UW->AddToViewport();

                if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
                {
                    PC->bShowMouseCursor = true;
                    FInputModeUIOnly UIInput;
                    UIInput.SetWidgetToFocus(UW->TakeWidget());
                    UIInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                    PC->SetInputMode(UIInput);
                }
                
                if (ASnakeWorld* W = Cast<ASnakeWorld>(
                        UGameplayStatics::GetActorOfClass(GetWorld(), ASnakeWorld::StaticClass())))
                {
                    UW->SetLevel(W->LevelIndex);
                }
                if (CurrentGameType == EGameType::PvP || CurrentGameType == EGameType::PvAI)
                {
                    UW->ScoreText  ->SetVisibility(ESlateVisibility::Collapsed);
                    UW->ScoreP1Text->SetVisibility(ESlateVisibility::Visible);
                    UW->ScoreP2Text->SetVisibility(ESlateVisibility::Visible);
                    
                    UW->SetPlayerScores(TotalApplesP1, TotalApplesP2);
                }
                else
                {
                    UW->ScoreText  ->SetVisibility(ESlateVisibility::Visible);
                    UW->ScoreP1Text->SetVisibility(ESlateVisibility::Collapsed);
                    UW->ScoreP2Text->SetVisibility(ESlateVisibility::Collapsed);
                    UW->SetScore(Score);
                }
            }
        }
        break;
    }
}


AActor* ASnakeGameMode::ChoosePlayerStart_Implementation(AController* Controller)
{
    int32 ControllerId = 0;
    
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            ControllerId = LP->GetControllerId();
        }
    }
    
    FName DesiredTag = (ControllerId == 1) ? TEXT("PlayerStart2") : TEXT("PlayerStart1");
    
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(DesiredTag))
        {
            UE_LOG(LogTemp, Log, TEXT("Spawning Controller %d at %s"), 
                   ControllerId, *DesiredTag.ToString());
            return *It;
        }
    }
    
    return Super::ChoosePlayerStart_Implementation(Controller);
}

void ASnakeGameMode::RestartGame()
{
    UWorld* W = GetWorld();
    if (!W) return;
    
    FString MapName = W->GetMapName();
    MapName.RemoveFromStart(W->StreamingLevelsPrefix);
    
    UGameplayStatics::OpenLevel(W, FName(*MapName));
}

FText ASnakeGameMode::GetCurrentGameTypeText() const
{
    // find the enum by name
    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameType"));
    if (!EnumPtr)
    {
        return FText::FromString(TEXT("Unknown"));
    }

    // return its DisplayName (what you set in UMETA(DisplayName="…"))
    return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(CurrentGameType));
}

