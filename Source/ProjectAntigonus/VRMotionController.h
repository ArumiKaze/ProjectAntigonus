#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMotionController.generated.h"

UENUM(BlueprintType)
enum class E_GRIPSTATE : uint8
{
	GRIP_OPEN,
	GRIP_CANGRAB,
	GRIP_GRAB
};

UCLASS()
class PROJECTANTIGONUS_API AVRMotionController : public AActor
{
	GENERATED_BODY()

private:

	EControllerHand m_hand;

	float telelaunchvelocity;

	bool b_isroomscale;

	bool b_shouldgrip;

	bool b_isteleactive;
	bool b_isvalidteledestination;
	bool b_isvalidpreviousframeteledestination;

	FRotator telerotation;

	FRotator initialcontrollerrotation;

	UPROPERTY()
	UHapticFeedbackEffect_Base *haptic_motioncontroller;

	UPROPERTY()
	class AVRPickupObject *attachedactor;

	UPROPERTY()
	UStaticMesh* mesh_beamspline;
	UPROPERTY()
	UMaterial* mat_spline;

	UPROPERTY()
	UStaticMeshComponent* arcendpoint;

	//UPROPERTY()
	//TArray<FVector> array_arcpoints;

	UPROPERTY()
	TArray<class USplineMeshComponent*> array_splinemeshes;

	//---Room scale set up---//
	void SetupRoomScaleOutline();
	void UpdateRoomScaleOutline();

	//---Controller rumble---//
	void RumbleController(float intensity);

	//---Grabbing---//
	UFUNCTION()
	AActor*& GetActorNearHand(AActor *&actor);

	//---Teleportation Arc---//
	void HandleTeleportationArc();

	//---Rumble Events---//
	void GrabSphereOnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void ControllerMeshOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent *scene;

	//---Hand---//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent *motioncontroller;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent *handmesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* arcdirection;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USplineComponent* arcspline;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent *grabsphere;

	//---Teleport Cylinder---//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *teleportcylinder;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *ring;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *arrow;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *roomscalemesh;

	//---SteamVRChaperone---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USteamVRChaperoneComponent *steamvrchaperone;

	//---Grip Enum---//
	UPROPERTY(BlueprintReadOnly)
	E_GRIPSTATE gripstate;

	virtual void BeginPlay() override;

public:	

	AVRMotionController();

	virtual void Tick(float DeltaTime) override;

	//---Initialize hand---//
	void Init(EControllerHand p_hand);

	//---Teleportation---//
	void ActivateTele();
	void DisableTele();

	//---Grab and Release Actors---//
	void GrabActor();
	void ReleaseActor();

	//---Getter---//
	void SetTeleRotation(FRotator newrot);
	bool GetIsTeleActive();
	bool GetIsValidTeleDest();
	FVector GetTeleDestLoc();
	FRotator GetTeleDestRot();
};
