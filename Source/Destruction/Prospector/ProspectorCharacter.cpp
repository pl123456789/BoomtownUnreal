// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProspectorCharacter.h"
#include "PanningMinigameComponent.h"
#include "GoldPanWidget.h"
#include "Voxel/GravelBarSite.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AProspectorCharacter::AProspectorCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	// AI-controlled, commanded via AProspectorPlayerController - never possessed by the player.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Spawned;

	// Placeholder visual until a real character asset is assigned - a plain cylinder roughly
	// matching the capsule so there's something visible to click-command around.
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.9f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshAsset.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CylinderMeshAsset.Object);
	}

	PanningComponent = CreateDefaultSubobject<UPanningMinigameComponent>(TEXT("PanningComponent"));
}

void AProspectorCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (PanningComponent)
	{
		PanningComponent->OnMinigameFinished.AddDynamic(this, &AProspectorCharacter::HandlePanningFinished);
		PanningComponent->OnProgressChanged.AddDynamic(this, &AProspectorCharacter::HandlePanProgress);
	}

	// AutoPossessAI possesses this pawn during PostInitializeComponents, before BeginPlay, so the
	// AIController is already valid here.
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->ReceiveMoveCompleted.AddDynamic(this, &AProspectorCharacter::HandleMoveCompleted);
	}
}

void AProspectorCharacter::RequestMoveTo(const FVector& Destination)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("[InputDiag] RequestMoveTo: %s Controller=%s"),
				*Destination.ToString(), GetController() ? *GetController()->GetClass()->GetName() : TEXT("NULL")));
	}
	DrawDebugSphere(GetWorld(), Destination, 40.0f, 12, FColor::Cyan, false, 5.0f);

	if (PanningComponent && PanningComponent->IsPanningActive())
	{
		return;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), Destination);
}

void AProspectorCharacter::RequestDigAt(AActor* TargetActor, const FVector& WorldLocation)
{
	if (PanningComponent && PanningComponent->IsPanningActive())
	{
		return;
	}

	if (CarriedBucket.TotalMassKg() >= MaxCarriedKg)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Bucket's full - press F to pan it out before digging more."));
		}
		return;
	}

	if (FVector::Dist(WorldLocation, GetActorLocation()) > DigReach)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, TEXT("Too far to dig there - move closer."));
		}
		return;
	}

	if (AGravelBarSite* Site = Cast<AGravelBarSite>(TargetActor))
	{
		const FSedimentPacket Dug = Site->DigSphereAtWorldLocation(WorldLocation, DigRadius);
		CarriedBucket += Dug;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
				FString::Printf(TEXT("Dug %.1f kg (%.2f g gold). Carrying %.1f / %.1f kg, %.2f g gold. (F to pan)"),
					Dug.TotalMassKg(), Dug.TotalGoldGrams(), CarriedBucket.TotalMassKg(), MaxCarriedKg, CarriedBucket.TotalGoldGrams()));
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, TEXT("Nothing to dig here - point at the gravel bar near the river."));
	}
}

bool AProspectorCharacter::RequestPan()
{
	if (!PanningComponent || PanningComponent->IsPanningActive())
	{
		return false;
	}

	if (CarriedBucket.TotalMassKg() <= 0.0f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Nothing to pan - dig up some sediment first."));
		}
		return false;
	}

	if (GoldPanWidgetClass && !GoldPanWidgetInstance)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			GoldPanWidgetInstance = CreateWidget<UGoldPanWidget>(PC, GoldPanWidgetClass);
		}
	}

	if (GoldPanWidgetInstance)
	{
		GoldPanWidgetInstance->AddToViewport();
	}

	PanningComponent->StartMinigame(CarriedBucket);
	CarriedBucket = FSedimentPacket();
	return true;
}

void AProspectorCharacter::RequestPanTilt(FVector2D TiltValue)
{
	if (PanningComponent && PanningComponent->IsPanningActive())
	{
		PanningComponent->SetTiltInput(TiltValue);
	}
}

void AProspectorCharacter::HandlePanningFinished(bool bSuccess, float GoldBankedGrams, FSedimentPacket LostToTailings)
{
	TotalGoldGrams += GoldBankedGrams;
	TailingsPile += LostToTailings;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, bSuccess ? FColor::Green : FColor::Orange,
			FString::Printf(TEXT("Panning %s! Recovered %.2f g gold. Total: %.2f g. (%.2f g lost to tailings)"),
				bSuccess ? TEXT("complete") : TEXT("cut short"), GoldBankedGrams, TotalGoldGrams, LostToTailings.TotalGoldGrams()));
	}

	if (GoldPanWidgetInstance)
	{
		GoldPanWidgetInstance->HandleMinigameFinished(bSuccess, GoldBankedGrams);
		GoldPanWidgetInstance->RemoveFromParent();
	}

	if (JobQueue.Num() > 0 && JobQueue[0].Type == EProspectorJobType::Pan)
	{
		JobQueue.RemoveAt(0);
		ProcessNextJob();
	}
}

void AProspectorCharacter::EnqueueMoveJob(const FVector& Destination)
{
	FProspectorJob Job;
	Job.Type = EProspectorJobType::MoveTo;
	Job.Location = Destination;
	JobQueue.Add(Job);

	if (JobQueue.Num() == 1)
	{
		ProcessNextJob();
	}
}

void AProspectorCharacter::EnqueueDigJob(AActor* TargetActor, const FVector& WorldLocation)
{
	FProspectorJob Job;
	Job.Type = EProspectorJobType::Dig;
	Job.Location = WorldLocation;
	Job.TargetActor = TargetActor;
	JobQueue.Add(Job);

	if (JobQueue.Num() == 1)
	{
		ProcessNextJob();
	}
}

void AProspectorCharacter::EnqueuePanJob()
{
	FProspectorJob Job;
	Job.Type = EProspectorJobType::Pan;
	JobQueue.Add(Job);

	if (JobQueue.Num() == 1)
	{
		ProcessNextJob();
	}
}

void AProspectorCharacter::ClearJobQueue()
{
	JobQueue.Empty();
}

void AProspectorCharacter::ProcessNextJob()
{
	if (JobQueue.Num() == 0)
	{
		return;
	}

	ExecuteJob(JobQueue[0]);
}

void AProspectorCharacter::ExecuteJob(const FProspectorJob& Job)
{
	switch (Job.Type)
	{
	case EProspectorJobType::MoveTo:
	case EProspectorJobType::Dig:
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), Job.Location);
		break;

	case EProspectorJobType::Pan:
		if (!RequestPan())
		{
			// Nothing to pan, or already panning from an unrelated call - skip straight to the next job
			// rather than waiting forever for a finish event that will never come.
			JobQueue.RemoveAt(0);
			ProcessNextJob();
		}
		break;
	}
}

void AProspectorCharacter::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (JobQueue.Num() == 0)
	{
		return;
	}

	const FProspectorJob CompletedJob = JobQueue[0];
	if (CompletedJob.Type == EProspectorJobType::Dig)
	{
		RequestDigAt(CompletedJob.TargetActor.Get(), CompletedJob.Location);
	}

	if (CompletedJob.Type != EProspectorJobType::Pan)
	{
		JobQueue.RemoveAt(0);
		ProcessNextJob();
	}
}

void AProspectorCharacter::HandlePanProgress(float NormalizedJunkRemaining)
{
	if (GoldPanWidgetInstance)
	{
		GoldPanWidgetInstance->HandleProgressChanged(NormalizedJunkRemaining);
	}
}
