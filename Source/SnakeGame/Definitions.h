#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"

UENUM(BlueprintType)
enum class ESnakeDirection : uint8
{
	Up    = 0,
	Right = 1,
	Down  = 2,
	Left  = 3,
	None  = 255
};

constexpr float TileSize = 100.0f;

// Unified helper to snap any world position to the nearest grid center
static FORCEINLINE FVector SnapToGrid(const FVector& InLocation)
{
	float X = FMath::RoundToFloat(InLocation.X / TileSize) * TileSize;
	float Y = FMath::RoundToFloat(InLocation.Y / TileSize) * TileSize;
	return FVector(X, Y, InLocation.Z);
}
