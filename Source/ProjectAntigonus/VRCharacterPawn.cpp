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

AVRCharacterPawn::AVRCharacterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	mc_y_thumbleft = 0.0f;
	mc_x_thumbleft = 0.0f;
	mc_y_thumbright = 0.0f;
	mc_x_thumbright = 0.0f;

	//---Player Settings---//
	def_playerheight = 180.0f;
	b_psvrcontrollerrollrotation = false;

	//---Player State---//
	b_isteleporting = false;

	//---Teleport Camera Settings---//
	camerafadeoutduration = 0.1f;
	camerafadeinduration = 0.2;
	cameratelefadecolor = FLinearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	//---Components---//
	vrorigin = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraorigin"));
	vrorigin->SetupAttachment(RootComponent);
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	camera->SetupAttachment(vrorigin);

	//---Blueprint Controllers---//
	static ConstructorHelpers::FObjectFinder<UBlueprint> blueprintcontroller(TEXT("Blueprint'/Game/VirtualReality/Mannequin/Character/BP_VRMotionControllers.BP_VRMotionControllers'"));
	if (blueprintcontroller.Object) {
		handcontroller = (UClass*)blueprintcontroller.Object->GeneratedClass;
	}
	lefthand = nullptr;
	righthand = nullptr;
}

// Called when the game starts or when spawned
void AVRCharacterPawn::BeginPlay()
{
	Super::BeginPlay();

	SetupVROptions();
}

// Called every frame
void AVRCharacterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (lefthand->GetIsTeleActive())
	{
		lefthand->SetTeleRotation(GetRotationFromInput(mc_y_thumbleft, mc_x_thumbleft, lefthand));
	}

	if (righthand->GetIsTeleActive())
	{
		righthand->SetTeleRotation(GetRotationFromInput(mc_y_thumbright, mc_x_thumbright, righthand));
	}
}

// Called to bind functionality to input
void AVRCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent)
	PlayerInputComponent->BindAction("GrabLeft", IE_Pressed, this, &AVRCharacterPawn::GrabLeft_Pressed);
	PlayerInputComponent->BindAction("GrabLeft", IE_Released, this, &AVRCharacterPawn::GrabLeft_Released);
	PlayerInputComponent->BindAction("GrabRight", IE_Pressed, this, &AVRCharacterPawn::GrabRight_Pressed);
	PlayerInputComponent->BindAction("GrabRight", IE_Released, this, &AVRCharacterPawn::GrabRight_Released);

	PlayerInputComponent->BindAction("TeleportLeft", IE_Pressed, this, &AVRCharacterPawn::TeleportLeft_Pressed);
	PlayerInputComponent->BindAction("TeleportLeft", IE_Released, this, &AVRCharacterPawn::TeleportLeft_Released);
	PlayerInputComponent->BindAction("TeleportRight", IE_Pressed, this, &AVRCharacterPawn::TeleportRight_Pressed);
	PlayerInputComponent->BindAction("TeleportRight", IE_Released, this, &AVRCharacterPawn::TeleportRight_Released);

	PlayerInputComponent->BindAxis("MotionControllerThumbLeft_Y", this, &AVRCharacterPawn::GetLeftY);
	PlayerInputComponent->BindAxis("MotionControllerThumbLeft_X", this, &AVRCharacterPawn::GetLeftX);
	PlayerInputComponent->BindAxis("MotionControllerThumbRight_Y", this, &AVRCharacterPawn::GetRightY);
	PlayerInputComponent->BindAxis("MotionControllerThumbRight_X", this, &AVRCharacterPawn::GetRightX);
}

void AVRCharacterPawn::GetLeftY(float value)
{
	mc_y_thumbleft = value;
}
void AVRCharacterPawn::GetLeftX(float value)
{
	mc_x_thumbleft = value;
}
void AVRCharacterPawn::GetRightY(float value)
{
	mc_y_thumbright = value;
}
void AVRCharacterPawn::GetRightX(float value)
{
	mc_x_thumbright = value;
}

//---Setup---//
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
		vrorigin->AddLocalOffset(FVector{ 0.0f, 0.0f, def_playerheight }, false, false);
		b_psvrcontrollerrollrotation = true;
	}

	//Spawn and attach both motion controllers
	UWorld* worldexist{ GetWorld() };
	if (worldexist)
	{
		FActorSpawnParameters spawnparams;
		spawnparams.Owner = this;
		spawnparams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		temphand = EControllerHand::Left;
		lefthand = GetWorld()->SpawnActor<AVRMotionController>(handcontroller, FVector{}, FRotator{}, spawnparams);
		FAttachmentTransformRules rules{ EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false };
		lefthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));

		temphand = EControllerHand::Right;
		righthand = GetWorld()->SpawnActor<AVRMotionController>(handcontroller, FVector{}, FRotator{}, spawnparams);
		righthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));

		/*
		//Left
		AVRMotionController* left_deferredhand{ Cast<AVRMotionController>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, handcontroller, FTransform{}, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, this)) };
		if (left_deferredhand)
		{
			left_deferredhand->Init(EControllerHand::Left);
			UGameplayStatics::FinishSpawningActor(left_deferredhand, FTransform{});
			lefthand = left_deferredhand;
			FAttachmentTransformRules rules{ EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false };
			lefthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));
		}

		//Right
		AVRMotionController* right_deferredhand{ Cast<AVRMotionController>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, handcontroller, FTransform{}, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, this)) };
		if (right_deferredhand)
		{
			right_deferredhand->Init(EControllerHand::Right);
			UGameplayStatics::FinishSpawningActor(right_deferredhand, FTransform{});
			righthand = right_deferredhand;
			FAttachmentTransformRules rules{ EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false };
			righthand->AttachToComponent(vrorigin, rules, FName(TEXT("")));
		}
		*/
	}
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
			camerafadedelegate.BindUFunction(this, FName(TEXT("DelayTeleportation")), &controller);
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

//---Motion Controller Inputs---//
//Left grab
void AVRCharacterPawn::GrabLeft_Pressed()
{
	lefthand->GrabActor();
}
void AVRCharacterPawn::GrabLeft_Released()
{
	lefthand->ReleaseActor();
}
//Right grab
void AVRCharacterPawn::GrabRight_Pressed()
{
	righthand->GrabActor();
}
void AVRCharacterPawn::GrabRight_Released()
{
	righthand->ReleaseActor();
}
//Left teleport
void AVRCharacterPawn::TeleportLeft_Pressed()
{
	lefthand->ActivateTele();
	righthand->DisableTele();
}
void AVRCharacterPawn::TeleportLeft_Released()
{
	if (lefthand->GetIsTeleActive())
	{
		HandleTeleportation(lefthand);
	}
}
//Right teleport
void AVRCharacterPawn::TeleportRight_Pressed()
{
	righthand->ActivateTele();
	lefthand->DisableTele();
}

void AVRCharacterPawn::TeleportRight_Released()
{
	if (righthand->GetIsTeleActive())
	{
		HandleTeleportation(righthand);
	}
}

FRotator AVRCharacterPawn::GetRotationFromInput(float upaxis, float rightaxis, AVRMotionController *& vrcontroller)
{
	FRotator returnvalue{ 0.0f };
	//Use PSVR method if true, desktop with thumbstick support if false
	if (b_psvrcontrollerrollrotation)
	{

	}
	else
	{
		float thumbdeadzone{ 0.0f };
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

EControllerHand AVRCharacterPawn::GetPawnHand()
{
	return temphand;
}