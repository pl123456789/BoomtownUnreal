// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProspectorPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AProspectorCharacter;
struct FInputActionValue;

// Owns all Enhanced Input for the COH3-style scheme: WASD pans the independently-possessed
// ARTSCameraPawn, the mouse wheel zooms it, right-click issues a move order to the AI-controlled
// AProspectorCharacter unit, and E/F relay dig/pan commands to it. The character is never possessed
// by this controller - it's found by class each time a command needs to reach it.
UCLASS()
class DESTRUCTION_API AProspectorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AProspectorPlayerController();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraPanAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraZoomAction;

	// Right-click: move the Prospector to the clicked ground point.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction;

	// E: dig whatever is under the cursor, if within the Prospector's reach.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DigCommandAction;

	// F: pan the Prospector's carried bucket for gold.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PanCommandAction;

	// Mouse movement, read continuously while the Prospector's panning minigame is active.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PanTiltAction;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// Constructs the actions above and a WASD+mouse mapping context in code for any left
	// unassigned, so the controls work without needing InputAction/InputMappingContext assets
	// created via the editor.
	void EnsureDefaultInputAssets();

	void DoCameraPan(const FInputActionValue& Value);
	void DoCameraZoom(const FInputActionValue& Value);
	void DoMoveCommand(const FInputActionValue& Value);
	void DoDigCommand(const FInputActionValue& Value);
	void DoPanCommand(const FInputActionValue& Value);
	void DoPanTilt(const FInputActionValue& Value);

	bool GetCursorGroundHit(FHitResult& OutHit) const;
	AProspectorCharacter* GetProspector() const;

	// True while either Shift key is held - queues a command onto the Prospector's job queue
	// instead of running it immediately.
	bool IsShiftHeld() const;
};
