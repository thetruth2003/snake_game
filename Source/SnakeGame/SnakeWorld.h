#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "SnakeWorld.generated.h"

UCLASS()
class SNAKEGAME_API ASnakeWorld : public AActor
{
	GENERATED_BODY()
    
public:    
	ASnakeWorld();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USceneComponent* SceneComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInstancedStaticMeshComponent* InstancedWalls;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInstancedStaticMeshComponent* InstancedFloors;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> DoorActor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	TSubclassOf<AActor> FoodClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	float FoodSpawnDelay = 0.5f;
	
	UPROPERTY()
	TArray<AActor*> SpawnedActors;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UFUNCTION()
	void SpawnFood();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level")
	int32 LevelIndex = 1;
	
	UFUNCTION(BlueprintCallable, Category="Level")
	void LoadLevelFromText();
	
	UFUNCTION(BlueprintCallable, Category="Level")
	bool DoesLevelExist(int32 Index) const;

protected:
	virtual void BeginPlay() override;
	

public:    
	virtual void Tick(float DeltaTime) override;
	const TArray<FVector>& GetFloorTileLocations() const { return FloorTileLocations; }
	TArray<FVector> FloorTileLocations;
	
};
