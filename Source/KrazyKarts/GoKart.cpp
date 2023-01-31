// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Components/SkeletalMeshComponent.h"
#include "GoKartMovementReplicator.h"
#include "DrawDebugHelpers.h" // Remove later. Only testing purposes

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Allow property replication
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
	MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>(TEXT("MovementReplicator"));

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	// Lowering NetUpdateFrequency for testing purposes
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}

}

// Showing Role type only for testing
FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{	
	Super::Tick(DeltaTime);
	
	// Showing Role type only for testing
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(float Value)
{	
	if (MovementComponent == nullptr) return;
	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(float Value)
{
	if (MovementComponent == nullptr) return;
	MovementComponent->SetSteeringThrow(Value);
}

