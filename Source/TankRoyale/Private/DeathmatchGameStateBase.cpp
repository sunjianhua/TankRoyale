// Copyright Blurr Development 2018.

#include "DeathmatchGameStateBase.h"
#include "Tank.h"
#include "TankAIController.h"
#include "TankPlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TankSpawnPoint.h"
#include "GameModeDeathmatch.h"

ADeathmatchGameStateBase::ADeathmatchGameStateBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADeathmatchGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	// TODO Pause game
	// there is no reason to empty an empty array
	//TeamOneTanks.Empty();
	//TeamTwoTanks.Empty();
	//TeamSpectatorTanks.Empty();
	WaitForPlayers();
}

void ADeathmatchGameStateBase::WaitForPlayers()
{
	if (!bGameStarted)
	{
		bool bTeamOneReady = false;
		bool bTeamTwoReady = false;

		if (TeamOneTanks.Num() >= TanksPerTeam) bTeamOneReady = true;
		if (TeamTwoTanks.Num() >= TanksPerTeam) bTeamTwoReady = true;

		if (bTeamOneReady && bTeamTwoReady)
		{
			// TODO Countdown from 5
			UE_LOG(LogTemp, Warning, TEXT("Starting game..."));
			StartGame();
		}
		else
		{
			// TODO UI Displaying game loading
			UE_LOG(LogTemp, Warning, TEXT("Team One Players: %d/%d | Team Two Players: %d/%d"), TeamOneTanks.Num(), TanksPerTeam, TeamTwoTanks.Num(), TanksPerTeam);

			FTimerHandle WaitTimer;
			GetWorld()->GetTimerManager().SetTimer(WaitTimer, this, &ADeathmatchGameStateBase::WaitForPlayers, 1.0f, false);
		}
	}
}

void ADeathmatchGameStateBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bGameStarted) return;

	TeamOneScore = TeamOneKills - TeamOneDeaths;
	TeamTwoScore = TeamTwoKills - TeamTwoDeaths;

	if (TeamOneScore == TeamTwoScore)
	{
		WinningTeam = 0;
	}
	else if (TeamOneScore > TeamTwoScore)
	{
		WinningTeam = 1;
	}
	else if (TeamOneScore < TeamTwoScore)
	{
		WinningTeam = 2;
	}

	if (TeamOneTanks.Num() <= 0 || TeamTwoTanks.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No team"));
		EndGame();
	}
	//UE_LOG(LogTemp, Warning, TEXT("Winning Team: %i, Team One Score: %i, Team Two Score: %i"), WinningTeam, TeamOneScore, TeamTwoScore);
}

void ADeathmatchGameStateBase::AssignTankTeam(ATank* Tank)
{
	auto Controller = Tank->GetController();

	//if (Cast<ATankAIController>(Controller) && TeamTwoTanks.Num() < TanksPerTeam)
	//{
	//	TeamTwoTanks.Add(Tank);
	//	return;
	//}
	if (!Tank->AsAssignedToTeam)
	{
		if (TeamOneTanks.Num() > TeamTwoTanks.Num() && TeamTwoTanks.Num() < TanksPerTeam)
		{
			AssignTankToTeamByN(2, Tank);
			return;

		}
		else if (TeamOneTanks.Num() < TanksPerTeam)
		{
			AssignTankToTeamByN(1, Tank);
			return;
		}
		else
		{
			TeamSpectatorTanks.Add(Tank);
			if (Cast<ATankPlayerController>(Controller)) Cast<ATankPlayerController>(Controller)->StartSpectatingOnly();
			Tank->DetachFromControllerPendingDestroy(); // TODO Might cause issues when too many players.
			Tank->DestroyConstructedComponents();
			Tank->Destroy();
			return;
		}
	}
}

void ADeathmatchGameStateBase::StartGame()
{
	/*
	for (ATank * TeamOneTank : TeamOneTanks)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeamOneTanks"));
		TeamOneTank->TankDeath(TeamOneTank, 20);
	}
	for (ATank * TeamTwoTank : TeamTwoTanks)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeamTwoTanks"));
		TeamTwoTank->TankDeath(TeamTwoTank, 20);
	}
	*/
	for (ATank * TeamOneTank : TeamOneTanks)
	{
		TeamOneTank->StartGame();
	}
	for (ATank * TeamTwoTank : TeamTwoTanks)
	{
		TeamTwoTank->StartGame();
	}
		GetWorldTimerManager().SetTimer(GameTimerHandler, this, &ADeathmatchGameStateBase::EndGame, 2.0f, true, 60.0f * GameTime);
		TeamOneScore = 0;
		TeamTwoScore = 0;
		TeamOneKills = 0;
		TeamTwoKills = 0;
		TeamOneDeaths = 0;
		TeamTwoDeaths = 0;
	bGameStarted = true;

	
	}

void ADeathmatchGameStateBase::AddTeamDeath(ATank* Tank, ATank* KillerTank)
{
	if (!bGameStarted) return;

	if (TeamOneTanks.Find(Tank) != INDEX_NONE)
	{
		TeamOneDeaths++;
		//UE_LOG(LogTemp, Warning, TEXT("contrller %s spawn location %s"), *Tank->GetController()->GetName(), *Tank->SpawnPointLocation.ToString());
		TeamOneTanks.Remove(Tank);
		return;
	}

	else if (TeamTwoTanks.Find(Tank) != INDEX_NONE)
	{
		TeamTwoDeaths++;
	//	UE_LOG(LogTemp, Warning, TEXT("contrller %s  spawn location %s"), *Tank->GetController()->GetName(), *Tank->SpawnPointLocation.ToString());
		TeamTwoTanks.Remove(Tank);
		return;
	}
}

