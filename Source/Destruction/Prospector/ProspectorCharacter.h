// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Voxel/GeoTypes.h"
#include "ProspectorCharacter.generated.h"

class UStaticMeshComponent;
class UPanningMinigameComponent;
class UGoldPanWidget;
class AGravelBarSite;

UENUM(BlueprintType)
enum class EProspectorJobType : uint8
{
	MoveTo,
	Dig,
	Pan
};

// One queued order: where the request said "job" is a wrapper around an existing Request* action -
// a move, a dig at a specific spot, or a pan - so the queue can just replay them in order.
USTRUCT(BlueprintType)
struct FProspectorJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EProspectorJobType Type = EProspectorJobType::MoveTo;

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> TargetActor;
};

// The commandable prospector unit, COH3-style: AI-controlled (AutoPossessAI), never possessed by
// the player directly. AProspectorPlayerController relays right-click move orders and E/F dig/pan
// commands to it via the public Request* methods below. Carries dug sediment as a real
// FSedimentPacket and pans it for gold via UPanningMinigameComponent.
UCLASS()
class DESTRUCTION_API AProspectorCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProspectorCharacter();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Panning")
	TObjectPtr<UPanningMinigameComponent> PanningComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Panning")
	TSubclassOf<UGoldPanWidget> GoldPanWidgetClass;

	// Maximum distance from the character to a dig/target point that digging still works at.
	UPROPERTY(EditAnywhere, Category = "Digging")
	float DigReach = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Digging")
	float DigRadius = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float MaxCarriedKg = 40.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FSedimentPacket CarriedBucket;

	// Lost-to-tailings material from past panning sessions - not reprocessable yet, but tracked so
	// that gameplay can be added later without losing the data.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FSedimentPacket TailingsPile;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	float TotalGoldGrams = 0.0f;

	// Pathfinds to Destination via the navmesh (this unit's AIController).
	void RequestMoveTo(const FVector& Destination);

	// Digs TargetActor (if it's a GravelBarSite within DigReach) at WorldLocation.
	void RequestDigAt(AActor* TargetActor, const FVector& WorldLocation);

	// Starts panning the carried bucket for gold. Returns false (and does nothing) if there's nothing
	// to pan or a pan is already in progress.
	bool RequestPan();

	// Feeds tilt input to the panning minigame while it's active; no-op otherwise.
	void RequestPanTilt(FVector2D TiltValue);

	// Job queue: shift-commands append here instead of acting immediately. Each job runs to
	// completion (move arrival, dig, or pan finishing) before the next one starts.
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	void EnqueueMoveJob(const FVector& Destination);

	UFUNCTION(BlueprintCallable, Category = "Jobs")
	void EnqueueDigJob(AActor* TargetActor, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Jobs")
	void EnqueuePanJob();

	UFUNCTION(BlueprintCallable, Category = "Jobs")
	void ClearJobQueue();

	UFUNCTION(BlueprintPure, Category = "Jobs")
	TArray<FProspectorJob> GetJobQueue() const { return JobQueue; }

private:
	UPROPERTY()
	TObjectPtr<UGoldPanWidget> GoldPanWidgetInstance;

	UPROPERTY()
	TArray<FProspectorJob> JobQueue;

	UFUNCTION()
	void HandlePanningFinished(bool bSuccess, float GoldBankedGrams, FSedimentPacket LostToTailings);

	UFUNCTION()
	void HandlePanProgress(float NormalizedJunkRemaining);

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void ProcessNextJob();
	void ExecuteJob(const FProspectorJob& Job);
};
