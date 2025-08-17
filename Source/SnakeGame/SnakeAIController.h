// SnakeAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Definitions.h"          // for TileSize & ESnakeDirection
#include "SnakeAIController.generated.h"

UCLASS()
class SNAKEGAME_API ASnakeAIController : public AAIController
{
    GENERATED_BODY()

public:
    ASnakeAIController();
    virtual void Tick(float DeltaTime) override;

private:
    bool FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath) const;
    static FVector SnapToGrid(const FVector& WorldPos);
    
    FVector PrevTilePosition = FVector(FLT_MAX);
};
