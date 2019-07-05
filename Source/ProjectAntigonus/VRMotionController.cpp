#include "VRMotionController.h"
#include "MotionControllerComponent.h"
#include "SteamVRChaperoneComponent.h"
#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Haptics/HapticFeedbackEffect_Base.h"
#include "Runtime/Engine/Classes/Components/SphereComponent.h"
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "Runtime/Engine/Classes/Components/SplineMeshComponent.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "Runtime/Engine/Classes/Components/ArrowComponent.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include "VRPickupObject.h"

AVRMotionController::AVRMotionController()
{
	telelaunchvelocity = 900.0f;

	//Attached actor object
	attachedactor = nullptr;

	//Grip enum state
	gripstate = E_GRIPSTATE::GRIP_OPEN;

	b_isroomscale = false;

	b_shouldgrip = false;

	b_isteleactive = false;
	b_isvalidteledestination = false;
	b_isvalidpreviousframeteledestination = false;

	PrimaryActorTick.bCanEverTick = true;

	telerotation = FRotator{ 0.0f };

	initialcontrollerrotation = FRotator{ 0.0f };

	scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

	//Load hand
	motioncontroller = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	motioncontroller->SetupAttachment(scene);
	motioncontroller->SetTrackingSource(EControllerHand::Left);
	if (m_hand == EControllerHand::Left)
	{
		motioncontroller->MotionSource = FName(TEXT("Left"));
	}
	else
	{
		motioncontroller->MotionSource = FName(TEXT("Right"));
	}
	handmesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	handmesh->SetupAttachment(motioncontroller);
	arcdirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	arcdirection->SetupAttachment(handmesh);
	arcspline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	arcspline->SetupAttachment(handmesh);
	grabsphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	grabsphere->SetupAttachment(handmesh);
	grabsphere->SetSphereRadius(10.0f);

	//Load arc end point
	arcendpoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArcEndPoint"));
	arcendpoint->SetupAttachment(scene);

	//Load Teleport Cylinder meshes and materials
	teleportcylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCylinder"));
	teleportcylinder->SetupAttachment(scene);
	ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	ring->SetupAttachment(teleportcylinder);
	arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	arrow->SetupAttachment(teleportcylinder);
	roomscalemesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomScaleMesh"));
	roomscalemesh->SetupAttachment(arrow);

	//Load SteamVR Chaperone
	steamvrchaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("SteamVRChaperone"));

	//Load haptic feedback curve
	static ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> hapticcurve(TEXT("HapticFeedbackEffect_Curve'/Game/VirtualReality/Mannequin/Character/HapticCurve_MotionController.HapticCurve_MotionController'"));
	if (hapticcurve.Object) {
		haptic_motioncontroller = hapticcurve.Object;
	}

	//
	static ConstructorHelpers::FObjectFinder<UStaticMesh> beam(TEXT("StaticMesh'/Game/VirtualReality/Meshes/BeamMesh.BeamMesh'"));
	if (beam.Object) {
		mesh_beamspline = beam.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> spline(TEXT("Material'/Game/VirtualReality/Materials/M_SplineArcMat.M_SplineArcMat'"));
	if (spline.Object) {
		mat_spline = spline.Object;
	}
}


void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();
	
	SetupRoomScaleOutline();

	//Hide teleport cylinder until activation
	teleportcylinder->SetVisibility(false, true);
	roomscalemesh->SetVisibility(false, true);

	//Invert scale on hand mesh to create left hand
	if (m_hand == EControllerHand::Left)
	{
		handmesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}

	grabsphere->OnComponentBeginOverlap.AddDynamic(this, &AVRMotionController::GrabSphereOnOverlap);
	handmesh->OnComponentHit.AddDynamic(this, &AVRMotionController::ControllerMeshOnHit);
}

