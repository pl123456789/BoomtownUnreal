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
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

	// Selection indicator - a flat disc beneath the unit, hidden until selected. Reuses the same
	// engine cylinder mesh as the placeholder body, just squashed flat.
	SelectionRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRingMesh"));
	SelectionRingMesh->SetupAttachment(RootComponent);
	SelectionRingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
	SelectionRingMesh->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.02f));
	SelectionRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRingMesh->SetVisibility(false);
	if (CylinderMeshAsset.Succeeded())
	{
		SelectionRingMesh->SetStaticMesh(CylinderMeshAsset.Object);
	}

	PanningComponent = CreateDefaultSubobject<UPanningMinigameComponent>(TEXT("PanningComponent"));
}

void AProspectorCharacter::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	if (SelectionRingMesh)
	{
		SelectionRingMesh->SetVisibility(bSelected);
	}
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
	// Replace the queue and start moving through it. ClearJobQueue empties any pending waypoints, then
	// EnqueueMoveJob starts this one as the (tracked) head - so a following shift+right-click appends
	// rather than interrupting, which a direct MoveToLocation call could not support.
	ClearJobQueue();
	EnqueueMoveJob(Destination);
}

EPathFollowingRequestResult::Type AProspectorCharacter::IssueMoveToLocation(const FVector& Destination)
{
	if (PanningComponent && PanningComponent->IsPanningActive())
	{
		return EPathFollowingRequestResult::Failed;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return EPathFollowingRequestResult::Failed;
	}

	// Clear the tracked id BEFORE issuing: MoveToLocation aborts any in-flight move synchronously, which
	// fires ReceiveMoveCompleted for the OLD request. With the id cleared, HandleMoveCompleted ignores
	// that stale/aborted completion instead of advancing the queue against the wrong job.
	ActiveMoveRequestID = FAIRequestID::InvalidRequest;

	const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(Destination);
	if (Result == EPathFollowingRequestResult::RequestSuccessful)
	{
		ActiveMoveRequestID = AIController->GetCurrentMoveRequestID();
	}
	else if (Result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("IssueMoveToLocation: MoveToLocation failed for destination %s"), *Destination.ToString());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Can't reach that spot."));
		}
	}
	return Result;
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

void AProspectorCharacter::BeginManualMovement()
{
	if (bManualMovementActive)
	{
		return;
	}

	bManualMovementActive = true;

	// Take over from the AIController exactly once: drop any queued jobs/waypoints, THEN stop the current
	// path-follow request. Order matters - StopMovement aborts the active move and fires HandleMoveCompleted
	// synchronously; clearing the queue first means that callback sees an empty queue and no-ops, instead of
	// advancing to (and issuing a move toward) the next queued waypoint. AddMovementInput then drives the
	// movement component directly; the pawn is never possessed by the player - the AIController stays attached
	// and resumes on the next move order.
	ClearJobQueue();
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void AProspectorCharacter::ApplyManualMovement(const FVector& WorldDirection)
{
	// WASD must never move the unit mid-pan; if a manual state was somehow active, end it safely.
	if (PanningComponent && PanningComponent->IsPanningActive())
	{
		EndManualMovement();
		return;
	}

	if (!bManualMovementActive)
	{
		BeginManualMovement();
	}

	AddMovementInput(WorldDirection, 1.0f);
}

void AProspectorCharacter::EndManualMovement()
{
	// Just clears the manual flag - the character decelerates naturally from CharacterMovement braking,
	// or is immediately redirected if a new AI move order follows. The previously cancelled order is not
	// resumed.
	bManualMovementActive = false;
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
	{
		const bool bIsDig = (Job.Type == EProspectorJobType::Dig);
		AActor* const DigTarget = Job.TargetActor.Get();
		const FVector Destination = Job.Location;

		const EPathFollowingRequestResult::Type Result = IssueMoveToLocation(Destination);
		if (Result != EPathFollowingRequestResult::RequestSuccessful)
		{
			// No async move completion will follow (already at the goal, path failed, or blocked by
			// panning), so advance the queue here. If a dig job was already at its spot, dig now.
			if (bIsDig && Result == EPathFollowingRequestResult::AlreadyAtGoal)
			{
				RequestDigAt(DigTarget, Destination);
			}
			JobQueue.RemoveAt(0);
			ProcessNextJob();
		}
		break;
	}

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

	// Only the current job's own move advances the queue. A completion from a superseded/aborted move
	// (e.g. a plain right-click replaced an in-flight queued move) carries a different request id and is
	// ignored, so it can't consume the wrong job.
	if (!RequestID.IsEquivalent(ActiveMoveRequestID))
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
