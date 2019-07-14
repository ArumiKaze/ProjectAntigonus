#include "VRCharacterPawn.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "Components/InputComponent.h"
#include "HeadMountedDisplay.h"
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "MotionControllerComponent.h"
#include "VRMotionController.h"

//---Contructor---//
AVRCharacterPawn::AVRCharacterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	//---Components---//
	vrorigin = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraorigin"));
	vrorigin->SetupAttachment(RootComponent);
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	camera->SetupAttachment(vrorigin);

	//---Load blueprint object---//
	static ConstructorHelpers::FObjectFinder<UBlueprint> blueprintcontroller(TEXT("Blueprint'/Game/VirtualReality/Mannequin/Character/BP_VRMotionControllers.BP_VRMotionControllers'"));
	if (blueprintcontroller.Object) {
		m_handcontroller = (UClass*)blueprintcontroller.Object->GeneratedClass;
	}

	//---Hand controllers---//
	m_lefthand = nullptr;
	m_righthand = nullptr;

	//---Desired hand---//
	m_desiredhand = EControllerHand::AnyHand;

	//---Player Settings---//
	m_defplayerheight = 180.0f;
	m_b_psvrcontrollerrollrotation = false;

	//---Controller Input Axis Values---//
	m_leftthumb_y = 0.0f;
	m_leftthumb_x = 0.0f;
	m_rightthumb_y = 0.0f;
	m_rightthumb_x = 0.0f;

	///////////////////////////////////////////////////////////////////////////

	//---Player State---//
	b_isteleporting = false;

	//---Teleport Camera Settings---//
	camerafadeoutduration = 0.1f;
	camerafadeinduration = 0.2;
	cameratelefadecolor = FLinearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
}

//---Begin Play---//
void AVRCharacterPawn::BeginPlay()
{
	Super::BeginPlay();

	SetupVROptions();
}

//---Tick---//
void AVRCharacterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_lefthand->GetIsTeleActive())
	{
		m_lefthand->SetTeleRotation(RotationFromInput(m_leftthumb_y, m_leftthumb_x, m_lefthand));
	}

	if (m_righthand->GetIsTeleActive())
	{
		m_righthand->SetTeleRotation(RotationFromInput(m_rightthumb_y, m_rightthumb_x, m_righthand));
	}
}

//---Player Input Component---//
void AVRCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent)

	PlayerInputComponent->BindAxis("MotionControllerThumbLeft_Y", this, &AVRCharacterPawn::LeftYThumb);
	PlayerInputComponent->BindAxis("MotionControllerThumbLeft_X", this, &AVRCharacterPawn::LeftXThumb);
	PlayerInputComponent->BindAxis("MotionControllerThumbRight_Y", this, &AVRCharacterPawn::RightYThumb);
	PlayerInputComponent->BindAxis("MotionControllerThumbRight_X", this, &AVRCharacterPawn::RightXThumb);

	PlayerInputComponent->BindAction("GrabLeft", IE_Pressed, this, &AVRCharacterPawn::GrabLeft_Pressed);
	PlayerInputComponent->BindAction("GrabLeft", IE_Released, this, &AVRCharacterPawn::GrabLeft_Released);
	PlayerInputComponent->BindAction("GrabRight", IE_Pressed, this, &AVRCharacterPawn::GrabRight_Pressed);
	PlayerInputComponent->BindAction("GrabRight", IE_Released, this, &AVRCharacterPawn::GrabRight_Released);

	PlayerInputComponent->BindAction("TeleportLeft", IE_Pressed, this, &AVRCharacterPawn::TeleportLeft_Pressed);
	PlayerInputComponent->BindAction("TeleportLeft", IE_Released, this, &AVRCharacterPawn::TeleportLeft_Released);
	PlayerInputComponent->BindAction("TeleportRight", IE_Pressed, this, &AVRCharacterPawn::TeleportRight_Pressed);
	PlayerInputComponent->BindAction("TeleportRight", IE_Released, this, &AVRCharacterPawn::TeleportRight_Released);
}

//---Controller Input Axis Functions---//
void AVRCharacterPawn::LeftYThumb(float value)
{
	if (Controller)
	{
		m_leftthumb_y = value;
	}
}
void AVRCharacterPawn::LeftXThumb(float value)
{
	if (Controller)
	{
		m_leftthumb_x = value;
	}
}
void AVRCharacterPawn::RightYThumb(float value)
{
	if (Controller)
	{
		m_rightthumb_y = value;
	}
}
void AVRCharacterPawn::RightXThumb(float value)
{
	if (Controller)
	{
		m_rightthumb_x = value;
	}
}

