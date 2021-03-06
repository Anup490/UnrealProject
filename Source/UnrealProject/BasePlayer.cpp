// Fill out your copyright notice in the Description page of Project Settings.
#include "BasePlayer.h"
#include "UnrealGameInstance.h"
#include "Constants.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "BaseCloud.h"
#include "BaseMenuWidget.h"
#include "BaseScoreWidget.h"
#include "BaseCloudSpawner.h"

// Sets default values
bool ABasePlayer::bShowMenu = true;

ABasePlayer::ABasePlayer()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>("Particle System");
	bIsMenuHidden = true;
	bIsMortal = false;
	bWasFalling = false;
	bIsFirstInput = true;
	iOldScale = 0;
	iScore = 0;
	Rotation = FRotator(0, 0, 0);
	PlayerController = UGameplayStatics::GetPlayerController(this, 0);
}

void ABasePlayer::onStartClick() 
{
	bShowMenu = false;
}

// Called when the game starts or when spawned
void ABasePlayer::BeginPlay()
{
	Super::BeginPlay();
	UUnrealGameInstance::SaveBasePlayer(this);

	FInputModeGameAndUI InputMode = FInputModeGameAndUI();
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	UGameplayStatics::GetPlayerController(this, 0)->SetInputMode(InputMode);

	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ABasePlayer::onOverlapBegin);
}

// Called every frame
void ABasePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GlowFireOnJump();
	AttachFireToMuffin();
	if ((GetVelocity().Z) < 0) 
	{
		bWasFalling = (GetVelocity().Z) < 0;
	}
	ShowUI();
}

// Called to bind functionality to input
void ABasePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveLeftRight", this, &ABasePlayer::MoveLeftRight);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABasePlayer::Jump);
}

void ABasePlayer::MoveLeftRight(float scale)
{
	if (bEnableControl) 
	{
		int iScale = (int)scale;
		if (iScale != 0) 
		{
			RotatePlayer(iScale);
			FVector MovementVector(0, MOVEMENT_MULTIPLIER, 0);
			AddMovementInput(MovementVector, scale, false);
		}
	}
}

void ABasePlayer::Jump() 
{
	if (bEnableControl) 
	{
		FVector JumpVector(0, 0, JUMP_MULTIPLIER);
		LaunchCharacter(JumpVector, false, true);
		ParticleSystem->Activate();
		bWasFalling = false;
	}
}

void ABasePlayer::RegisterCallbackAndShowUI(class UBaseMenuWidget* Menu)
{
	if (Menu)
	{
		Menu->SetCallback(ABasePlayer::onStartClick);
		ShowUI();
	}
}

void ABasePlayer::ShowUI() 
{
	if (bShowMenu && bIsMenuHidden)
	{
		EnableAndShowMuffin(false);
		ShowMenu(true);
		bIsMenuHidden = false;
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = true;
		}
		ShowScoreUI(false);
	}
	else if ((!bShowMenu) && (!bIsMenuHidden))
	{
		ShowMenu(false);
		EnableAndShowMuffin(true);
		bIsMenuHidden = true;
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = false;
		}
		ShowScoreUI(true);
	}
}

void ABasePlayer::ExplodeMuffin(UParticleSystem* ParticleTemplate) 
{
	float ZVelocity = GetVelocity().Z;
	if (bIsMortal && bWasFalling && (ZVelocity == 0)) 
	{
		UGameplayStatics::SpawnEmitterAtLocation
		(
			GetWorld(), 
			ParticleTemplate,
			GetActorLocation(), 
			FRotator(0,0,0), 
			FVector(1,1,1), 
			true, 
			EPSCPoolMethod::None,
			true
		);
		bShowMenu = true;
		iScore = 0;
		ResetScore();
		bIsMortal = false;
		ResetCloudSpawner();
	}
}

void ABasePlayer::ResetCloudSpawner() 
{
	TArray<AActor*> CloudSpawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCloudSpawner::StaticClass(), CloudSpawners);
	for (AActor* Actor : CloudSpawners) 
	{
		ABaseCloudSpawner* CloudSpawner = Cast<ABaseCloudSpawner>(Actor);
		if (CloudSpawner) 
		{
			CloudSpawner->OnReset();
		}
	}
}

void ABasePlayer::onOverlapBegin
(
	class UPrimitiveComponent* OverlappedComp,
	class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
) 
{
	if (Cast<ABaseCloud>(OtherActor)) 
	{
		bIsMortal = true;
		SetScore(++iScore);
	}
}

void ABasePlayer::GlowFireOnJump() 
{
	if (GetVelocity().Z > 0) 
	{
		ParticleSystem->Activate();
	}
	else 
	{
		ParticleSystem->Deactivate();
	}
}

void ABasePlayer::AttachFireToMuffin() 
{
	FVector CapsuleLocation = GetCapsuleComponent()->GetComponentLocation();
	CapsuleLocation.Z = CapsuleLocation.Z - 20;
	ParticleSystem->SetWorldLocation(CapsuleLocation);
}

void ABasePlayer::RotatePlayer(int iScale) 
{
	if (iOldScale != iScale) 
	{
		iOldScale = iScale;
		int iRotationDegree = 180;
		if (bIsFirstInput) 
		{
			iRotationDegree = (iScale > 0) ? 360 : 180;
			bIsFirstInput = false;
		}
		PlayerController->SetControlRotation(AddRotation(FRotator(0, iRotationDegree * iScale, 0)));
	}
}

FRotator ABasePlayer::AddRotation(FRotator&& RotationOffset) 
{
	Rotation = Rotation + RotationOffset;
	return Rotation;
}

void ABasePlayer::EnableAndShowMuffin(bool showAndActivate) 
{
	bEnableControl = showAndActivate;
	SetActorHiddenInGame(!showAndActivate);
}