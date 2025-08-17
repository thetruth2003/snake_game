#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "SnakePlayerController.generated.h"

UCLASS()
class SNAKEGAME_API ASnakePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* P1Mapping;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* P2Mapping;
	

	// Tested for one controller for two players
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* SnakeMapping;

protected:
	virtual void BeginPlay() override;
};