//---Controller Input Actions---//
//Left grab
void AVRCharacterPawn::GrabLeft_Pressed()
{
	if (m_lefthand)
	{
		m_lefthand->GrabActor();
	}
}
void AVRCharacterPawn::GrabLeft_Released()
{
	if (m_lefthand)
	{
		m_lefthand->ReleaseActor();
	}
}
//Right grab
void AVRCharacterPawn::GrabRight_Pressed()
{
	if (m_righthand)
	{
		m_righthand->GrabActor();
	}
}
void AVRCharacterPawn::GrabRight_Released()
{
	if (m_righthand)
	{
		m_righthand->ReleaseActor();
	}
}
//Left teleport
void AVRCharacterPawn::TeleportLeft_Pressed()
{
	m_lefthand->ActivateTele();
	m_righthand->DisableTele();
}
void AVRCharacterPawn::TeleportLeft_Released()
{
	if (m_lefthand->GetIsTeleActive())
	{
		HandleTeleportation(m_lefthand);
	}
}
//Right teleport
void AVRCharacterPawn::TeleportRight_Pressed()
{
	m_righthand->ActivateTele();
	m_lefthand->DisableTele();
}
void AVRCharacterPawn::TeleportRight_Released()
{
	if (m_righthand->GetIsTeleActive())
	{
		HandleTeleportation(m_righthand);
	}
}

//---Set up player height and both controllers---//
void AVRCharacterPawn::SetupVROptions()
{
	//Set up player height for various platforms
	FName HMDname{ UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName() };
	if (HMDname == "OculusHMD" || HMDname == "SteamVR")
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}
	else if (HMDname == "PSVR")
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
		vrorigin->AddLocalOffset(FVector{ 0.0f, 0.0f, m_defplayerheight }, false, false);
		m_b_psvrcontrollerrollrotation = true;
	}

	//Spawn and attach both motion controllers
	UWorld* worldexist{ GetWorld() };
	if (worldexist)
	{
		FActorSpawnParameters spawnparams;
		spawnparams.Owner = this;
		spawnparams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		m_desiredhand = EControllerHand::Left;
		m_lefthand = GetWorld()->SpawnActor<AVRMotionController>(m_handcontroller, FVector{}, FRotator{}, spawnparams);
		FAttachmentTransformRules rules{ EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false };
		m_lefthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));

		m_desiredhand = EControllerHand::Right;
		m_righthand = GetWorld()->SpawnActor<AVRMotionController>(m_handcontroller, FVector{}, FRotator{}, spawnparams);
		m_righthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));
	}
}

//---Calculate teleport rotation---//
FRotator AVRCharacterPawn::RotationFromInput(float upaxis, float rightaxis, AVRMotionController *& vrcontroller)
{
	FRotator returnvalue{ 0.0f };

	//Use PSVR method if true, desktop with thumbstick support if false
	if (m_b_psvrcontrollerrollrotation)
	{
		//Multiply by 3 at the end to make 180 spins of oreintation much easier
		float temp{ UKismetMathLibrary::ConvertTransformToRelative(FTransform{ vrcontroller->GetInitControllerRot(), FVector{ 0.0f }, FVector{ 1.0f } }, vrcontroller->GetMotionControllerComponent().GetComponentTransform()).Rotator().Roll * 3.0f };

		//Return roll differenmce since we last initiated teleport, allows wrist to change the pawn orientation when teleporting
		returnvalue = FRotator{ 0.0f, temp + GetActorRotation().Yaw, 0.0f };
	}
	else
	{
		//Adjust this value to narrow the 'deadzone' center
		float thumbdeadzone{ 0.7f };
		//Check whether thumb is near the center to ignore rotation overrides
		if ((abs(upaxis) + abs(rightaxis)) >= thumbdeadzone)
		{
			//Rotate input X+Y to always point forward relative to the current pawn rotation
			FVector temp{ upaxis, rightaxis, 0.0f };
			temp.Normalize(0.0001f);
			returnvalue = UKismetMathLibrary::MakeRotFromX(UKismetMathLibrary::Quat_RotateVector(FRotator{ 0.0f, GetActorRotation().Yaw, 0.0f }.Quaternion(), temp));
		}
		else
		{
			returnvalue = GetActorRotation();
		}
	}

	return returnvalue;
}

//---Handle Teleportation---//
void AVRCharacterPawn::HandleTeleportation(AVRMotionController *&controller)
{
	if (!b_isteleporting)
	{
		if (controller->GetIsValidTeleDest())
		{
			b_isteleporting = true;
			UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->StartCameraFade(0.0f, 1.0f, camerafadeoutduration, cameratelefadecolor, false, true);
			camerafadedelegate.BindUFunction(this, FName(TEXT("DelayTeleportation")), controller);
			GetWorldTimerManager().SetTimer(camerafadetimer, camerafadedelegate, camerafadeoutduration, false);
		}
		else
		{
			controller->DisableTele();
		}
	}
}
void AVRCharacterPawn::DelayTeleportation(AVRMotionController *&delaycontroller)
{
	delaycontroller->DisableTele();
	TeleportTo(delaycontroller->GetTeleDestLoc(), delaycontroller->GetTeleDestRot());
	UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->StartCameraFade(1.0f, 0.0f, camerafadeinduration, cameratelefadecolor, false, false);
	b_isteleporting = false;
}

//---Getter---//
//Get desired hand, left or right, for hand creation
EControllerHand AVRCharacterPawn::GetPawnHand()
{
	return m_desiredhand;
}