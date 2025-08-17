#include "SnakePawn.h"
#include "SnakeTailSegment.h"
#include "SnakeFood.h"
#include "SnakeGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "SnakeWorld.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"
#include "Definitions.h"
#include "SnakeAIController.h"

ASnakePawn::ASnakePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetupAttachment(RootComponent);
		
	// Hard override collision
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);

	// Create & configure the proximity sphere for "huh" sound
	ProximitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProximitySphere"));
	ProximitySphere->SetupAttachment(RootComponent);
	ProximitySphere->InitSphereRadius(300.f);
	ProximitySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProximitySphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	ProximitySphere->OnComponentBeginOverlap.AddDynamic(this, &ASnakePawn::OnProximityOverlapBegin);

	// Create question-mark widget
	QuestionMarkWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestionMarkWidget"));
	QuestionMarkWidget->SetupAttachment(RootComponent);
	QuestionMarkWidget->SetWidgetSpace(EWidgetSpace::Screen);
	QuestionMarkWidget->SetDrawAtDesiredSize(true);
	QuestionMarkWidget->SetVisibility(false);
}

void ASnakePawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
		CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		CollisionComponent->SetGenerateOverlapEvents(true);
	}
	
	FVector SnappedLocation = SnapToGrid(GetActorLocation());
	SetActorLocation(SnappedLocation);
	LastTilePosition = SnappedLocation;
	
	if (CollisionComponent)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ASnakePawn::OnOverlapBegin);
	}
}

FVector ASnakePawn::SnapToGrid(const FVector& InLocation)
{
	return ::SnapToGrid(InLocation);
}

void ASnakePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateFalling(DeltaTime);
	UpdateMovement(DeltaTime);

	// Record the head's current 
	const float RecordDistance = 10.0f;
	FVector CurrentHeadPos = GetActorLocation();
	if (HeadPositionHistory.Num() == 0 ||
	    FVector::Dist(HeadPositionHistory.Last(), CurrentHeadPos) >= RecordDistance)
	{
		HeadPositionHistory.Add(CurrentHeadPos);
	}
	
	int32 MaxHistoryLength = (TailSegments.Num() + 1) * TailHistorySpacing + 10;
	if (HeadPositionHistory.Num() > MaxHistoryLength)
	{
		HeadPositionHistory.RemoveAt(0, HeadPositionHistory.Num() - MaxHistoryLength);
	}

	// Update tail to follow the head history
	const float SmoothSpeed = 10.0f;
	for (int32 i = 0; i < TailSegments.Num(); i++)
	{
		int32 HistoryIndex = HeadPositionHistory.Num() - 1 - (i + 1) * TailHistorySpacing;
		HistoryIndex = FMath::Clamp(HistoryIndex, 0, HeadPositionHistory.Num() - 1);

		FVector TargetPos = HeadPositionHistory[HistoryIndex];
		FVector CurrentPos = TailSegments[i]->GetActorLocation();
		FVector NewPos = FMath::VInterpTo(CurrentPos, TargetPos, DeltaTime, SmoothSpeed);
		TailSegments[i]->SetActorLocation(NewPos);
	}
}

void ASnakePawn::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	// Collision with food
	if (OtherActor->IsA(ASnakeFood::StaticClass()))
	{
		GrowTail();

		if (EatParticle)
		{
			FVector SpawnLoc = OtherActor->GetActorLocation();
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(), EatParticle, SpawnLoc
			);
		}

		if (EatSound)
		{
			FVector SpawnLoc = OtherActor->GetActorLocation();
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), EatSound, SpawnLoc);
		}
		
		OtherActor->Destroy();

		// Notify GameMode
		ASnakeGameMode* GM = Cast<ASnakeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GM)
		{
			int32 ControllerId = 0;
			AController* Con = GetController();

			if (APlayerController* PC = Cast<APlayerController>(Con))
			{
				ControllerId = PC->GetLocalPlayer()->GetControllerId();
			}
			else if (Cast<ASnakeAIController>(Con))
			{
				// AI always counts as player 2
				ControllerId = 1;
			}

			GM->NotifyAppleEaten(ControllerId);
		}
	}

	// Collision with Tail
	if (OtherActor->IsA(ASnakeTailSegment::StaticClass()))
	{
		ASnakeTailSegment* TailSeg = Cast<ASnakeTailSegment>(OtherActor);
		if (TailSeg && TailSeg->bCanCollide)
		{
			UE_LOG(LogTemp, Warning, TEXT("Collision with tail detected! Game Over!"));
			GameOver();
			return;
		}
		else
		{
			return;
		}
	}

	// Collision with Walls
	if (OtherActor->ActorHasTag("Wall") || (OtherComp && OtherComp->ComponentHasTag("Wall")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Collision with wall detected! Game Over!"));
		GameOver();
		return;
	}
}



