// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProspectorPlayerController.h"
#include "ProspectorCharacter.h"
#include "SelectableUnit.h"
#include "RTSCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AProspectorPlayerController::AProspectorPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AProspectorPlayerController::EnsureDefaultInputAssets()
{
	const auto MakeAction = [this](TObjectPtr<UInputAction>& ActionRef, EInputActionValueType ValueType, const TCHAR* Name)
	{
		if (!ActionRef)
		{
			UInputAction* NewAction = NewObject<UInputAction>(this, Name);
			NewAction->ValueType = ValueType;
			ActionRef = NewAction;
		}
	};

	MakeAction(CameraPanAction, EInputActionValueType::Axis2D, TEXT("IA_CameraPan_Runtime"));
	MakeAction(CameraZoomAction, EInputActionValueType::Axis1D, TEXT("IA_CameraZoom_Runtime"));
	MakeAction(MoveCommandAction, EInputActionValueType::Boolean, TEXT("IA_MoveCommand_Runtime"));
	MakeAction(DigCommandAction, EInputActionValueType::Boolean, TEXT("IA_DigCommand_Runtime"));
	MakeAction(PanCommandAction, EInputActionValueType::Boolean, TEXT("IA_PanCommand_Runtime"));
	MakeAction(PanTiltAction, EInputActionValueType::Axis2D, TEXT("IA_PanTilt_Runtime"));
	MakeAction(SelectAction, EInputActionValueType::Boolean, TEXT("IA_SelectCommand_Runtime"));
	MakeAction(DeselectAction, EInputActionValueType::Boolean, TEXT("IA_DeselectCommand_Runtime"));
	MakeAction(TabCycleAction, EInputActionValueType::Boolean, TEXT("IA_TabCycleCommand_Runtime"));
	MakeAction(CenterFollowAction, EInputActionValueType::Boolean, TEXT("IA_CenterFollow_Runtime"));

	if (DefaultMappingContext)
	{
		return;
	}

	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_ProspectorController_Runtime"));

	IMC->MapKey(CameraPanAction, EKeys::W).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
	FEnhancedActionKeyMapping& SMapping = IMC->MapKey(CameraPanAction, EKeys::S);
	SMapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
	SMapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	IMC->MapKey(CameraPanAction, EKeys::D);
	IMC->MapKey(CameraPanAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(this));

	IMC->MapKey(CameraZoomAction, EKeys::MouseWheelAxis);
	IMC->MapKey(MoveCommandAction, EKeys::RightMouseButton);
	IMC->MapKey(DigCommandAction, EKeys::E);
	IMC->MapKey(PanCommandAction, EKeys::F);
	IMC->MapKey(PanTiltAction, EKeys::Mouse2D);
	IMC->MapKey(SelectAction, EKeys::LeftMouseButton);
	IMC->MapKey(DeselectAction, EKeys::Escape);
	IMC->MapKey(TabCycleAction, EKeys::Tab);
	IMC->MapKey(CenterFollowAction, EKeys::C);

	DefaultMappingContext = IMC;
}

void AProspectorPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AProspectorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Not BeginPlay: SetupInputComponent only ever runs from InitInputSystem(), which itself only
	// runs once this controller's LocalPlayer is already assigned - BeginPlay has no such guarantee
	// and GetLocalPlayer() can still be null there, silently dropping the mapping context.
	EnsureDefaultInputAssets();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	UE_LOG(LogTemp, Warning, TEXT("[InputDiag] SetupInputComponent: LocalPlayer=%s Subsystem=%s DefaultMappingContext=%s InputComponentClass=%s EnhancedCast=%s"),
		GetLocalPlayer() ? TEXT("valid") : TEXT("NULL"),
		Subsystem ? TEXT("valid") : TEXT("NULL"),
		DefaultMappingContext ? TEXT("valid") : TEXT("NULL"),
		InputComponent ? *InputComponent->GetClass()->GetName() : TEXT("NULL"),
		EnhancedInputComponent ? TEXT("valid") : TEXT("NULL"));

	if (EnhancedInputComponent)
	{
		if (CameraPanAction)
		{
			EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Triggered, this, &AProspectorPlayerController::DoCameraPan);
			// Completed fires once when every WASD key has been released, so manual character movement
			// ends cleanly (Bill decelerates naturally) instead of leaving a stuck movement state.
			EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Completed, this, &AProspectorPlayerController::DoCameraPanReleased);
		}
		if (CameraZoomAction)
		{
			EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AProspectorPlayerController::DoCameraZoom);
		}
		if (MoveCommandAction)
		{
			EnhancedInputComponent->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoMoveCommand);
		}
		if (DigCommandAction)
		{
			EnhancedInputComponent->BindAction(DigCommandAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoDigCommand);
		}
		if (PanCommandAction)
		{
			EnhancedInputComponent->BindAction(PanCommandAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoPanCommand);
		}
		if (PanTiltAction)
		{
			EnhancedInputComponent->BindAction(PanTiltAction, ETriggerEvent::Triggered, this, &AProspectorPlayerController::DoPanTilt);
		}
		if (SelectAction)
		{
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoSelectCommand);
		}
		if (DeselectAction)
		{
			EnhancedInputComponent->BindAction(DeselectAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoDeselectCommand);
		}
		if (TabCycleAction)
		{
			// Started (not Triggered) so holding Tab down doesn't cycle every frame - one keypress
			// advances exactly one unit.
			EnhancedInputComponent->BindAction(TabCycleAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoTabCycleCommand);
		}
		if (CenterFollowAction)
		{
			// Started so one keypress toggles once; holding C must not repeatedly toggle.
			EnhancedInputComponent->BindAction(CenterFollowAction, ETriggerEvent::Started, this, &AProspectorPlayerController::DoCenterFollowCommand);
		}
	}
}

