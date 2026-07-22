// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProspectorPlayerController.h"
#include "ProspectorCharacter.h"
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
	}
}

bool AProspectorPlayerController::GetCursorGroundHit(FHitResult& OutHit) const
{
	return GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_WorldStatic), false, OutHit) && OutHit.bBlockingHit;
}

AProspectorCharacter* AProspectorPlayerController::GetProspector() const
{
	return Cast<AProspectorCharacter>(UGameplayStatics::GetActorOfClass(GetWorld(), AProspectorCharacter::StaticClass()));
}

bool AProspectorPlayerController::IsShiftHeld() const
{
	return IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
}

void AProspectorPlayerController::DoCameraPan(const FInputActionValue& Value)
{
	const FVector2D PanVector = Value.Get<FVector2D>();
	if (ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn()))
	{
		Cam->PanBy(PanVector, GetWorld()->GetDeltaSeconds());
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
	FHitResult Hit;
	const bool bHit = GetCursorGroundHit(Hit);
	UE_LOG(LogTemp, Warning, TEXT("[InputDiag] DoMoveCommand fired: CursorHit=%s Actor=%s Location=%s Prospector=%s"),
		bHit ? TEXT("true") : TEXT("false"),
		(bHit && Hit.GetActor()) ? *Hit.GetActor()->GetName() : TEXT("NULL"),
		*Hit.Location.ToString(),
		GetProspector() ? TEXT("valid") : TEXT("NULL"));

	if (bHit)
	{
		if (AProspectorCharacter* Prospector = GetProspector())
		{
			if (IsShiftHeld())
			{
				Prospector->EnqueueMoveJob(Hit.Location);
			}
			else
			{
				Prospector->ClearJobQueue();
				Prospector->RequestMoveTo(Hit.Location);
			}
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[InputDiag] Move command fired but no cursor hit."));
	}
}

void AProspectorPlayerController::DoDigCommand(const FInputActionValue& Value)
{
	FHitResult Hit;
	const bool bHit = GetCursorGroundHit(Hit);
	UE_LOG(LogTemp, Warning, TEXT("[InputDiag] DoDigCommand fired: CursorHit=%s Actor=%s Prospector=%s"),
		bHit ? TEXT("true") : TEXT("false"),
		(bHit && Hit.GetActor()) ? *Hit.GetActor()->GetName() : TEXT("NULL"),
		GetProspector() ? TEXT("valid") : TEXT("NULL"));

	if (bHit)
	{
		if (AProspectorCharacter* Prospector = GetProspector())
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
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[InputDiag] Dig key fired but no cursor hit."));
	}
}

void AProspectorPlayerController::DoPanCommand(const FInputActionValue& Value)
{
	if (AProspectorCharacter* Prospector = GetProspector())
	{
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
}

void AProspectorPlayerController::DoPanTilt(const FInputActionValue& Value)
{
	if (AProspectorCharacter* Prospector = GetProspector())
	{
		Prospector->RequestPanTilt(Value.Get<FVector2D>());
	}
}
