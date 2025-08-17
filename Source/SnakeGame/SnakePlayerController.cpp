// SnakePlayerController.cpp
#include "SnakePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

void ASnakePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		int32 Id = LP->GetControllerId();
		auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		// Choose P1 or P2 mapping based on controller ID
		UInputMappingContext* ToApply = (Id == 0) ? P1Mapping : P2Mapping;
		if (ToApply)
		{
			Subsystem->AddMappingContext(ToApply, 0);
			UE_LOG(LogTemp, Log, TEXT("Player %d → %s"), Id, *GetNameSafe(ToApply));
		}
	}
}
