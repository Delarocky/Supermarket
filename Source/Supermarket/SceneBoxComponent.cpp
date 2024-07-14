#include "SceneBoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"

USceneBoxComponent::USceneBoxComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionProfileName(TEXT("OverlapAll"));
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

bool USceneBoxComponent::CheckOverlap() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<AActor*> OverlappingActors;

    bool bHit = UKismetSystemLibrary::BoxOverlapActors(
        World,
        GetComponentLocation(),
        GetScaledBoxExtent(),
        TArray<TEnumAsByte<EObjectTypeQuery>>(),
        AActor::StaticClass(),
        ActorsToIgnore,
        OverlappingActors
    );

    for (AActor* OverlappingActor : OverlappingActors)
    {
        if (OverlappingActor != GetOwner() && OverlappingActor->FindComponentByClass<USceneBoxComponent>())
        {
            UE_LOG(LogTemp, Warning, TEXT("Overlap detected with: %s"), *OverlappingActor->GetName());
            return true;
        }
    }

    return false;
}