void ASnakePawn::GameOver()
{
	UE_LOG(LogTemp, Warning, TEXT("Game Over triggered in GameOver() function."));
	
	ASnakeGameMode* GameMode = Cast<ASnakeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		GameMode->SetGameState(EGameState::Outro);
	}

	/* // Restart the current level.
	UWorld* World = GetWorld();
	if (World)
	{
		FString CurrentLevel = World->GetMapName();
		CurrentLevel.RemoveFromStart(World->StreamingLevelsPrefix);
		UGameplayStatics::OpenLevel(World, FName(*CurrentLevel));
	}
	*/
}

void ASnakePawn::OnProximityOverlapBegin(UPrimitiveComponent* OverlappedComp,
										 AActor* OtherActor,
										 UPrimitiveComponent* OtherComp,
										 int32 OtherBodyIndex,
										 bool bFromSweep,
										 const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA(ASnakeFood::StaticClass()))
	{
		// play notice sound "huh"
		if (NoticeSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, NoticeSound, GetActorLocation());
		}

		// show the question-mark widget briefly
		if (QuestionMarkWidget)
		{
			QuestionMarkWidget->SetVisibility(true);
			GetWorld()->GetTimerManager().SetTimer(
				QuestionMarkTimerHandle,
				this,
				&ASnakePawn::HideQuestionMark,
				0.5f,    // half a second
				false
			);
		}
	}
}

void ASnakePawn::HideQuestionMark()
{
	if (QuestionMarkWidget)
	{
		QuestionMarkWidget->SetVisibility(false);
	}
}

void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &ASnakePawn::HandlePauseToggle);
	PlayerInputComponent->BindAction("TriggerGameOver", IE_Pressed, this, &ASnakePawn::GameOver);

}

void ASnakePawn::HandlePauseToggle()
{
	ASnakeGameMode* GM = Cast<ASnakeGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return;
	
	if (GM->GetCurrentState() == EGameState::Game)
	{
		GM->SetGameState(EGameState::Pause);
	}
	else if (GM->GetCurrentState() == EGameState::Pause)
	{
		GM->SetGameState(EGameState::Game);
	}
}

void ASnakePawn::MoveSnake(float Distance)
{
	FVector Position = GetActorLocation();
	
	switch (Direction)
	{
	case ESnakeDirection::Up:
		Position.X += Distance;
		break;
	case ESnakeDirection::Right:
		Position.Y += Distance;
		break;
	case ESnakeDirection::Down:
		Position.X -= Distance;
		break;
	case ESnakeDirection::Left:
		Position.Y -= Distance;
		break;
	}

	SetActorLocation(Position);
	MovedTileDistance += Distance;
}

void ASnakePawn::UpdateMovement(float DeltaTime)
{
	float DistanceToTravel = Speed * DeltaTime;
	FVector CurrentPosition = GetActorLocation();

	while (DistanceToTravel > 0.f)
	{
		float Step = FMath::Min(DistanceToTravel, TileSize - MovedTileDistance);
		CurrentPosition += GetDirectionVector() * Step;
		MovedTileDistance += Step;
		DistanceToTravel -= Step;

		if (MovedTileDistance >= TileSize - KINDA_SMALL_NUMBER)
		{
			// Snap exactly to grid, reset counters, update history
			FVector Snapped = SnapToGrid(CurrentPosition);
			LastTilePosition = Snapped;
			CurrentPosition = Snapped;
			MovedTileDistance = 0.f;

			UpdateTailTargets(LastTilePosition);
			UpdateDirection();
		}
	}
	SetActorLocation(CurrentPosition);
}

