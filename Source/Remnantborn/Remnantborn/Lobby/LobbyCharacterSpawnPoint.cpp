#include "LobbyCharacterSpawnPoint.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"

ALobbyCharacterSpawnPoint::ALobbyCharacterSpawnPoint()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
    SpawnDirection->SetupAttachment(SceneRoot);
    SpawnDirection->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnArea->SetupAttachment(SceneRoot);
    SpawnArea->SetBoxExtent(FVector(50.f, 50.f, 100.f));
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PlayerIndex = -1;
    bIsOccupied = false;
    DisplayName = FText::FromString(TEXT("Spawn Point"));

    PrimaryActorTick.bCanEverTick = false;
}

void ALobbyCharacterSpawnPoint::BeginPlay()
{
    Super::BeginPlay();
    UpdateVisualState();
}

FTransform ALobbyCharacterSpawnPoint::GetSpawnTransform() const
{
    return GetActorTransform();
}

void ALobbyCharacterSpawnPoint::SetOccupied(bool bNewOccupied)
{
    if (bIsOccupied != bNewOccupied)
    {
        bIsOccupied = bNewOccupied;
        UpdateVisualState();
    }
}

void ALobbyCharacterSpawnPoint::UpdateVisualState()
{
    if (SpawnArea)
    {
        if (bIsOccupied)
        {
            SpawnArea->ShapeColor = FColor::Red;
        }
        else
        {
            SpawnArea->ShapeColor = FColor::Green;
        }
    }
}

#if WITH_EDITOR
void ALobbyCharacterSpawnPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ALobbyCharacterSpawnPoint, bIsOccupied))
    {
        UpdateVisualState();
    }
}
#endif