bool AProspectorPlayerController::GetCursorGroundHit(FHitResult& OutHit) const
{
	return GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_WorldStatic), false, OutHit) && OutHit.bBlockingHit;
}

bool AProspectorPlayerController::IsShiftHeld() const
{
	return IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
}

bool AProspectorPlayerController::IsAltHeld() const
{
	return IsInputKeyDown(EKeys::LeftAlt);
}

void AProspectorPlayerController::SelectUnit(AActor* Unit)
{
	ISelectableUnit* Selectable = Cast<ISelectableUnit>(Unit);
	if (!Selectable)
	{
		return;
	}

	if (SelectedUnit.Get() == Unit)
	{
		return;
	}

	// Capture follow state before DeselectCurrentUnit stops it, so an actual selection switch (Tab or
	// clicking another unit) transfers follow to the new unit instead of just losing it.
	ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn());
	const bool bWasFollowing = Cam && Cam->IsFollowingActor();

	DeselectCurrentUnit();

	SelectedUnit = Unit;
	Selectable->SetSelected(true);

	if (bWasFollowing && Cam)
	{
		Cam->BeginFollowingActor(Unit);
	}
}

void AProspectorPlayerController::DeselectCurrentUnit()
{
	if (AActor* Previous = SelectedUnit.Get())
	{
		// End any manual WASD movement on the unit being deselected so it can't keep receiving input.
		if (AProspectorCharacter* Prospector = Cast<AProspectorCharacter>(Previous))
		{
			Prospector->EndManualMovement();
		}
		if (ISelectableUnit* Selectable = Cast<ISelectableUnit>(Previous))
		{
			Selectable->SetSelected(false);
		}
	}
	SelectedUnit = nullptr;

	// Clearing the selection always stops follow - SelectUnit re-enables it afterward if this was an
	// actual switch rather than a real deselect. Safe to call even when not currently following.
	if (ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn()))
	{
		Cam->StopFollowingActor();
	}
}

void AProspectorPlayerController::DoCameraPan(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn());

	// Left Alt held: WASD pans the RTS camera (unchanged M0004 behaviour). Any active manual character
	// movement ends here so releasing Alt->character / pressing Alt->camera transitions cleanly.
	if (IsAltHeld())
	{
		if (AProspectorCharacter* Selected = Cast<AProspectorCharacter>(SelectedUnit.Get()))
		{
			Selected->EndManualMovement();
		}
		if (Cam)
		{
			// Manual camera control always wins over follow - stop it (idempotent) before panning, and
			// leave the camera free until C is pressed again.
			Cam->StopFollowingActor();
			Cam->PanBy(Input, GetWorld()->GetDeltaSeconds());
		}
		return;
	}

	// Alt not held: WASD drives the selected character, camera-relative. Nothing selected -> do nothing
	// (the camera stays stationary).
	AProspectorCharacter* Selected = Cast<AProspectorCharacter>(SelectedUnit.Get());
	if (!Selected)
	{
		return;
	}

	const float ViewYaw = Cam ? Cam->GetViewYaw() : 0.0f;
	const FRotator YawRotation(0.0f, ViewYaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Input.Y = forward/back, Input.X = right/left (same convention as camera pan). Normalize so a
	// diagonal (two keys) isn't faster than a single axis. Forward/Right are already horizontal, so
	// camera pitch never affects the direction.
	const FVector WorldDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal();
	Selected->ApplyManualMovement(WorldDirection);
}

void AProspectorPlayerController::DoCameraPanReleased(const FInputActionValue& Value)
{
	if (AProspectorCharacter* Selected = Cast<AProspectorCharacter>(SelectedUnit.Get()))
	{
		Selected->EndManualMovement();
	}
}

void AProspectorPlayerController::DoCameraZoom(const FInputActionValue& Value)
{
	if (ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn()))
	{
		Cam->ZoomBy(Value.Get<float>());
	}
}

