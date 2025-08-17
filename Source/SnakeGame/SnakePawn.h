#pragma once

#include "CoreMinimal.h"
#include "Definitions.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"    
#include "Sound/SoundBase.h"
#include "SnakePawn.generated.h"

class ASnakeTailSegment;

UCLASS()
class SNAKEGAME_API ASnakePawn : public APawn
{
	GENERATED_BODY()

public:
	ASnakePawn();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* SceneComponent;

	UPROPERTY(EditAnywhere, Category = "Effects")
	UParticleSystem* EatParticle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* EatSound;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Detection", meta=(AllowPrivateAccess="true"))
	USphereComponent* ProximitySphere;  
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	UWidgetComponent* QuestionMarkWidget;  
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	USoundBase* NoticeSound; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESnakeDirection Direction = ESnakeDirection::None;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake")
	TArray<ASnakeTailSegment*> TailSegments;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake")
	FVector LastTilePosition;
	
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void GrowTail();
	
	void UpdateTailTargets(const FVector& PreviousHeadPosition);
	
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void UpdateTailPositions(const FVector& PreviousTilePosition);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake")
	TArray<FVector> TailTargetPositions;
	
	virtual void Tick(float DeltaTime) override;

	void HandlePauseToggle();
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Makes the snake jump."))
	void Jump();
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Add a direction onto a queue where the first in line direction gets set and popped."))
	void SetNextDirection(ESnakeDirection InDirection);
	
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult & SweepResult);
	
	UFUNCTION(BlueprintCallable, Category = "Game")
	void GameOver();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snake")
	TSubclassOf<ASnakeTailSegment> TailSegmentClass;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ToolTip = "Used for falling and jumping."))
	float VelocityZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ToolTip = "If the snake is in air."))
	bool bInAir = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ToolTip = "Speed of the snake (cm / second)."))
	float Speed = 500.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ToolTip = "The forward rotation of the snake."))
	FRotator ForwardRotation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<ESnakeDirection> DirectionQueue;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ToolTip = "How long the snake has moved since reaching the last tile."))
	float MovedTileDistance = 0.0f;
	
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void UpdateDirection();

	FVector GetDirectionVector() const;
	
	UFUNCTION()
	void UpdateMovement(float DeltaTime);
	
	UFUNCTION()
	void MoveSnake(float Distance);
	
	UFUNCTION()
	void UpdateFalling(float DeltaTime);

private:
	TArray<FVector> HeadPositionHistory;
	
	UPROPERTY(EditAnywhere, Category = "Snake|Tail")
	int32 TailHistorySpacing = 5;
	
	static FVector SnapToGrid(const FVector& InLocation);

	FTimerHandle QuestionMarkTimerHandle;
	
	UFUNCTION()
	void OnProximityOverlapBegin(UPrimitiveComponent* OverlappedComp,
								 AActor* OtherActor,
								 UPrimitiveComponent* OtherComp,
								 int32 OtherBodyIndex,
								 bool bFromSweep,
								 const FHitResult& SweepResult);
	
	void HideQuestionMark();
};
