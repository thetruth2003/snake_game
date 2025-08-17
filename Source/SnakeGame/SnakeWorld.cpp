#include "SnakeWorld.h"

#include "Definitions.h"
#include "Engine/World.h"
#include "SnakeFood.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ASnakeWorld::ASnakeWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    
    InstancedWalls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedWalls"));
    InstancedWalls->SetupAttachment(RootComponent);
    InstancedWalls->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InstancedWalls->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    InstancedWalls->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
    InstancedWalls->ComponentTags.Empty();
    InstancedWalls->ComponentTags.Add(FName("Wall"));
    
    InstancedFloors = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedFloors"));
    InstancedFloors->SetupAttachment(RootComponent);
}

void ASnakeWorld::OnConstruction(const FTransform& Transform)
{
    UE_LOG(LogTemp, Log, TEXT("OnConstruction Called!"));

    // Clean up previous instances and spawned actors
    InstancedWalls->ClearInstances();
    InstancedFloors->ClearInstances();
    for (AActor* Actor : SpawnedActors)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    SpawnedActors.Empty();
    
    // Clear previous floor tile locations.
    FloorTileLocations.Empty();


    InstancedWalls->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InstancedWalls->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    InstancedWalls->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
    if (!InstancedWalls->ComponentTags.Contains(FName("Wall")))
    {
        InstancedWalls->ComponentTags.Empty();
        InstancedWalls->ComponentTags.Add(FName("Wall"));
    }
    LoadLevelFromText();
}

void ASnakeWorld::BeginPlay()
{
    Super::BeginPlay();
    SpawnFood();
}

void ASnakeWorld::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

bool ASnakeWorld::DoesLevelExist(int32 Index) const
{
    const FString FileName = FString::Printf(TEXT("Levels/Level%d.txt"), Index);
    const FString FullPath = FPaths::ProjectContentDir() / FileName;
    return FPlatformFileManager::Get().GetPlatformFile().FileExists(*FullPath);
}

void ASnakeWorld::LoadLevelFromText()
{
    InstancedWalls->ClearInstances();
    InstancedFloors->ClearInstances();
    for (AActor* Actor : SpawnedActors)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    SpawnedActors.Empty();
    FloorTileLocations.Empty();

    FString FileName = FString::Printf(TEXT("Levels/Level%d.txt"), LevelIndex);
    FString FilePath = FPaths::ProjectContentDir() + FileName;
    UE_LOG(LogTemp, Warning, TEXT("[LevelLoad] Attempting to load: %s"), *FilePath);
    
    TArray<FString> Lines;

    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelLoad] Failed to load file!"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[LevelLoad] Loaded %d lines"), Lines.Num());
    
    if (FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        int y = 0;
        for (const FString& Line : Lines)
        {
            for (int x = 0; x < Line.Len(); x++)
            {
                FTransform TileTransform(FRotator::ZeroRotator, FVector((Lines.Num() - y) * 100, x * 100, 0.0f));
                
                switch (Line[x])
                {
                    case '#':
                        InstancedWalls->AddInstance(TileTransform);
                        break;

                    case 'O':
                        break;

                    case 'D':
                        InstancedFloors->AddInstance(TileTransform);
                        if (IsValid(DoorActor))
                        {
                            AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(DoorActor, TileTransform, FActorSpawnParameters());
                            if (SpawnedActor)
                            {
                                SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
                                SpawnedActors.Add(SpawnedActor);
                            }
                        }
                        break;

                    case '.':
                        InstancedFloors->AddInstance(TileTransform);
                        FloorTileLocations.Add(TileTransform.GetTranslation());
                        break;
                }
            }
            y++;
        }
    } 
}

void ASnakeWorld::SpawnFood()
{
    if (!FoodClass || FloorTileLocations.Num() == 0)
        return;
    
    TSet<FVector> FloorSet(FloorTileLocations);
    TArray<FVector> ValidSpawnTiles;
    FVector Offsets[4] = {
        FVector(TileSize,  0,       0),
        FVector(-TileSize, 0,       0),
        FVector(0,        TileSize, 0),
        FVector(0,       -TileSize, 0)
    };

    for (const FVector& Loc : FloorTileLocations)
    {
        bool bSurrounded = true;
        for (auto& Offset : Offsets)
        {
            FVector Neigh = SnapToGrid(Loc + Offset);
            if (!FloorSet.Contains(Neigh))
            {
                bSurrounded = false;
                break;
            }
        }

        if (bSurrounded)
            ValidSpawnTiles.Add(Loc);
    }
    
    const TArray<FVector>& Pool = (ValidSpawnTiles.Num() > 0)
        ? ValidSpawnTiles
        : FloorTileLocations;
    
    int32 Index = FMath::RandRange(0, Pool.Num() - 1);
    FVector Chosen = Pool[Index];
    FVector SpawnLocation = GetActorLocation() + Chosen;
    GetWorld()->SpawnActor<AActor>(FoodClass, SpawnLocation, FRotator::ZeroRotator);
}