void AProspectorPlayerController::DoMoveCommand(const FInputActionValue& Value)
{
	// Right-click only issues a move order when a unit is actually selected.
	AProspectorCharacter* Prospector = Cast<AProspectorCharacter>(SelectedUnit.Get());
	if (!Prospector)
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = GetCursorGroundHit(Hit);

	if (bHit)
	{
		// A move order always ends manual WASD control first, so the unit returns immediately to normal
		// NavMesh pathfinding rather than fighting leftover manual input.
		Prospector->EndManualMovement();

		if (IsShiftHeld())
		{
			// Append a waypoint to the queue (starts moving if the unit was idle).
			Prospector->EnqueueMoveJob(Hit.Location);
		}
		else
		{
			// Replace the queue and move now (RequestMoveTo clears the queue internally).
			Prospector->RequestMoveTo(Hit.Location);
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Move command fired but no cursor hit."));
	}
}

void AProspectorPlayerController::DoDigCommand(const FInputActionValue& Value)
{
	// E only ever acts on the selected unit - no world search, no implicit fallback to another
	// Prospector. A non-Prospector or no selection at all is a safe no-op.
	AProspectorCharacter* Prospector = Cast<AProspectorCharacter>(SelectedUnit.Get());
	if (!Prospector)
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = GetCursorGroundHit(Hit);

	if (bHit)
	{
		if (IsShiftHeld())
		{
			Prospector->EnqueueDigJob(Hit.GetActor(), Hit.Location);
		}
		else
		{
			Prospector->ClearJobQueue();
			Prospector->RequestDigAt(Hit.GetActor(), Hit.Location);
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Dig key fired but no cursor hit."));
	}
}

void AProspectorPlayerController::DoPanCommand(const FInputActionValue& Value)
{
	// F only ever acts on the selected unit - see DoDigCommand.
	AProspectorCharacter* Prospector = Cast<AProspectorCharacter>(SelectedUnit.Get());
	if (!Prospector)
	{
		return;
	}

	if (IsShiftHeld())
	{
		Prospector->EnqueuePanJob();
	}
	else
	{
		Prospector->ClearJobQueue();
		Prospector->RequestPan();
	}
}

void AProspectorPlayerController::DoPanTilt(const FInputActionValue& Value)
{
	// Mouse pan-tilt input only ever reaches the selected unit - see DoDigCommand.
	const FVector2D Tilt = Value.Get<FVector2D>();
	if (AProspectorCharacter* Prospector = Cast<AProspectorCharacter>(SelectedUnit.Get()))
	{
		Prospector->RequestPanTilt(Tilt);
	}
}

void AProspectorPlayerController::DoSelectCommand(const FInputActionValue& Value)
{
	FHitResult Hit;
	const bool bHit = GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Pawn), false, Hit) && Hit.bBlockingHit;

	if (bHit && Hit.GetActor() && Cast<ISelectableUnit>(Hit.GetActor()))
	{
		SelectUnit(Hit.GetActor());
	}
	else
	{
		DeselectCurrentUnit();
	}
}

void AProspectorPlayerController::DoDeselectCommand(const FInputActionValue& Value)
{
	DeselectCurrentUnit();
}

void AProspectorPlayerController::DoTabCycleCommand(const FInputActionValue& Value)
{
	TArray<AActor*> SelectableActors;
	UGameplayStatics::GetAllActorsWithInterface(this, USelectableUnit::StaticClass(), SelectableActors);
	SelectableActors.RemoveAll([](const AActor* Actor) { return !IsValid(Actor); });

	if (SelectableActors.Num() == 0)
	{
		return;
	}

	// Stable, deterministic order regardless of world-scan iteration order.
	SelectableActors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetFName().LexicalLess(B.GetFName());
	});

	// If the currently selected unit isn't in the (still-valid) list - including the case where it was
	// destroyed and SelectedUnit already resolved to null - start over from the first entry.
	int32 NextIndex = 0;
	if (AActor* Current = SelectedUnit.Get())
	{
		const int32 CurrentIndex = SelectableActors.IndexOfByKey(Current);
		if (CurrentIndex != INDEX_NONE)
		{
			NextIndex = (CurrentIndex + 1) % SelectableActors.Num();
		}
	}

	// SelectUnit no-ops if this is already the selected actor (single-unit case: Tab keeps Bill selected,
	// no flicker), and otherwise reuses the normal deselect-then-select cleanup (ends manual movement on
	// the outgoing unit, toggles the visual state on both, and transfers follow if it was active).
	SelectUnit(SelectableActors[NextIndex]);
}

void AProspectorPlayerController::DoCenterFollowCommand(const FInputActionValue& Value)
{
	AActor* Selected = SelectedUnit.Get();
	if (!Selected)
	{
		return;
	}

	ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn());
	if (!Cam)
	{
		return;
	}

	if (Cam->IsFollowingActor())
	{
		Cam->StopFollowingActor();
	}
	else
	{
		Cam->BeginFollowingActor(Selected);
	}
}
