#pragma once

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h" 
#include "GameFramework/Actor.h"
#include "SnakeFood.generated.h"

UCLASS()
class SNAKEGAME_API ASnakeFood : public AActor
{
	GENERATED_BODY()

public:    
	ASnakeFood();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
	UPointLightComponent* GlowLight;   // ← new
};