void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update animation of hand
	if ((attachedactor != nullptr) || b_shouldgrip)
	{
		gripstate = E_GRIPSTATE::GRIP_GRAB;
	}
	else
	{
		if (GetActorNearHand() != nullptr)
		{
			gripstate = E_GRIPSTATE::GRIP_CANGRAB;
		}
		else
		{
			if (b_shouldgrip)
			{
				gripstate = E_GRIPSTATE::GRIP_GRAB;
			}
			else
			{
				gripstate = E_GRIPSTATE::GRIP_OPEN;
			}
		}
	}

	//Handle teleport arc
	HandleTeleportationArc();

	//Update room scale
	UpdateRoomScaleOutline();

	//Only let hand collide with environment while gripping
	switch (gripstate)
	{
	case E_GRIPSTATE::GRIP_OPEN:
	case E_GRIPSTATE::GRIP_CANGRAB:
		handmesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case E_GRIPSTATE::GRIP_GRAB:
		handmesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

//---Room scale set up---//
void AVRMotionController::SetupRoomScaleOutline()
{
	FVector OutRectCenter{ 0.0f };
	FRotator OutRectRotation{ 0.0f };
	float OutSideLengthX{ 0.0f };
	float OutSideLengthY{ 0.0f };
	UKismetMathLibrary::MinimumAreaRectangle(this, steamvrchaperone->GetBounds(), FVector(0.0f, 0.0f, 0.0f), OutRectCenter, OutRectRotation, OutSideLengthX, OutSideLengthY, false);

	//Measure Chaperone, default to 100x100 if roomscale is not used
	b_isroomscale = UKismetMathLibrary::BooleanNAND(UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthX, 100.0f, 0.01f), UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthY, 100.0f, 0.01f));
	if (b_isroomscale)
	{
		//Setup room-scale mesh (1x1x1 units in size by default) to the size of the room-scale dimensions
		float chaperonemeshheight{ 70.0f };
		roomscalemesh->SetWorldScale3D(FVector(OutSideLengthX, OutSideLengthY, chaperonemeshheight));
		roomscalemesh->SetRelativeRotation(OutRectRotation);
	}
}
void AVRMotionController::UpdateRoomScaleOutline()
{
	if (roomscalemesh->IsVisible())
	{
		FRotator hmdorientation{ 0.0f };
		FVector hmdposition{ 0.0f };
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(hmdorientation, hmdposition);
		
		FVector teleporttarget{ UKismetMathLibrary::Quat_UnrotateVector(FRotator{ 0.0f, hmdorientation.Yaw, 0.0f }.Quaternion() , FVector{ hmdposition.X, hmdposition.Y, 0.0f } / -1.0f) };
		roomscalemesh->SetRelativeLocation(teleporttarget, false, false);
	}
}

void AVRMotionController::RumbleController(float intensity)
{
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayHapticEffect(haptic_motioncontroller, m_hand, intensity, false);
}

//---Grabbing---//
//Get an actor near the hand
AActor* AVRMotionController::GetActorNearHand()
{
	float nearestoverlap{ 10000.0f };
	AActor* nearestoverlappingactor{ nullptr };

	TArray <AActor*> actorarray;
	grabsphere->GetOverlappingActors(actorarray, AActor::StaticClass());
	for (const auto &eachactor : actorarray)
	{
		//!!!!!!!!!!!!!!!!!! implement some kind of check to filter actors that are interactible
		float tempoverlap{ (eachactor->GetActorLocation() - grabsphere->GetComponentLocation()).Size() };
		if ( tempoverlap < nearestoverlap)
		{
			nearestoverlappingactor = eachactor;
			nearestoverlap = tempoverlap;
		}
	}
	return nearestoverlappingactor;
}