void ADeathmatchGameStateBase::EndGame()
{
	if (!bGameStarted) return;
	UE_LOG(LogTemp, Warning, TEXT("EndGame"));
	bGameStarted = false;
	GetWorldTimerManager().ClearAllTimersForObject(this);
	GetWorld()->GetFirstPlayerController()->SetPause(true);
	this->DisplayEndGameUI();
}

int32 ADeathmatchGameStateBase::GetTankTeam(ATank* Tank) const
{
	if (!ensure(Tank)) return 0;

	if (TeamOneTanks.Find(Tank) != INDEX_NONE) return 1;
	else if (TeamTwoTanks.Find(Tank) != INDEX_NONE) return 2;
	else return 0;
}

int32 ADeathmatchGameStateBase::GetTeamAlive(int32 Team) const
{
	if (Team == 1)
	{
		return TeamOneTanks.Num();
	}
	else if (Team == 2)
	{
		return TeamTwoTanks.Num();
	}

	return 0;
}




// Receive hits from tanks and check them against tanks
void ADeathmatchGameStateBase::RegisterTankHit(ATank* ShootingTank, ATank* HitTank)
{
	if (!ensure(ShootingTank)) return;
	if (!ensure(HitTank)) return;
	//if (!(TeamOneTanks.Find(HitTank) != INDEX_NONE) || !(TeamTwoTanks.Find(HitTank) != INDEX_NONE)) return;

	bool bTeamkill = false;
	FString Killer = ShootingTank->GetName();
	FString State = "Unset";
	FString Victim = HitTank->GetName();

	FColor FeedColour;
	if ((TeamOneTanks.Find(ShootingTank) != INDEX_NONE && TeamOneTanks.Find(HitTank) != INDEX_NONE) || (TeamTwoTanks.Find(ShootingTank) != INDEX_NONE && TeamTwoTanks.Find(HitTank) != INDEX_NONE))
	{
		FeedColour = FColor::Red;
		bTeamkill = true;
	}
	else
	{
		FeedColour = FColor::Blue;
		bTeamkill = false;
	}

	if (HitTank->GetHealthPercent() <= 0)
	{
		if (bTeamkill == false)
		{
			if (TeamOneTanks.Find(ShootingTank) != INDEX_NONE) TeamOneKills++;
			if (TeamTwoTanks.Find(ShootingTank) != INDEX_NONE) TeamTwoKills++;
			State = "Killed";
		}
		else State = "Team Killed";
	}
	else State = "Hit";

	this->AddKillToFeed(Killer, State, Victim);

	return;
}

TArray<ATank*> ADeathmatchGameStateBase::GetTeamTanks(int32 Team) const
{
	if (Team == 1) return TeamOneTanks;
	else return TeamTwoTanks;
}


AActor * ADeathmatchGameStateBase::Spawn(AController * NewPlayer, int16 SpawnPointNumber)
{
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SpawnPoint"), TankSpawnPoints);
	SpawnPoint = (Cast<ATankSpawnPoint>(TankSpawnPoints[0]));
	UE_LOG(LogTemp, Warning, TEXT("foued sp %s"), *SpawnPoint->GetName());
	if (SpawnPointNumber >= 5) {
		AActor * point = SpawnPoint->SapwnPoints[1];
		ATank * Tank = GetWorld()->SpawnActor<ATank>(TankSubClass, point->GetActorLocation(), FRotator(0, 0, 0));
		Tank->SetSpawnPointLocation(point->GetActorLocation());
		NewPlayer->Possess(Tank);
		return  Tank;
	}
	else if (SpawnPointNumber < 5)
	{
		AActor * point = SpawnPoint->SapwnPoints[0];
		ATank * Tank = GetWorld()->SpawnActor<ATank>(TankSubClass, point->GetActorLocation(), FRotator(0, 0, 0));
		Tank->SetSpawnPointLocation(point->GetActorLocation());
		NewPlayer->Possess(Tank);
		return  Tank;

	}
	return  nullptr;
}


void ADeathmatchGameStateBase::AssignTankToTeamByN(int16 TeamN, ATank * Tank)
{
	if (TeamN == 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Added to team 2"));
		TeamTwoTanks.Add(Tank);
		UE_LOG(LogTemp, Warning, TEXT("Added to team 2   %d"), TeamTwoTanks.Num());
		Tank->Tags.Add(Tank->TeamTwoTag);
		Tank->AsAssignedToTeamSeter(true);
	}
	else if (TeamN == 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Added to team 1"));
		TeamOneTanks.Add(Tank);
		Tank->Tags.Add(Tank->TeamOneTag);
		Tank->AsAssignedToTeamSeter(true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("The users team could not be assigned!"));
	}
}

AActor * ADeathmatchGameStateBase::SpawnAi(int16 SpawnPointNumber)
{
	if (SpawnPointNumber < 5)
	{
		AActor * point = SpawnPoint->SapwnPoints[0];
		ATank * Tank = GetWorld()->SpawnActor<ATank>(TankSubClass, point->GetActorLocation(), FRotator(0, 0, 0));
		Tank->SetSpawnPointLocation(point->GetActorLocation());
		Tank->SpawnDefaultController();
		return  Tank;
	}
	else if (SpawnPointNumber >= 5) {
		AActor * point = SpawnPoint->SapwnPoints[1];
		ATank * Tank = GetWorld()->SpawnActor<ATank>(TankSubClass, point->GetActorLocation(), FRotator(0, 0, 0));
		Tank->SetSpawnPointLocation(point->GetActorLocation());
		Tank->SpawnDefaultController();
		return  Tank;
	}

	
	return  nullptr;
}



void ADeathmatchGameStateBase::Respawn(AController * NewPlayer , FVector SpawnLocation)
{
	
}








