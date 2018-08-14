// Copyright Blurr Development 2018.

#include "Tank.h"
#include "TankTrack.h"
#include "Engine/World.h"
#include "GameModes/GameModeDeathmatch.h"
#include "Kismet/GameplayStatics.h"
#include "AmmoPickup.h"
#include "TankAimingComponent.h"


// Sets default values
ATank::ATank()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ATank::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = StartingHealth;

	if (Cast<AGameModeDeathmatch>(GetWorld()->GetAuthGameMode())) GameMode = EGameMode::Deathmatch;

	if (GameMode == EGameMode::Deathmatch)
	{
		Cast<AGameModeDeathmatch>(GetWorld()->GetAuthGameMode())->AssignTankTeam(this);
	}
}


float ATank::TakeDamage(float DamageAmount, struct FDamageEvent const &DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	int32 DamagePoints = FPlatformMath::RoundToInt(DamageAmount);
	int32 DamageToApply = FMath::Clamp<int32>(DamagePoints, 0, CurrentHealth);

	CurrentHealth -= DamageToApply;

	if (CurrentHealth <= 0 && bDead == false)
	{
		TankDeath(DamageCauser, DamageToApply);
	}

	return DamageToApply;
}

float ATank::GetHealthPercent() const
{
	return ((float)CurrentHealth / (float)StartingHealth);
}

void ATank::SetOnPickup(bool On, AAmmoPickup* Pickup)
{
	bOnPickup = On;
	CurrentPickup = Pickup;
}

void ATank::TankDeath(AActor* DamageCauser, int32 DamageToApply)
{
	bDead = true;
	OnDeath.Broadcast();

	if (GameMode == EGameMode::Deathmatch)
	{
		auto KillerTank = Cast<ATank>(DamageCauser);
		Cast<AGameModeDeathmatch>(GetWorld()->GetAuthGameMode())->AddTeamDeath(this, KillerTank);

		DropRemainingAmmo();
	}

	// Play explosion sound
	if (!ensure(ExplodeSound)) return;
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation(), ExplodeVolumeMultiplier, ExplodePitchMultiplier, ExplodeStartTime);

	// Play the particle emitter
	if (!ensure(EmitterTemplate)) return;
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EmitterTemplate, GetTransform());
	
	// TODO Destroy the wheels

	// Destroy the actor
	Destroy();

	return;
}

void ATank::DropRemainingAmmo()
{
	if (!ensure(AmmoPickupBlueprint)) return;
	FVector Location = GetActorLocation();
	auto SpawnedPickup = GetWorld()->SpawnActor<AAmmoPickup>(AmmoPickupBlueprint);
	SpawnedPickup->SetActorLocation(Location);

	auto AimingComponent = FindComponentByClass<UTankAimingComponent>();
	if (!ensure(AimingComponent)) return;
	int32 Ammo = AimingComponent->GetRoundsLeft();
	SpawnedPickup->SetupPickup(Ammo);
}

void ATank::DropHalfAmmo()
{
	if (!ensure(AmmoPickupBlueprint)) return;
	FVector Location = GetActorLocation();
	auto SpawnedPickup = GetWorld()->SpawnActor<AAmmoPickup>(AmmoPickupBlueprint);
	SpawnedPickup->SetActorLocation(Location);

	auto AimingComponent = FindComponentByClass<UTankAimingComponent>();
	if (!ensure(AimingComponent)) return;
	int32 Ammo = (AimingComponent->GetRoundsLeft())/2;
	SpawnedPickup->SetupPickup(Ammo);
}

void ATank::UsePickup()
{
	// Get the aiming component and then add the ammo from the pickup
	auto AimingComponent = FindComponentByClass<UTankAimingComponent>();
	if (!ensure(AimingComponent)) return;

	int32 AmountToPickup = CurrentPickup->GetStoredAmmo();
	int32 AmountToLeave = 0;

	// Make sure the tank isn't already full of ammo
	if (AimingComponent->GetRoundsLeft() >= AimingComponent->GetMaxRounds()) return;

	// Check how much the tank can take and set AmountToLeave to what is left
	if ((AimingComponent->GetMaxRounds() - AimingComponent->GetRoundsLeft()) < AmountToPickup)
	{
		AmountToLeave = AmountToPickup;
		AmountToPickup = (AimingComponent->GetMaxRounds() - AimingComponent->GetRoundsLeft());
		AmountToLeave = AmountToLeave - AmountToPickup;
	}

	// Add the ammo to the tank
	AimingComponent->AddAmmo(AmountToPickup);

	// Destroy the pickup
	CurrentPickup->Deactivate();

	// Spawn new pickup with what is left
	FVector Location = GetActorLocation();
	auto SpawnedPickup = GetWorld()->SpawnActor<AAmmoPickup>(AmmoPickupBlueprint);
	SpawnedPickup->SetActorLocation(Location);
	SpawnedPickup->SetupPickup(AmountToLeave);
}