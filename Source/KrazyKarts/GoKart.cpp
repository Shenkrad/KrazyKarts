// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "DrawDebugHelpers.h" // Remove later. Only testing purposes

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Allow property replication
	bReplicates = true;
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

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		UnacknowledgeMoves.Add(Move);
		SimulateMove(Move);

		Server_SendMove(Move);
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}

	// If it is the server and in control of the pawn
	if(GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = CreateMove(DeltaTime);
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
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgeMoves(ServerState.LastMove);

	// Simulate unacknowledge moves to be ahead of lag issues
	for (const FGoKartMove& Move : UnacknowledgeMoves)
	{	
		SimulateMove(Move);
	}
}

void AGoKart::MoveForward(float Value)
{	
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{	
	return true; //TODO: Real Validation
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100;
	
	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void AGoKart::ApplyRotation(float DeltaTime, float InSteeringThrow)
{	
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * InSteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	// Rotate Velocity vector with RotationDelta
	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
}

void AGoKart::SimulateMove(const FGoKartMove Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;	
	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	Move.SteeringThrow = SteeringThrow;

	return Move;
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

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{	
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}