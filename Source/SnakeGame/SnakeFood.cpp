#include "SnakeFood.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"  // ← add this

ASnakeFood::ASnakeFood()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(MeshComponent);
	GlowLight->SetIntensity(3000.0f); 
	GlowLight->SetAttenuationRadius(200.0f);
	GlowLight->SetLightColor(FLinearColor::Green);
	GlowLight->bUseInverseSquaredFalloff = false;
	GlowLight->SetCastShadows(false);
}