//---Teleportation Arc---//
void AVRMotionController::HandleTeleportationArc()
{
	//Clear tele arc
	if (array_splinemeshes.IsValidIndex(0))
	{
		array_splinemeshes.Empty();
		/*
		while (array_splinemeshes.IsValidIndex(0))
		{
			//This destroys the component but it does not remove it from the array.
			array_splinemeshes[0]->DestroyComponent();
			array_splinemeshes.RemoveAt(0);
		}
		*/
	}
	arcspline->ClearSplinePoints(true);

	//Trace teleport destionation
	bool tracetelesuccess{ false };
	//TArray<FPredictProjectilePathPointData> array_tracepoints;
	TArray<FVector> array_tracepoints;
	FVector navmeshlocation{ 0.0f };
	FVector tracelocation{ 0.0f };
	if (b_isteleactive)
	{
		FPredictProjectilePathParams params;
		params.StartLocation = FVector{ arcdirection->GetComponentLocation() };
		params.LaunchVelocity = FVector{ arcdirection->GetForwardVector() * telelaunchvelocity };
		params.bTraceWithCollision = true;
		params.ProjectileRadius = 0.0f;
		TArray<TEnumAsByte<EObjectTypeQuery>> teleboundry;
		//teleboundry.Add(EObjectTypeQuery::ObjectTypeQuery1);
		teleboundry.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		params.ObjectTypes = teleboundry;
		params.bTraceComplex = false;
		params.SimFrequency = 30.0f;

		FPredictProjectilePathResult result;

		bool b_hit{ UGameplayStatics::PredictProjectilePath(this, params, result) };

		UNavigationSystemV1* navsystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
		FNavLocation projectedlocation;
		//bool b_projectpoint{ navsystem->ProjectPointToNavigation(result.HitResult.Location, projectedlocation, FVector{ 500.0f }, (ANavigationData*)0, 0) };
		

		//tracetelesuccess = b_hit && b_projectpoint;
		for (const auto &eachpoint : result.PathData)
		{
			array_tracepoints.Emplace(eachpoint.Location);
		}
		navmeshlocation = projectedlocation;
		tracelocation = result.HitResult.Location;
	}

	b_isvalidteledestination = tracetelesuccess;
	teleportcylinder->SetVisibility(b_isvalidteledestination, true);

	TArray<TEnumAsByte<EObjectTypeQuery>> floorboundry;
	floorboundry.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	FHitResult outhit;
	UKismetSystemLibrary::LineTraceSingleForObjects(this, navmeshlocation, FVector{ 0.0f, 0.0f, navmeshlocation.Z + -200.0f }, floorboundry, false, TArray<AActor*>(), EDrawDebugTrace::None, outhit, true);

	FVector newlocation{ outhit.bBlockingHit ? outhit.ImpactPoint : navmeshlocation };
	teleportcylinder->SetWorldLocation(newlocation, false, nullptr, ETeleportType::TeleportPhysics);

	//Rumble controller when a valid teleport location is found
	if ((b_isvalidteledestination && !b_isvalidpreviousframeteledestination) || (b_isvalidpreviousframeteledestination && !b_isvalidteledestination))
	{
		RumbleController(0.3);
	}

	b_isvalidpreviousframeteledestination = tracetelesuccess;

	//Create small stub line when it fails to find a teleport location
	if (!tracetelesuccess)
	{
		if (array_tracepoints.IsValidIndex(0))
		{
			array_tracepoints.Empty();
		}
		array_tracepoints.Emplace(arcdirection->GetComponentLocation());
		array_tracepoints.Emplace(arcdirection->GetComponentLocation() + (arcdirection->GetForwardVector() * 20.0f));
	}

	//Build spline from all trace points, generates tangets to build a smoothly curved spline mesh
	for (const auto &eachdata : array_tracepoints)
	{
		arcspline->AddSplinePoint(eachdata, ESplineCoordinateSpace::Local, true);
	}

	//Update the point type to create the curve
	arcspline->SetSplinePointType(array_tracepoints.Num(), ESplinePointType::CurveClamped, true);

	for (int i{ 0 }; i < arcspline->GetNumberOfSplinePoints() - 2; ++i)
	{

		USplineMeshComponent* splinemesh = NewObject<USplineMeshComponent>(arcspline);

		splinemesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		RegisterAllComponents();
		splinemesh->SetMobility(EComponentMobility::Movable);
		//splinemesh->SetupAttachment(arcspline);

		UMaterialInstanceDynamic* dynamicMat = UMaterialInstanceDynamic::Create(mat_spline, NULL);
		//dynamicMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(mSegments[i].mColor));

		splinemesh->bCastDynamicShadow = false;
		splinemesh->SetStaticMesh(mesh_beamspline);
		splinemesh->SetMaterial(0, dynamicMat);

		//Width of the mesh 
		//splinemesh->SetStartScale(FVector2D(50, 50));
		//splinemesh->SetEndScale(FVector2D(50, 50));

		array_splinemeshes.Emplace(splinemesh);

		FVector pointlocationstart{ array_tracepoints[i] };
		FVector pointtangentstart{ arcspline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local) };
		FVector pointlocationend{ array_tracepoints[i+1] };
		FVector pointtangentend{ arcspline->GetTangentAtSplinePoint(i+1, ESplineCoordinateSpace::Local) };

		//Set tangents and position to slightly bend the cylinder, all cylinders combined create a smooth arc
		splinemesh->SetStartAndEnd(pointlocationstart, pointtangentstart, pointlocationend, pointtangentend, true);
	}

	arcendpoint->SetVisibility(tracetelesuccess && b_isteleactive, false);
	arcendpoint->SetWorldLocation(tracelocation, false, nullptr, ETeleportType::TeleportPhysics);
	FRotator HMDrotation{ 0.0f };
	FVector HMDposition{ 0.0f };
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDrotation, HMDposition);
	arrow->SetWorldRotation(UKismetMathLibrary::ComposeRotators(telerotation, FRotator{ 0.0f, HMDrotation.Yaw, 0.0f }));
	roomscalemesh->SetWorldRotation(telerotation);
}

