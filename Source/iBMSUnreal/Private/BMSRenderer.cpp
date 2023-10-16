// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSRenderer.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
// Sets default values
ABMSRenderer::ABMSRenderer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABMSRenderer::BeginPlay()
{
	Super::BeginPlay();
	// spawn note sprite for test
	// auto note = GetWorld()->SpawnActor<APaperSpriteActor>();
	// note->GetRenderComponent()->Mobility = EComponentMobility::Movable;
	// note->GetRenderComponent()->SetSprite(NoteSprite);
	// // translucent unlit material
	// note->GetRenderComponent()->SetMaterial(0, LoadObject<UMaterialInterface>(NULL, TEXT("/Game/Materials/TranslucentUnlitSpriteMaterial")));
	// // attach to note area
	// note->AttachToActor(NoteArea, FAttachmentTransformRules::KeepRelativeTransform);
	// // set position
	// note->SetActorRelativeLocation(FVector(0, 0, 0));
	// // set scale
	// note->SetActorScale3D(FVector(1, 1, 1));
	
	
}

// Called every frame
void ABMSRenderer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	float mouseX;
	float mouseY;
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetMousePosition(mouseX, mouseY);
	UE_LOG(LogTemp, Warning, TEXT("Mouse Location: %f, %f"), mouseX, mouseY);
	FVector worldPosition;
	FVector worldDirection;
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->DeprojectScreenPositionToWorld(mouseX, mouseY, worldPosition, worldDirection);
	worldPosition = worldPosition / GNearClippingPlane * 10;
	UE_LOG(LogTemp, Warning, TEXT("World Location: %f, %f, %f"), worldPosition.X, worldPosition.Y, worldPosition.Z);

}

