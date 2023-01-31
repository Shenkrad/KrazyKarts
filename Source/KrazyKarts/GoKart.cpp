// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
// #include "GoKartMovementComponent.h"
#include "DrawDebugHelpers.h" // Remove later. Only testing purposes

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Allow property replication
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
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

void AGoKart::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ServerState);
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

	if (MovementComponent == nullptr) return;

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		UnacknowledgeMoves.Add(Move);
		MovementComponent->SimulateMove(Move);

		Server_SendMove(Move);
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	// If it is the server and in control of the pawn
	if(GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}
	
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

void AGoKart::OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetVelocity(ServerState.Velocity);
	SetActorTransform(ServerState.Transform);
	ClearAcknowledgeMoves(ServerState.LastMove);

	// Simulate unacknowledge moves to be ahead of lag issues
	for (const FGoKartMove& Move : UnacknowledgeMoves)
	{	
		MovementComponent->SimulateMove(Move);
	}
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

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;
	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{	
	return true; //TODO: Real Validation
}


// Updating unacknowledge moves from the server
void AGoKart::ClearAcknowledgeMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgeMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgeMoves = NewMoves;
}

