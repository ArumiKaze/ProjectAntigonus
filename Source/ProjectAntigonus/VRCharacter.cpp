#include "VRCharacter.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "Components/InputComponent.h"
#include "HeadMountedDisplay.h"
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "Runtime/HeadMountedDisplay/Public/IXRTrackingSystem.h"
#include "MotionControllerComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"

AVRCharacter::AVRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraorigin"));
	VROrigin->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	LeftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
	LeftHand->SetTrackingSource(EControllerHand::Left);
	LeftHand->AttachToComponent(VROrigin, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false));
	
	RightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
	RightHand->SetTrackingSource(EControllerHand::Right);
	RightHand->AttachToComponent(VROrigin, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false));

	bPositionalHeadTracking = false;
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SetupVROptions();
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent)
	PlayerInputComponent->BindAction("GrabLeft", IE_Pressed, this, &AVRCharacter::GrabLeft_Pressed);
	PlayerInputComponent->BindAction("GrabLeft", IE_Released, this, &AVRCharacter::GrabLeft_Released);
	PlayerInputComponent->BindAction("GrabRight", IE_Pressed, this, &AVRCharacter::GrabRight_Pressed);
	PlayerInputComponent->BindAction("GrabRight", IE_Released, this, &AVRCharacter::GrabRight_Released);

	PlayerInputComponent->BindAction("TeleportLeft", IE_Pressed, this, &AVRCharacter::TeleportLeft_Pressed);
	PlayerInputComponent->BindAction("TeleportLeft", IE_Released, this, &AVRCharacter::TeleportLeft_Released);
	PlayerInputComponent->BindAction("TeleportRight", IE_Pressed, this, &AVRCharacter::TeleportRight_Pressed);
	PlayerInputComponent->BindAction("TeleportRight", IE_Released, this, &AVRCharacter::TeleportRight_Released);
}

void AVRCharacter::SetupVROptions()
{
	IHeadMountedDisplay* HMD{ GEngine->XRSystem->GetHMDDevice() };
	if (HMD && HMD->IsHMDEnabled())
	{
		HMD->EnableHMD(bPositionalHeadTracking);
		if (!bPositionalHeadTracking)
		{
			Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		}

		FName HMDname{ UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName() };
		if (HMDname == "OculusHMD" || HMDname == "SteamVR")
		{
			UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
		}
	}
}

void AVRCharacter::ResetHMDOrigin()
{
	IHeadMountedDisplay* HMD{ GEngine->XRSystem->GetHMDDevice() };
	if (HMD && HMD->IsHMDEnabled())
	{
		UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
	}
}

void AVRCharacter::ToggleTrackingSpace()
{
}

//---Motion Controller Inputs---//
//Left grab
void AVRCharacter::GrabLeft_Pressed()
{
}
void AVRCharacter::GrabLeft_Released()
{
}
//Right grab
void AVRCharacter::GrabRight_Pressed()
{
}
void AVRCharacter::GrabRight_Released()
{
}
//Left teleport
void AVRCharacter::TeleportLeft_Pressed()
{
}
void AVRCharacter::TeleportLeft_Released()
{
}
//Right teleport
void AVRCharacter::TeleportRight_Pressed()
{
}

void AVRCharacter::TeleportRight_Released()
{
}