//---Rumble controller on overlap with valid static mesh---//
void AVRMotionController::GrabSphereOnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<UStaticMeshComponent>(OtherComp)->IsSimulatingPhysics())
	{
		RumbleController(0.8f);
	}
}
void AVRMotionController::ControllerMeshOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	RumbleController(UKismetMathLibrary::MapRangeClamped(NormalImpulse.Size(), 0.0f, 1500.0f, 0.0f, 0.8f));
}

//---Initialize hand---//
void AVRMotionController::Init(EControllerHand p_hand)
{
	m_hand = p_hand;
}

//---Teleportation---//
void AVRMotionController::ActivateTele()
{
	b_isteleactive = true;
	teleportcylinder->SetVisibility(true, true);
	//Only show during teleport if room-scale is available
	roomscalemesh->SetVisibility(b_isroomscale, false);
	//Store rotation for later to compare roll value to support wrist-based orientation of the teleporter
	initialcontrollerrotation = motioncontroller->GetComponentRotation();
}
void AVRMotionController::DisableTele()
{
	if (b_isteleactive)
	{
		b_isteleactive = false;
		teleportcylinder->SetVisibility(false, true);
		arcendpoint->SetVisibility(false, false);
		roomscalemesh->SetVisibility(false, false);
	}
}

//---Grab and Release Actors---//
void AVRMotionController::GrabActor()
{
	b_shouldgrip = true;
	AActor* actorexist{ GetActorNearHand() };
	if (actorexist)
	{
		attachedactor = actorexist;
		Cast<AVRPickupObject>(attachedactor)->Pickup(motioncontroller);
		RumbleController(0.7f);
	}
}
void AVRMotionController::ReleaseActor()
{
	b_shouldgrip = false;
	if (attachedactor)
	{
		if (attachedactor->GetRootComponent()->GetAttachParent() == motioncontroller)
		{
			Cast<AVRPickupObject>(attachedactor)->Drop();
			RumbleController(0.2f);
		}
		attachedactor = nullptr;
	}
}

//---Getter---//
void AVRMotionController::SetTeleRotation(FRotator newrot)
{
	telerotation = newrot;
}
bool AVRMotionController::GetIsTeleActive()
{
	return b_isteleactive;
}
bool AVRMotionController::GetIsValidTeleDest()
{
	return b_isvalidteledestination;
}
FVector AVRMotionController::GetTeleDestLoc()
{
	FVector pos{ 0.0f };
	FRotator ori{ 0.0f };
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(ori, pos);

	UKismetMathLibrary::Quat_RotateVector(telerotation.Quaternion(), FVector{ pos.X, pos.Y, 0.0f });

	return FVector{ teleportcylinder->GetComponentLocation() - UKismetMathLibrary::Quat_RotateVector(telerotation.Quaternion(), FVector{ pos.X, pos.Y, 0.0f }) };
}
FRotator AVRMotionController::GetTeleDestRot()
{
	return telerotation;
}
