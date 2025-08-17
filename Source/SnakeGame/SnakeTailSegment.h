#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnakeTailSegment.generated.h"

UCLASS()
class SNAKEGAME_API ASnakeTailSegment : public AActor
{
	GENERATED_BODY()
	
public:	
	ASnakeTailSegment();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	bool bCanCollide = false;
};
