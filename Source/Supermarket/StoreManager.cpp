// StoreManager.cpp
#include "StoreManager.h"
#include "Kismet/GameplayStatics.h"

AStoreManager* AStoreManager::Instance = nullptr;

AStoreManager::AStoreManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bStoreOpen = false;  // Start with the store closed
}

void AStoreManager::SetStoreOpen(bool bIsOpen)
{
    FScopeLock Lock(&StatusLock);
    if (bStoreOpen != bIsOpen)
    {
        bStoreOpen = bIsOpen;
        OnStoreStatusChanged.Broadcast(bStoreOpen);
    }
}

AStoreManager* AStoreManager::GetInstance(UWorld* World)
{
    if (World)
    {
        AStoreManager* StoreManager = Cast<AStoreManager>(UGameplayStatics::GetActorOfClass(World, AStoreManager::StaticClass()));
        if (!StoreManager)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            StoreManager = World->SpawnActor<AStoreManager>(AStoreManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        }
        return StoreManager;
    }
    return nullptr;
}