void ASnakePawn::UpdateFalling(float DeltaTime)
{
	FVector Position = GetActorLocation();
	VelocityZ -= 10.0f * DeltaTime;
	Position.Z += VelocityZ;

	if (Position.Z <= 0.0f)
	{
		Position.Z = -Position.Z;
		VelocityZ = -VelocityZ * 0.5f;

		if (FMath::Abs(VelocityZ) < 0.1f)
		{
			VelocityZ = 0.0f;
			Position.Z = 0.0f;
			bInAir = false;
		}
	}
	else
	{
		bInAir = true;
	}

	SetActorLocation(Position);
}

void ASnakePawn::Jump()
{
	if (!bInAir)
	{
		VelocityZ = 2.5f;
	}
}


void ASnakePawn::UpdateDirection()
{
	if (DirectionQueue.IsEmpty())
	{
		return;
	}

	Direction = DirectionQueue[0];
	DirectionQueue.RemoveAt(0);
	switch (Direction)
	{
	case ESnakeDirection::Up:
		ForwardRotation = FRotator(0.0f, 0.0f, 0.0f);
		break;
	case ESnakeDirection::Right:
		ForwardRotation = FRotator(0.0f, 90.0f, 0.0f);
		break;
	case ESnakeDirection::Down:
		ForwardRotation = FRotator(0.0f, 180.0f, 0.0f);
		break;
	case ESnakeDirection::Left:
		ForwardRotation = FRotator(0.0f, 270.0f, 0.0f);
		break;
	}
}

// Return unit vector based on the current direction
FVector ASnakePawn::GetDirectionVector() const
{
	switch (Direction)
	{
	case ESnakeDirection::Up:
		return FVector(1.0f, 0.0f, 0.0f);
	case ESnakeDirection::Right:
		return FVector(0.0f, 1.0f, 0.0f);
	case ESnakeDirection::Down:
		return FVector(-1.0f, 0.0f, 0.0f);
	case ESnakeDirection::Left:
		return FVector(0.0f, -1.0f, 0.0f);
	default:
		return FVector::ZeroVector;
	}
}

// Add a new direction to the movement queue
void ASnakePawn::SetNextDirection(ESnakeDirection InDirection)
{
	DirectionQueue.Add(InDirection);
}

void ASnakePawn::GrowTail()
{
	if (!GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;  
	UClass* SpawnClass = TailSegmentClass.Get() ? TailSegmentClass.Get()
												: ASnakeTailSegment::StaticClass();

	ASnakeTailSegment* NewSegment = GetWorld()->SpawnActor<ASnakeTailSegment>(
		SpawnClass,
		LastTilePosition,
		FRotator::ZeroRotator,
		SpawnParams
	);

	
	if (NewSegment)
	{
		if (UStaticMeshComponent* HeadMesh = FindComponentByClass<UStaticMeshComponent>())
		{
			if (UMaterialInterface* HeadMat = HeadMesh->GetMaterial(0))
			{
				NewSegment->MeshComponent->SetMaterial(0, HeadMat);
			}
		}
		
		NewSegment->MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewSegment->bCanCollide = false;
		
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDel;
		TimerDel.BindLambda([NewSegment]()
		{
			if (NewSegment)
			{
				NewSegment->MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				NewSegment->bCanCollide = true;
			}
		});
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 0.3f, false);
		
		TailSegments.Add(NewSegment);
		TailTargetPositions.Add(LastTilePosition);

		UE_LOG(LogTemp, Warning, TEXT("Tail grown. Total segments: %d"), TailSegments.Num());
	}
}



void ASnakePawn::UpdateTailTargets(const FVector& PreviousHeadPosition)
{
	if (TailSegments.Num() > 0)
	{
		TailTargetPositions[0] = PreviousHeadPosition;
		for (int32 i = 1; i < TailSegments.Num(); i++)
		{
			TailTargetPositions[i] = TailTargetPositions[i - 1];
		}
	}
}

void ASnakePawn::UpdateTailPositions(const FVector& PreviousTilePosition)
{
	if (TailSegments.Num() > 0)
	{
		TArray<FVector> OldPositions;
		OldPositions.SetNum(TailSegments.Num());

		for (int32 i = 0; i < TailSegments.Num(); i++)
		{
			OldPositions[i] = TailSegments[i]->GetActorLocation();
		}

		TailSegments[0]->SetActorLocation(PreviousTilePosition);

		for (int32 i = 1; i < TailSegments.Num(); i++)
		{
			TailSegments[i]->SetActorLocation(OldPositions[i - 1]);
		}
	}
}
